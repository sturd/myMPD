#ifndef MYMPD_WEB_SERVER_SNAPCAST_H
#define MYMPD_WEB_SERVER_SNAPCAST_H

#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"
#include "src/lib/api.h"

/**
 * Struct for SnapCast backend connections
 */
struct t_snapcast_nc_data {
    struct mg_connection *frontend_nc;  //!< pointer to frontend connection
    sds uri;                            //!< uri to connect the SnapCast connection
    sds body;                           //!< command body to forward to snapcast
    enum mympd_cmd_ids cmd_id;          //!< jsonrpc method of the frontend connection
};

void create_snapcast_connection(struct mg_connection *nc, struct mg_connection *backend_nc,
        sds uri, sds body, mg_event_handler_t fn);
void send_snapcast_request(struct mg_connection *nc, void *fn_data);
void handle_snapcast_close(struct mg_connection *nc, struct t_snapcast_nc_data *backend_nc_data);
void free_snapcast_nc_data(struct t_snapcast_nc_data *data);
void forward_snapcast_to_frontend(struct mg_connection *nc, int ev, void *ev_data, void *fn_data);

#endif // MYMPD_WEB_SERVER_SNAPCAST_H
