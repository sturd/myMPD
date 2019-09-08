/* myMPD
   (c) 2018-2019 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of:
   
   ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <limits.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <inttypes.h>

#include "api.h"
#include "utility.h"
#include "log.h"
#include "list.h"
#include "tiny_queue.h"
#include "config_defs.h"
#include "global.h"
#include "web_server.h"
#include "mpd_client.h"
#include "../dist/src/mongoose/mongoose.h"
#include "../dist/src/frozen/frozen.h"

//embed assets in release build
#ifndef DEBUG
#define INCBIN_PREFIX 
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "../dist/src/incbin/incbin.h"
//compressed assets
INCBIN(sw_min_js, "../dist/htdocs/sw.min.js.gz");
INCBIN(mympd_webmanifest, "../dist/htdocs/mympd.webmanifest.gz");
INCBIN(index_html, "../dist/htdocs/index.html.gz");
INCBIN(coverimage_notavailable_svg, "../dist/htdocs/assets/coverimage-notavailable.svg.gz");
INCBIN(coverimage_stream_svg, "../dist/htdocs/assets/coverimage-stream.svg.gz");
INCBIN(coverimage_loading_svg, "../dist/htdocs/assets/coverimage-loading.svg.gz");
INCBIN(bootstrap_min_css, "../dist/htdocs/css/bootstrap.min.css.gz");
INCBIN(mympd_min_css, "../dist/htdocs/css/mympd.min.css.gz");
INCBIN(keymap_min_js, "../dist/htdocs/js/keymap.min.js.gz");
INCBIN(mympd_min_js, "../dist/htdocs/js/mympd.min.js.gz");
INCBIN(i18n_min_js, "../dist/htdocs/js//i18n.min.js.gz");
INCBIN(bootstrap_nativ_v4_min_js, "../dist/htdocs/js/bootstrap-native-v4.min.js.gz");
//uncompressed assets
INCBIN(robots_txt, "../htdocs/robots.txt");
INCBIN(favicon_ico, "../htdocs/assets/favicon.ico");
INCBIN(appicon_192_png, "../htdocs/assets/appicon-192.png");
INCBIN(appicon_512_png, "../htdocs/assets/appicon-512.png");
INCBIN(appicon_167_png, "../htdocs/assets/appicon-167.png");
INCBIN(MaterialIcons_Regular_woff2, "../htdocs/assets/MaterialIcons-Regular.woff2");

//Todo: define asset struct
//uri, mime-type, compressed, incbin_size, incbin_data

#endif

//private definitions
static int is_websocket(const struct mg_connection *nc);
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data);
static void ev_handler_redirect(struct mg_connection *nc_http, int ev, void *ev_data);
static void send_ws_notify(struct mg_mgr *mgr, t_work_result *response);
static void send_api_response(struct mg_mgr *mgr, t_work_result *response);
static bool handle_api(int conn_id, const char *request, int request_len);
static int has_prefix(const struct mg_str *uri, const struct mg_str *prefix);

//public functions
bool web_server_init(void *arg_mgr, t_config *config, t_mg_user_data *mg_user_data) {
    struct mg_mgr *mgr = (struct mg_mgr *) arg_mgr;
    struct mg_connection *nc_https;
    struct mg_connection *nc_http;
    struct mg_bind_opts bind_opts_https;
    struct mg_bind_opts bind_opts_http;
    const char *err_https;
    const char *err_http;

    //initialize mgr user_data, malloced in main.c
    mg_user_data->config = config;
    mg_user_data->music_directory = strdup("");
    mg_user_data->rewrite_patterns = NULL;
    mg_user_data->conn_id = 1;
    mg_user_data->feat_library = false;
    size_t pics_directory_len = config->varlibdir_len + 6;
    mg_user_data->pics_directory = malloc(pics_directory_len);
    assert(mg_user_data->pics_directory);
    snprintf(mg_user_data->pics_directory, pics_directory_len, "%s/pics", config->varlibdir);
    //init monogoose mgr with mg_user_data
    mg_mgr_init(mgr, mg_user_data);
    
    //bind to webport
    memset(&bind_opts_http, 0, sizeof(bind_opts_http));
    bind_opts_http.error_string = &err_http;
    if (config->ssl == true) {
        nc_http = mg_bind_opt(mgr, config->webport, ev_handler_redirect, bind_opts_http);
    }
    else {
        nc_http = mg_bind_opt(mgr, config->webport, ev_handler, bind_opts_http);
    }
    if (nc_http == NULL) {
        LOG_ERROR("Can't bind to port %s: %s", config->webport, err_http);
        mg_mgr_free(mgr);
        return false;
    }
    mg_set_protocol_http_websocket(nc_http);
    LOG_INFO("Listening on http port %s", config->webport);

    //bind to sslport
    if (config->ssl == true) {
        memset(&bind_opts_https, 0, sizeof(bind_opts_https));
        bind_opts_https.error_string = &err_https;
        bind_opts_https.ssl_cert = config->ssl_cert;
        bind_opts_https.ssl_key = config->ssl_key;
        nc_https = mg_bind_opt(mgr, config->ssl_port, ev_handler, bind_opts_https);
        if (nc_https == NULL) {
            LOG_ERROR("Can't bind to port %s: %s", config->ssl_port, err_https);
            mg_mgr_free(mgr);
            return false;
        } 
        LOG_INFO("Listening on ssl port %s", config->ssl_port);
        LOG_DEBUG("Using certificate: %s", config->ssl_cert);
        LOG_DEBUG("Using private key: %s", config->ssl_key);
        mg_set_protocol_http_websocket(nc_https);
    }
    return mgr;
}

void web_server_free(void *arg_mgr) {
    struct mg_mgr *mgr = (struct mg_mgr *) arg_mgr;
    mg_mgr_free(mgr);
    mgr = NULL;
}

void *web_server_loop(void *arg_mgr) {
    struct mg_mgr *mgr = (struct mg_mgr *) arg_mgr;
    t_mg_user_data *mg_user_data = (t_mg_user_data *) mgr->user_data;
    while (s_signal_received == 0) {
        unsigned web_server_queue_length = tiny_queue_length(web_server_queue, 50);
        if (web_server_queue_length > 0) {
            t_work_result *response = tiny_queue_shift(web_server_queue, 50);
            if (response != NULL) {
                if (response->conn_id == -1) {
                    //internal message
                    char *p_charbuf = NULL;
                    bool feat_library;
                    int je = json_scanf(response->data, response->length, "{musicDirectory: %Q, featLibrary: %B}", &p_charbuf, &feat_library);
                    if (je == 2) {
                        FREE_PTR(mg_user_data->music_directory);
                        mg_user_data->music_directory = strdup(p_charbuf);
                        mg_user_data->feat_library = feat_library;
                        FREE_PTR(p_charbuf);
                        
                        if (mg_user_data->rewrite_patterns != NULL) {
                            FREE_PTR(mg_user_data->rewrite_patterns);
                            mg_user_data->rewrite_patterns = NULL;
                        }
                        size_t rewrite_patterns_len = strlen(mg_user_data->pics_directory) + 8;
                        if (feat_library == true && strncmp(mg_user_data->music_directory, "none", 4) != 0) {
                            rewrite_patterns_len += strlen(mg_user_data->music_directory) + 11;
                        }
                        char *rewrite_patterns = malloc(rewrite_patterns_len);
                        assert(rewrite_patterns);
                        if (feat_library == true) {
                            snprintf(rewrite_patterns, rewrite_patterns_len, "/pics/=%s,/library/=%s", mg_user_data->pics_directory, mg_user_data->music_directory);
                            LOG_DEBUG("Setting music_directory to %s", mg_user_data->music_directory);
                        }
                        else {
                            snprintf(rewrite_patterns, rewrite_patterns_len, "/pics/=%s", mg_user_data->pics_directory);
                        }
                        mg_user_data->rewrite_patterns = rewrite_patterns;
                        LOG_DEBUG("Setting rewrite_patterns to %s", mg_user_data->rewrite_patterns);
                    }
                    else {
                        LOG_WARN("Unknown internal message: %s", response->data);
                    }
                    FREE_PTR(response);
                }
                else if (response->conn_id == 0) {
                    //Websocket notify from mpd idle
                    send_ws_notify(mgr, response);
                } 
                else {
                    //api response
                    send_api_response(mgr, response);
                }
            }
        }
        //webserver polling
        mg_mgr_poll(mgr, 50);
    }
    return NULL;
}

//private functions
static int is_websocket(const struct mg_connection *nc) {
    return nc->flags & MG_F_IS_WEBSOCKET;
}

static void send_ws_notify(struct mg_mgr *mgr, t_work_result *response) {
    struct mg_connection *nc;
    for (nc = mg_next(mgr, NULL); nc != NULL; nc = mg_next(mgr, nc)) {
        if (!is_websocket(nc))
            continue;
        if (nc->user_data != NULL) {
            LOG_DEBUG("Sending notify to conn_id %d: %s", (intptr_t)nc->user_data, response->data);
        }
        else {
            LOG_WARN("Sending notify to unknown connection: %s", response->data);
        }
        mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, response->data, response->length);
    }
    FREE_PTR(response);
}

static void send_api_response(struct mg_mgr *mgr, t_work_result *response) {
    struct mg_connection *nc;
    for (nc = mg_next(mgr, NULL); nc != NULL; nc = mg_next(mgr, nc)) {
        if (is_websocket(nc)) {
            continue;
        }
        if (nc->user_data != NULL) {
            if ((intptr_t)nc->user_data == response->conn_id) {
                LOG_DEBUG("Sending response to conn_id %d: %s", (intptr_t)nc->user_data, response->data);
                mg_send_head(nc, 200, response->length, "Content-Type: application/json");
                mg_printf(nc, "%.*s", response->length, response->data);
                break;
            }
        }
        else {
            LOG_DEBUG("Unknown connection");
        }
    }
    FREE_PTR(response);
}

static int has_prefix(const struct mg_str *uri, const struct mg_str *prefix) {
  return uri->len > prefix->len && memcmp(uri->p, prefix->p, prefix->len) == 0;
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    t_mg_user_data *mg_user_data = (t_mg_user_data *) nc->mgr->user_data;
    t_config *config = (t_config *) mg_user_data->config;
    
    switch(ev) {
        case MG_EV_ACCEPT: {
            //increment conn_id
            if (mg_user_data->conn_id < INT_MAX) {
                mg_user_data->conn_id++;
            }
            else {
                mg_user_data->conn_id = 1;
            }
            //set conn_id
            nc->user_data = (void *)(intptr_t)mg_user_data->conn_id;
            LOG_DEBUG("New connection id %d", (intptr_t)nc->user_data);
            break;
        }
        case MG_EV_WEBSOCKET_HANDSHAKE_REQUEST: {
            struct http_message *hm = (struct http_message *) ev_data;
            LOG_VERBOSE("New websocket request (%d): %.*s", (intptr_t)nc->user_data, (int)hm->uri.len, hm->uri.p);
            if (mg_vcmp(&hm->uri, "/ws/") != 0) {
                printf("ERROR: Websocket request not to /ws/, closing connection");
                mg_printf(nc, "%s", "HTTP/1.1 403 FORBIDDEN\r\n\r\n");
                nc->flags |= MG_F_SEND_AND_CLOSE;
            }
            break;
        }
        case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
             LOG_VERBOSE("New Websocket connection established (%d)", (intptr_t)nc->user_data);
             const char *response = "{\"type\": \"welcome\", \"data\": {\"mympdVersion\": \"" MYMPD_VERSION "\"}}";
             mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, response, strlen(response));
             break;
        }
        case MG_EV_HTTP_REQUEST: {
            struct http_message *hm = (struct http_message *) ev_data;
            static const struct mg_str library_prefix = MG_MK_STR("/library");
            static const struct mg_str pics_prefix = MG_MK_STR("/pics");
            LOG_VERBOSE("HTTP request (%d): %.*s", (intptr_t)nc->user_data, (int)hm->uri.len, hm->uri.p);
            if (mg_vcmp(&hm->uri, "/api") == 0) {
                //api request
                bool rc = handle_api((intptr_t)nc->user_data, hm->body.p, hm->body.len);
                if (rc == false) {
                    LOG_ERROR("Invalid API request");
                    const char *response = "{\"type\": \"error\", \"data\": \"Invalid API request\"}";
                    mg_send_head(nc, 200, strlen(response), "Content-Type: application/json");
                    mg_printf(nc, "%s", response);
                }
            }
            else if (mg_vcmp(&hm->uri, "/ca.crt") == 0 && config->custom_cert == false) {
                //deliver ca certificate
                size_t ca_file_len = config->varlibdir_len + 12;
                char ca_file[ca_file_len];
                snprintf(ca_file, ca_file_len, "%s/ssl/ca.pem", config->varlibdir);
                mg_http_serve_file(nc, hm, ca_file, mg_mk_str("application/x-x509-ca-cert"), mg_mk_str(""));
            }
            else if (has_prefix(&hm->uri, &library_prefix) && hm->query_string.len > 0 && mg_vcmp(&hm->query_string, "cover") == 0 
                     && config->plugins_coverextract == true && mg_user_data->feat_library == true) {
                size_t image_file_len = 1500;
                char image_file[image_file_len];
                snprintf(image_file, image_file_len, "%s/assets/coverimage-notavailable.png", DOC_ROOT);

                if (strlen(mg_user_data->music_directory) == 0) {
                    LOG_ERROR("Error extracting coverimage, invalid music_directory");
                    mg_http_serve_file(nc, hm, image_file, mg_mk_str("image/png"), mg_mk_str(""));
                    break;
                }
                //coverextract
                char uri_decoded[(int)hm->uri.len];
                if (mg_url_decode(hm->uri.p, (int)hm->uri.len, uri_decoded, (int)hm->uri.len, 0) == -1) {
                    LOG_ERROR("url_decode buffer to small");
                    mg_http_serve_file(nc, hm, image_file, mg_mk_str("image/png"), mg_mk_str(""));
                    break;
                }
                // replace /library through path to music_directory
                char *uri_trimmed = uri_decoded;
                uri_trimmed += 8;
                size_t media_file_len = strlen(mg_user_data->music_directory) + strlen(uri_trimmed) + 1;
                char media_file[media_file_len];
                snprintf(media_file, media_file_len, "%s%s", mg_user_data->music_directory, uri_trimmed);
                uri_trimmed = NULL;
                
                LOG_VERBOSE("Exctracting coverimage from %s", media_file);
                
                size_t image_mime_type_len = 100;
                char image_mime_type[image_mime_type_len];
                size_t cache_dir_len = config->varlibdir_len + 12;
                char cache_dir[cache_dir_len];
                snprintf(cache_dir, cache_dir_len, "%s/covercache", config->varlibdir);
                bool rc = plugin_coverextract(media_file, cache_dir, image_file, image_file_len, image_mime_type, image_mime_type_len, true);
                if (rc == true) {
                    size_t path_len = config->varlibdir_len + strlen(image_file) + 13;
                    char path[path_len];
                    snprintf(path, path_len, "%s/covercache/%s", config->varlibdir, image_file);
                    LOG_DEBUG("Serving file %s (%s)", path, image_mime_type);
                    mg_http_serve_file(nc, hm, path, mg_mk_str(image_mime_type), mg_mk_str(""));
                }
                else {
                    LOG_ERROR("Error extracting coverimage from %s", media_file);
                    snprintf(image_file, image_file_len, "%s/assets/coverimage-notavailable.png", DOC_ROOT);
                    mg_http_serve_file(nc, hm, image_file, mg_mk_str("image/png"), mg_mk_str(""));
                }
            }
            else {
                static struct mg_serve_http_opts s_http_server_opts;
                s_http_server_opts.document_root = DOC_ROOT;
                if (mg_user_data->rewrite_patterns != NULL) {
                    s_http_server_opts.url_rewrites = mg_user_data->rewrite_patterns;
                }
                if (has_prefix(&hm->uri, &library_prefix) && mg_user_data->feat_library == true) {
                    s_http_server_opts.enable_directory_listing = "yes";
                    s_http_server_opts.extra_headers = "Content-Security-Policy: default-src 'none'; "
                        "style-src 'self' 'unsafe-inline'; font-src 'self'; script-src 'self' 'unsafe-inline'; img-src 'self' data:; "
                        "connect-src 'self' ws: wss:; manifest-src 'self'; "
                        "media-src *; frame-ancestors 'none'; base-uri 'none';\r\n"
                        "X-Content-Type-Options: nosniff\r\n"
                        "X-XSS-Protection: 1; mode=block\r\n"
                        "X-Frame-Options: deny";
                }
                else {
                    s_http_server_opts.enable_directory_listing = "no";
                    s_http_server_opts.extra_headers = "Content-Security-Policy: default-src 'none'; "
                        "style-src 'self'; font-src 'self'; script-src 'self'; img-src 'self' data:; "
                        "connect-src 'self' ws: wss:; manifest-src 'self'; "
                        "media-src *; frame-ancestors 'none'; base-uri 'none';\r\n"
                        "X-Content-Type-Options: nosniff\r\n"
                        "X-XSS-Protection: 1; mode=block\r\n"
                        "X-Frame-Options: deny";
                }
                #ifdef DEBUG
                //serve all files from filesystem
                mg_serve_http(nc, hm, s_http_server_opts);
                #else
                //serve only files in library_prefix and pics_prefix from filesystem
                //if (has_prefix(&hm->uri, &library_prefix) || has_prefix(&hm->uri, &pics_prefix)) {
                    mg_serve_http(nc, hm, s_http_server_opts);
                //}
                //else {
                    //enforce 0-terminated string
                //    int uri_len = (int)hm->uri.len + 1;
                //    char uri[uri_len] = hm->uri.p;
                //    uri[uri_len] = '\0';
                    //Todo: get and send asset
                //}
                #endif
            }
            break;
        }
        case MG_EV_CLOSE: {
            if (nc->user_data != NULL) {
                LOG_VERBOSE("HTTP connection %ld closed", (intptr_t)nc->user_data);
                nc->user_data = NULL;
            }
            break;
        }
        default: {
            break;
        }
    }
}

static void ev_handler_redirect(struct mg_connection *nc, int ev, void *ev_data) {
    switch(ev) {
        case MG_EV_HTTP_REQUEST: {
            struct http_message *hm = (struct http_message *) ev_data;
            struct mg_str *host_hdr = mg_get_http_header(hm, "Host");
            t_mg_user_data *mg_user_data = (t_mg_user_data *) nc->mgr->user_data;
            t_config *config = (t_config *) mg_user_data->config;
            
            size_t host_header_len = (int)host_hdr->len + 1;
            char host_header[host_header_len];
            snprintf(host_header, host_header_len, "%.*s", (int)host_hdr->len, host_hdr->p);
            char *crap = NULL;
            char *host = strtok_r(host_header, ":", &crap);
            size_t s_redirect_len = strlen(host) + strlen(config->ssl_port) + 11;
            char s_redirect[s_redirect_len];
            if (strcmp(config->ssl_port, "443") == 0) {
                snprintf(s_redirect, s_redirect_len, "https://%s/", host);
            }
            else {
                snprintf(s_redirect, s_redirect_len, "https://%s:%s/", host, config->ssl_port);
            }
            LOG_VERBOSE("Redirecting to %s", s_redirect);
            mg_http_send_redirect(nc, 301, mg_mk_str(s_redirect), mg_mk_str(NULL));
            break;
        }
        default: {
            break;
        }
    }
}

static bool handle_api(int conn_id, const char *request_body, int request_len) {
    char *cmd = NULL;
    
    LOG_VERBOSE("API request (%d): %.*s", conn_id, request_len, request_body);
    const int je = json_scanf(request_body, request_len, "{cmd: %Q}", &cmd);
    if (je < 1) {
        return false;
    }

    enum mympd_cmd_ids cmd_id = get_cmd_id(cmd);
    if (cmd_id == 0) {
        return false;
    }
    
    t_work_request *request = (t_work_request*)malloc(sizeof(t_work_request));
    assert(request);
    request->conn_id = conn_id;
    request->cmd_id = cmd_id;
    request->length = copy_string(request->data, request_body, 1000, request_len);
    if (request->length < request_len) {
        LOG_ERROR("Request buffer truncated %d / %d\n", request_len, 1000); 
        free(request);
        return false;
    }
    
    if (strncmp(cmd, "MYMPD_API_", 10) == 0) {
        tiny_queue_push(mympd_api_queue, request);
    }
    else {
        tiny_queue_push(mpd_client_queue, request);
    }

    FREE_PTR(cmd);        
    return true;
}
