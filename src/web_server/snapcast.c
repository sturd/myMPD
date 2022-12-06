#include "compile_time.h"
#include "src/web_server/snapcast.h"

#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/web_server/proxy.h"
#include "src/web_server/utility.h"

/**
 * Send the request to SnapCast and
 * forwards the raw data from SnapCast response to frontend connection
 * @param nc mongoose backend connection
 * @param ev mongoose event
 * @param ev_data mongoose ev_data (not used)
 * @param fn_data mongoose fn_data (t_snapcast_nc_data)
 */
void forward_snapcast_to_frontend(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    (void) ev_data;
    struct t_snapcast_nc_data *snapcast_nc_data = (struct t_snapcast_nc_data *)fn_data;
    switch(ev) {
        case MG_EV_CONNECT: {
            send_snapcast_request(nc, fn_data);
            break;
        }
        case MG_EV_ERROR:
            MYMPD_LOG_ERROR("HTTP connection \"%lu\" failed", nc->id);
            break;
        case MG_EV_READ:
            if (snapcast_nc_data->frontend_nc != NULL) {
                mg_send(snapcast_nc_data->frontend_nc, nc->recv.buf, nc->recv.len);
            }
            mg_iobuf_del(&nc->recv, 0, nc->recv.len);
            break;
        case MG_EV_CLOSE: {
            handle_snapcast_close(nc, snapcast_nc_data);
            break;
        }
    }
}

/**
 * Sends the request to the SnapCast connection
 * @param nc mongoose backend connection
 * @param fn_data mongoose fn_data pointer
 */
void send_snapcast_request(struct mg_connection *nc, void *fn_data) {
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    struct t_snapcast_nc_data *snapcast_nc_data = (struct t_snapcast_nc_data *)fn_data;
    mg_user_data->connection_count++;
    struct mg_str host = mg_url_host(snapcast_nc_data->uri);
    struct mg_str body = mg_str(snapcast_nc_data->body);
    MYMPD_LOG_INFO("SnapCast connection \"%lu\" established, host \"%.*s\"", nc->id, (int)host.len, host.ptr);
    if (mg_url_is_ssl(snapcast_nc_data->uri)) {
        struct mg_tls_opts tls_opts = {
            .srvname = host
        };
        mg_tls_init(nc, &tls_opts);
    }

    mg_printf(nc, "POST %s HTTP/1.1\r\n"
        "Host: %.*s\r\n"
        "User-Agent: myMPD/"MYMPD_VERSION"\r\n"
        "Connection: close\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        mg_url_uri(snapcast_nc_data->uri),
        (int)host.len, host.ptr,
        (int)body.len, body.ptr);
    MYMPD_LOG_DEBUG("Sending POST %s HTTP/1.1 to SnapCast connection \"%lu\"", mg_url_uri(snapcast_nc_data->uri), nc->id);
}

/**
 * Create the connection to with the SnapCast server
 * @param nc mongoose connection
 * @param backend_nc mongoose backend connection
 * @param uri uri of the SnapCast HTTP RPC endpoint
 * @param body connection event handler
 */
void create_snapcast_connection(struct mg_connection *nc, struct mg_connection *backend_nc, sds uri, sds body, mg_event_handler_t fn) {
    if (backend_nc == NULL) {
        MYMPD_LOG_INFO("Creating new http SnapCast connection to \"%s\"", uri);
        struct t_snapcast_nc_data *snapcast_nc_data = malloc(sizeof(struct t_snapcast_nc_data));
        snapcast_nc_data->uri = sdsdup(uri);
        snapcast_nc_data->body = sdsdup(body);
        snapcast_nc_data->frontend_nc = nc;
        backend_nc = mg_http_connect(nc->mgr, uri, fn, snapcast_nc_data);
        if (backend_nc == NULL) {
            //no backend connection, close frontend connection
            MYMPD_LOG_WARN("Can not create http SnapCast connection");
            webserver_send_error(nc, 502, "Could not create SnapCast connection");
            nc->is_draining = 1;
            //free snapcast_nc_data
            free_snapcast_nc_data(snapcast_nc_data);
            FREE_PTR(snapcast_nc_data);
        }
        else {
            //save backend connection pointer in frontend connection fn_data
            struct t_frontend_nc_data *frontend_nc_data = (struct t_frontend_nc_data *)nc->fn_data;
            frontend_nc_data->backend_nc = backend_nc;
        }
    }
}

/**
 * Handles the connection close on SnapCast side
 * @param nc mongoose connection
 * @param snapcast_nc_data backend data
 */
void handle_snapcast_close(struct mg_connection *nc, struct t_snapcast_nc_data *snapcast_nc_data) {
    MYMPD_LOG_INFO("SnapCast tcp connection \"%lu\" closed", nc->id);
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    mg_user_data->connection_count--;
    if (snapcast_nc_data->frontend_nc != NULL) {
        //remove backend connection pointer from frontend connection
        struct t_frontend_nc_data *frontend_nc_data = (struct t_frontend_nc_data *)snapcast_nc_data->frontend_nc->fn_data;
        frontend_nc_data->backend_nc = NULL;
        //close frontend connection
        snapcast_nc_data->frontend_nc->is_draining = 1;
    }
    //free snapcast_nc_data
    free_snapcast_nc_data(snapcast_nc_data);
    FREE_PTR(nc->fn_data);
}

/**
 * Frees the SnapCast data struct
 * @param data SnapCast data to free
 */
void free_snapcast_nc_data(struct t_snapcast_nc_data *data) {
    FREE_SDS(data->uri);
    data->frontend_nc = NULL;
}
