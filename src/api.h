/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __API_H__
#define __API_H__

//API cmds
#define MYMPD_CMDS(X) \
    X(MYMPD_API_UNKNOWN) \
    X(MYMPD_API_QUEUE_CROP_OR_CLEAR) \
    X(MYMPD_API_QUEUE_CLEAR) \
    X(MYMPD_API_QUEUE_CROP) \
    X(MYMPD_API_QUEUE_SAVE) \
    X(MYMPD_API_QUEUE_LIST) \
    X(MYMPD_API_QUEUE_SEARCH) \
    X(MYMPD_API_QUEUE_RM_SONG) \
    X(MYMPD_API_QUEUE_RM_RANGE) \
    X(MYMPD_API_QUEUE_MOVE_SONG) \
    X(MYMPD_API_QUEUE_ADD_URI_AFTER) \
    X(MYMPD_API_QUEUE_ADD_URI) \
    X(MYMPD_API_QUEUE_ADD_PLAY_URI) \
    X(MYMPD_API_QUEUE_REPLACE_URI) \
    X(MYMPD_API_QUEUE_ADD_PLAYLIST) \
    X(MYMPD_API_QUEUE_REPLACE_PLAYLIST) \
    X(MYMPD_API_QUEUE_SHUFFLE) \
    X(MYMPD_API_QUEUE_LAST_PLAYED) \
    X(MYMPD_API_QUEUE_ADD_RANDOM) \
    X(MYMPD_API_QUEUE_PRIO_SET_HIGHEST) \
    X(MYMPD_API_PLAYLIST_CLEAR) \
    X(MYMPD_API_PLAYLIST_RENAME) \
    X(MYMPD_API_PLAYLIST_MOVE_SONG) \
    X(MYMPD_API_PLAYLIST_ADD_URI) \
    X(MYMPD_API_PLAYLIST_RM_SONG) \
    X(MYMPD_API_PLAYLIST_RM_ALL) \
    X(MYMPD_API_PLAYLIST_RM) \
    X(MYMPD_API_PLAYLIST_LIST) \
    X(MYMPD_API_PLAYLIST_CONTENT_LIST) \
    X(MYMPD_API_PLAYLIST_SHUFFLE) \
    X(MYMPD_API_PLAYLIST_SORT) \
    X(MYMPD_API_STICKERCACHE_CREATED) \
    X(MYMPD_API_ALBUMCACHE_CREATED) \
    X(MYMPD_API_SMARTPLS_STICKER_SAVE) \
    X(MYMPD_API_SMARTPLS_NEWEST_SAVE) \
    X(MYMPD_API_SMARTPLS_SEARCH_SAVE) \
    X(MYMPD_API_SMARTPLS_GET) \
    X(MYMPD_API_SMARTPLS_UPDATE_ALL) \
    X(MYMPD_API_SMARTPLS_UPDATE) \
    X(MYMPD_API_DATABASE_SEARCH_ADV) \
    X(MYMPD_API_DATABASE_SEARCH) \
    X(MYMPD_API_DATABASE_UPDATE) \
    X(MYMPD_API_DATABASE_RESCAN) \
    X(MYMPD_API_DATABASE_FILESYSTEM_LIST) \
    X(MYMPD_API_DATABASE_TAG_LIST) \
    X(MYMPD_API_DATABASE_TAG_ALBUM_TITLE_LIST) \
    X(MYMPD_API_DATABASE_STATS) \
    X(MYMPD_API_DATABASE_SONGDETAILS) \
    X(MYMPD_API_DATABASE_COMMENTS) \
    X(MYMPD_API_DATABASE_FINGERPRINT) \
    X(MYMPD_API_DATABASE_GET_ALBUMS) \
    X(MYMPD_API_PLAYER_PLAY_SONG) \
    X(MYMPD_API_PLAYER_VOLUME_SET) \
    X(MYMPD_API_PLAYER_VOLUME_GET) \
    X(MYMPD_API_PLAYER_PAUSE) \
    X(MYMPD_API_PLAYER_RESUME) \
    X(MYMPD_API_PLAYER_PLAY) \
    X(MYMPD_API_PLAYER_STOP) \
    X(MYMPD_API_PLAYER_SEEK_CURRENT) \
    X(MYMPD_API_PLAYER_NEXT) \
    X(MYMPD_API_PLAYER_PREV) \
    X(MYMPD_API_PLAYER_OUTPUT_LIST) \
    X(MYMPD_API_PLAYER_OUTPUT_ATTRIBUTS_SET) \
    X(MYMPD_API_PLAYER_TOGGLE_OUTPUT) \
    X(MYMPD_API_PLAYER_CURRENT_SONG) \
    X(MYMPD_API_PLAYER_STATE) \
    X(MYMPD_API_PLAYER_OPTIONS_SET) \
    X(MYMPD_API_LIKE) \
    X(MYMPD_API_MESSAGE_SEND) \
    X(MYMPD_API_URLHANDLERS) \
    X(MYMPD_API_ALBUMART) \
    X(MYMPD_API_TIMER_STARTPLAY) \
    X(MYMPD_API_MOUNT_LIST) \
    X(MYMPD_API_MOUNT_MOUNT) \
    X(MYMPD_API_MOUNT_UNMOUNT) \
    X(MYMPD_API_MOUNT_NEIGHBOR_LIST) \
    X(MYMPD_API_PARTITION_LIST) \
    X(MYMPD_API_PARTITION_NEW) \
    X(MYMPD_API_PARTITION_SWITCH) \
    X(MYMPD_API_PARTITION_RM) \
    X(MYMPD_API_PARTITION_OUTPUT_MOVE) \
    X(MYMPD_API_TRIGGER_LIST) \
    X(MYMPD_API_TRIGGER_GET) \
    X(MYMPD_API_TRIGGER_SAVE) \
    X(MYMPD_API_TRIGGER_DELETE) \
    X(MYMPD_API_JUKEBOX_LIST) \
    X(MYMPD_API_JUKEBOX_RM) \
    X(MYMPD_API_STATE_SAVE) \
    X(MYMPD_API_LYRICS_GET) \
    X(MYMPD_API_COLS_SAVE) \
    X(MYMPD_API_SETTINGS_GET) \
    X(MYMPD_API_SETTINGS_SET) \
    X(MYMPD_API_CONNECTION_SAVE) \
    X(MYMPD_API_COVERCACHE_CROP) \
    X(MYMPD_API_COVERCACHE_CLEAR) \
    X(MYMPD_API_TIMER_SET) \
    X(MYMPD_API_TIMER_SAVE) \
    X(MYMPD_API_TIMER_LIST) \
    X(MYMPD_API_TIMER_GET) \
    X(MYMPD_API_TIMER_RM) \
    X(MYMPD_API_TIMER_TOGGLE) \
    X(MYMPD_API_SCRIPT_INIT) \
    X(MYMPD_API_SCRIPT_EXECUTE) \
    X(MYMPD_API_SCRIPT_POST_EXECUTE) \
    X(MYMPD_API_SCRIPT_LIST) \
    X(MYMPD_API_SCRIPT_GET) \
    X(MYMPD_API_SCRIPT_SAVE) \
    X(MYMPD_API_SCRIPT_DELETE) \
    X(MYMPD_API_HOME_LIST) \
    X(MYMPD_API_HOME_ICON_GET) \
    X(MYMPD_API_HOME_ICON_SAVE) \
    X(MYMPD_API_HOME_ICON_DELETE) \
    X(MYMPD_API_HOME_ICON_MOVE) \
    X(MYMPD_API_PICTURE_LIST) \
    X(MYMPD_API_CACHES_CREATE)

#define GEN_ENUM(X) X,
#define GEN_STR(X) #X,

//global enums
enum mympd_cmd_ids {
    MYMPD_CMDS(GEN_ENUM)
};

//global functions
enum mympd_cmd_ids get_cmd_id(const char *cmd);
bool is_public_api_method(enum mympd_cmd_ids cmd_id);
bool is_mympd_only_api_method(enum mympd_cmd_ids cmd_id);
#endif
