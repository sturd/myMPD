/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <limits.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <libgen.h>

#include "../dist/src/sds/sds.h"
#include "../dist/src/mongoose/mongoose.h"
#include "../dist/src/frozen/frozen.h"
#include "../sds_extras.h"
#include "../api.h"
#include "../utility.h"
#include "../log.h"
#include "../list.h"
#include "../tiny_queue.h"
#include "config_defs.h"
#include "../global.h"
#include "../plugins.h"
#include "web_server_utility.h"
#include "web_server_albumart.h"

//privat definitions
static bool handle_coverextract(struct mg_connection *nc, struct http_message *hm, t_config *config, sds media_file);

//public functions
void send_albumart(struct mg_connection *nc, struct http_message *hm, sds data, sds binary) {
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    char *p_charbuf3 = NULL;
    int je = json_scanf(data, sdslen(data), "{result: {coverfile:%Q, coverfile_name:%Q, mime_type:%Q}}", 
        &p_charbuf1, &p_charbuf2, &p_charbuf3);
    if (je == 3) {
        if (strcmp(p_charbuf1, "binary") == 0) {
            sds header = sdscatfmt(sdsempty(), "Content-Type: %s", p_charbuf3);
            mg_send_head(nc, 200, sdslen(binary), header);
            mg_send(nc, binary, sdslen(binary));
            sdsfree(header);
        }
        else {
            mg_http_serve_file(nc, hm, p_charbuf2, mg_mk_str(p_charbuf3), mg_mk_str(""));
        }
    }
    else {
        serve_na_image(nc, hm);
    }
    FREE_PTR(p_charbuf1);
    FREE_PTR(p_charbuf2);
    FREE_PTR(p_charbuf3);
}

//returns true if an image is served
//returns false if waiting for mpd_client to handle request
bool handle_albumart(struct mg_connection *nc, struct http_message *hm, t_mg_user_data *mg_user_data, t_config *config, int conn_id) {
    //decode uri
    sds uri_decoded = sdsurldecode(sdsempty(), hm->uri.p, (int)hm->uri.len, 0);
    if (sdslen(uri_decoded) == 0) {
        LOG_ERROR("Failed to decode uri");
        serve_na_image(nc, hm);
        sdsfree(uri_decoded);
        return true;
    }
    if (validate_uri(uri_decoded) == false) {
        LOG_ERROR("Invalid uri: %s", uri_decoded);
        serve_na_image(nc, hm);
        sdsfree(uri_decoded);
        return true;
    }
    //try image in /pics folder, if uri contains ://
    if (strstr(uri_decoded, "://") != NULL) {
        char *name = strstr(uri_decoded, "://");
        if (strlen(name) < 4) {
            LOG_ERROR("Uri to short");
            serve_na_image(nc, hm);
            sdsfree(uri_decoded);
            return true;
        }
        name += 3;
        uri_to_filename(name);
        sds coverfile = sdscatfmt(sdsempty(), "%s/pics/%s", config->varlibdir, name);
        LOG_DEBUG("Check for stream cover %s", coverfile);
        coverfile = find_image_file(coverfile);
        
        if (sdslen(coverfile) > 0) {
            sds mime_type = get_mime_type_by_ext(coverfile);
            LOG_DEBUG("Serving file %s (%s)", coverfile, mime_type);
            mg_http_serve_file(nc, hm, coverfile, mg_mk_str(mime_type), mg_mk_str(""));
            sdsfree(mime_type);
        }
        else {
            serve_stream_image(nc, hm);
        }
        sdsfree(coverfile);
        sdsfree(uri_decoded);
        return true;
    }
    //remove /albumart/
    sdsrange(uri_decoded, 10, -1);
    //create absolute file
    sds mediafile = sdscatfmt(sdsempty(), "%s/%s", mg_user_data->music_directory, uri_decoded);
    LOG_DEBUG("Absolut media_file: %s", mediafile);
    //check covercache
    if (config->covercache == true) {
        sds filename = sdsdup(uri_decoded);
        uri_to_filename(filename);
        sds covercachefile = sdscatfmt(sdsempty(), "%s/covercache/%s", config->varlibdir, filename);
        sdsfree(filename);
        covercachefile = find_image_file(covercachefile);
        if (sdslen(covercachefile) > 0) {
            sds mime_type = get_mime_type_by_ext(covercachefile);
            LOG_DEBUG("Serving file %s (%s)", covercachefile, mime_type);
            mg_http_serve_file(nc, hm, covercachefile, mg_mk_str(mime_type), mg_mk_str(""));
            sdsfree(uri_decoded);
            sdsfree(covercachefile);
            sdsfree(mediafile);
            sdsfree(mime_type);
            return true;
        }
        else {
            sdsfree(covercachefile);
        }
    }
    //check music_directory folder
    if (mg_user_data->feat_library == true && access(mediafile, F_OK) == 0) {
        //try image in folder under music_directory
        char *path = uri_decoded;
        dirname(path);
        sds coverfile = sdscatfmt(sdsempty(), "%s/%s/%s", mg_user_data->music_directory, path, mg_user_data->coverimage_name);
        LOG_DEBUG("Check for cover %s", coverfile);
        if (access(coverfile, F_OK ) == 0) { /* Flawfinder: ignore */
            //todo: get mime_type
            LOG_DEBUG("Serving file %s (%s)", coverfile, "image/jpeg");
            mg_http_serve_file(nc, hm, coverfile, mg_mk_str("image/jpeg"), mg_mk_str(""));
            sdsfree(uri_decoded);
            sdsfree(coverfile);
            sdsfree(mediafile);
            return true;
        }
        sdsfree(coverfile);
        //try coverextract plugin
        if (config->plugins_coverextract == true) {
            LOG_DEBUG("Exctracting coverimage from %s/%s", mg_user_data->music_directory, mediafile);
            bool rc = handle_coverextract(nc, hm, config, uri_decoded);
            if (rc == true) {
                sdsfree(uri_decoded);
                sdsfree(mediafile);
                return true;
            }
        }
    }
    //ask mpd
    #ifdef EMBEDDED_LIBMPDCLIENT
    else if (mg_user_data->feat_library == false && mg_user_data->feat_mpd_albumart == true) {
        LOG_DEBUG("Sending getalbumart to mpd_client_queue");
        t_work_request *request = create_request(conn_id, 0, MPD_API_ALBUMART, "MPD_API_ALBUMART", hm, "");
        request->data = sdscat(request->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MPD_API_ALBUMART\",\"params\":{");
        request->data = tojson_char(request->data, "uri", uri_decoded, false);
        request->data = sdscat(request->data, "}}");

        tiny_queue_push(mpd_client_queue, request);
        sdsfree(mediafile);
        sdsfree(uri_decoded);
        return false;
    }
    #else
    (void) conn_id;
    #endif

    LOG_VERBOSE("No coverimage found for %s", mediafile);
    sdsfree(mediafile);
    sdsfree(uri_decoded);
    serve_na_image(nc, hm);
    return true;
}

//privat functions

static bool handle_coverextract(struct mg_connection *nc, struct http_message *hm, t_config *config, sds media_file) {
    size_t image_mime_type_len = 100;
    char image_mime_type[image_mime_type_len]; /* Flawfinder: ignore */
    size_t image_file_len = 1500;
    char image_file[image_file_len]; /* Flawfinder: ignore */
    
    sds cache_dir = sdscatfmt(sdsempty(), "%s/covercache", config->varlibdir);

    bool rc = plugin_coverextract(media_file, cache_dir, image_file, image_file_len, image_mime_type, image_mime_type_len, true);
    sdsfree(cache_dir);
    if (rc == true) {
        sds path = sdscatfmt(sdsempty(), "%s/covercache/%s", config->varlibdir, image_file);
        LOG_DEBUG("Serving file %s (%s)", path, image_mime_type);
        mg_http_serve_file(nc, hm, path, mg_mk_str(image_mime_type), mg_mk_str(""));
        sdsfree(path);
    }
    return rc;
}
