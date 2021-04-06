/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define _GNU_SOURCE

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <inttypes.h>
#include <unistd.h>
#include <time.h>

#include "../dist/src/sds/sds.h"

#include "sds_extras.h"
#include "list.h"
#include "mympd_config_defs.h"
#include "state_files.h"
#include "utility.h"
#include "log.h"
#include "mympd_config.h"

//private functions
static const char *mympd_getenv(const char *env_var, bool first_startup);
static sds mympd_getenv_string(const char *env_var, const char *default_value, bool first_startup);
static bool mympd_getenv_bool(const char *env_var, bool default_value, bool first_startup);
static int mympd_getenv_int(const char *env_var, int default_value, bool first_startup);

//public functions
void mympd_free_config(struct t_config *config) {
    sdsfree(config->http_host);
    sdsfree(config->http_port);
#ifdef ENABLE_SSL
    sdsfree(config->ssl_port);
    sdsfree(config->ssl_cert);
    sdsfree(config->ssl_key);
    sdsfree(config->ssl_san);
#endif
    sdsfree(config->user);
    sdsfree(config->workdir);
    sdsfree(config->acl);
    sdsfree(config->scriptacl);
    sdsfree(config->lualibs);
}

void mympd_free_config_initial(struct t_config *config) {
    sdsfree(config->user);
    sdsfree(config->workdir);
}

void mympd_config_defaults(struct t_config *config) {
    //configureable with environment variables at first startup
    config->http_host = mympd_getenv_string("MYMPD_HTTP_HOST", "0.0.0.0", config->first_startup);
    config->http_port = mympd_getenv_string("MYMPD_HTTP_PORT", "80", config->first_startup);
    #ifdef ENABLE_SSL
    config->ssl = mympd_getenv_bool("MYMPD_SSL", true, config->first_startup);
    config->ssl_port = mympd_getenv_string("MYMPD_SSL_PORT", "443", config->first_startup);
    config->ssl_cert = mympd_getenv_string("MYMPD_SSL_CERT", VARLIB_PATH"/ssl/server.pem", config->first_startup);
    config->ssl_key = mympd_getenv_string("MYMPD_SSL_KEY", VARLIB_PATH"/ssl/server.key", config->first_startup);
    config->ssl_san = mympd_getenv_string("MYMPD_SSL_SAN", "", config->first_startup); 
    config->custom_cert = mympd_getenv_bool("MYMPD_CUSTOM_CERT", false, config->first_startup);
    #endif
    config->acl = mympd_getenv_string("MYMPD_ACL", "", config->first_startup);
    config->scriptacl = mympd_getenv_string("MYMPD_SCRIPTACL", "+127.0.0.0/8", config->first_startup);
    #ifdef ENABLE_LUA
    config->lualibs = mympd_getenv_string("MYMPD_LUALIBS", "all", config->first_startup);
    #endif
    
    config->covercache = mympd_getenv_bool("MYMPD_COVERCACHE", true, config->first_startup);
    config->covercache_keep_days = mympd_getenv_int("MYMPD_COVERCACHE_KEEP_DAYS", 14, config->first_startup);
}

void mympd_config_defaults_initial(struct t_config *config) {
    //command line options
    config->user = sdsnew("mympd");
    config->workdir = sdsnew(VARLIB_PATH);
    config->syslog = false;
    //not configureable
    config->startup_time = time(NULL);
    config->first_startup = false;
}

bool mympd_read_config(struct t_config *config) {
    config->http_host = state_file_rw_string(config, "http_host", config->http_host, false);
    config->http_port = state_file_rw_string(config, "http_port", config->http_port, false);
    #ifdef ENABLE_SSL
    config->ssl = state_file_rw_bool(config, "ssl", config->ssl, false);
    config->ssl_port = state_file_rw_string(config, "ssl_port", config->ssl_port, false);
    config->ssl_cert = state_file_rw_string(config, "ssl_cert", config->ssl_cert, false);
    config->ssl_key = state_file_rw_string(config, "ssl_key", config->ssl_key, false);
    config->ssl_san = state_file_rw_string(config, "ssl_san", config->ssl_san, false);
    config->custom_cert = state_file_rw_bool(config, "custom_cert", config->custom_cert, false);
    #endif
    config->acl = state_file_rw_string(config, "acl", config->acl, false);
    config->scriptacl = state_file_rw_string(config, "scriptacl", config->scriptacl, false);
    config->lualibs = state_file_rw_string(config, "lualibs", config->lualibs, false);
    config->covercache = state_file_rw_bool(config, "covercache", config->covercache, false);
    config->covercache_keep_days = state_file_rw_int(config, "covercache_keep_days", config->covercache_keep_days, false);
    
    //set correct path to certificate/key, if workdir is non default and cert paths are default
    #ifdef ENABLE_SSL
    if (strcmp(config->workdir, VARLIB_PATH) != 0 && config->custom_cert == false) {
        config->ssl_cert = sdscrop(config->ssl_cert);
        config->ssl_cert = sdscatfmt(config->ssl_cert, "%s/ssl/server.pem", config->workdir);
        config->ssl_key = sdscrop(config->ssl_key);
        config->ssl_key = sdscatfmt(config->ssl_key, "%s/ssl/server.key", config->workdir);
    }
    #endif
    return true;
}

//private functions
static const char *mympd_getenv(const char *env_var, bool first_startup) {
    const char *env_value = getenv(env_var);
    if (first_startup == true && env_value != NULL) {
        MYMPD_LOG_INFO("Using environment variable \"%s\" with value \"%s\"", env_var, env_value);
        return env_var;
    }
    if (first_startup == false && env_value != NULL) {
        MYMPD_LOG_INFO("Ignoring environment variable \"%s\" with value \"%s\"", env_var, env_value);
    }
    return NULL;
}

static sds mympd_getenv_string(const char *env_var, const char *default_value, bool first_startup) {
    const char *env_value = mympd_getenv(env_var, first_startup);
    return env_value != NULL ? sdsnew(env_value) : sdsnew(default_value);
}

static bool mympd_getenv_bool(const char *env_var, bool default_value, bool first_startup) {
    const char *env_value = mympd_getenv(env_var, first_startup);
    return env_value != NULL ? strcmp(env_value, "true") == 0 ? true : false 
                             : default_value;
}

static int mympd_getenv_int(const char *env_var, int default_value, bool first_startup) {
    const char *env_value = mympd_getenv(env_var, first_startup);
    return env_value != NULL ? (int)strtoimax(env_var, NULL, 10) : default_value;
}
