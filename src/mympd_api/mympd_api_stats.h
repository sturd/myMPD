/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_STATS_H
#define MYMPD_API_STATS_H

#include "../lib/mympd_state.h"

bool mympd_api_add_song_to_last_played_list(struct t_mympd_state *mympd_state, const int song_id);
bool mympd_api_last_played_list_save(struct t_mympd_state *mympd_state);
sds mympd_api_get_last_played_songs(struct t_mympd_state *mympd_state, sds buffer, sds method, 
                                     long request_id, const unsigned int offset, 
                                     const unsigned int limit, const struct t_tags *tagcols);
sds mympd_api_get_stats(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
#endif
