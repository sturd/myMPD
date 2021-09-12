/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_BROWSE_H
#define MYMPD_API_BROWSE_H

#include "../lib/mympd_state.h"

sds mympd_api_get_fingerprint(struct t_mympd_state *mympd_state, sds buffer, sds method,
                               long request_id, const char *uri);
sds mympd_api_get_songdetails(struct t_mympd_state *mympd_state, sds buffer, sds method,
                               long request_id, const char *uri);
sds mympd_api_get_filesystem(struct t_mympd_state *mympd_state, sds buffer, 
                              sds method, long request_id, sds path, const unsigned int offset, 
                              const unsigned int limit, sds searchstr,
                              const struct t_tags *tagcols);
sds mympd_api_get_songs_in_album(struct t_mympd_state *mympd_state, sds buffer, sds method,
                                  long request_id, sds album, sds albumartist, 
                                  const struct t_tags *tagcols);
sds mympd_api_get_firstsong_in_albums(struct t_mympd_state *mympd_state, sds buffer, sds method,
                                       long request_id, sds expression, sds sort, bool sortdesc, 
                                       const unsigned int offset, unsigned int limit);
sds mympd_api_get_db_tag(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id, 
                          sds searchstr, sds tag, const unsigned int offset, const unsigned int limit);
sds mympd_api_read_comments(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id, 
                             const char *uri);
#endif
