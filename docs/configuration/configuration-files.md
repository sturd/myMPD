---
layout: page
permalink: /configuration/configuration-files
title: Configuration files
---

At the first start (if there is no config folder in the working folder) myMPD tries to [autodetect the MPD connection]({{site.baseurl}}/configuration/mpd-connection) and reads some environment variables.

<div class="alert alert-warning">
After the first start all environment variables are ignored, except loglevel.
</div>

To change these settings afterwards, you must edit the files in the folder `/var/lib/mympd/config/` and restart myMPD.


| FILE | TYPE | ENVIRONMENT | DEFAULT | DESCRIPTION |
| ---- | ---- | ----------- | ------- | ----------- |
| acl | string | MYMPD_ACL | | ACL to access the myMPD webserver: [ACL]({{ site.baseurl }}/configuration/acl), allows all hosts in the default configuration |
| covercache_keep_days | number | MYMPD_COVERCACHE_KEEP_DAYS | 31 | How long to keep images in the covercache, 0 to disable the cache |
| http_host | string | MYMPD_HTTP_HOST | 0.0.0.0 | IP address to listen on, use [::] to listen on IPv6 |
| http_port | number | MYMPD_HTTP_PORT | 80 | Port to listen on. Redirects to `ssl_port` if `ssl` is set to `true` |
| loglevel | number | MYMPD_LOGLEVEL | 5 | [Logging]({{ site.baseurl }}/configuration/logging) - this environment variable is always used |
| lualibs | string | MYMPD_LUALIBS | all | Comma separated list of lua libraries to load, look at [Scripting - LUA standard libraries]({{ site.baseurl }}/scripting#lua-standard-libraries) |
| scriptacl | string | MYMPD_SCRIPTACL | +127.0.0.1 | ACL to access the myMPD script backend: [ACL]({{ site.baseurl }}/configuration/acl), allows only local connections in the default configuration. The acl above must also grant access. |
| ssl | boolean | MYMPD_SSL | true | `true` = enables https, `false` = disables https |
| ssl_port | number | MYMPD_SSL_PORT | 443 | Port to listen to https traffic |
| ssl_san | string | MYMPD_SSL_SAN | | Additional SAN for certificate creation |
| custom_cert | boolean | MYMPD_CUSTOM_CERT | false | `true` = use custom ssl key and certificate |
| ssl_cert | string | MYMPD_SSL_CERT | | Path to custom ssl certificate file |
| ssl_key | string | MYMPD_SSL_KEY | | Path to custom ssl key file |
| pin_hash | string | N/A | | SHA256 hash of pin, create it with `mympd -p` |
{: .table .table-sm }

- More details on [SSL]({{ site.baseurl }}/configuration/ssl)

You can use `mympd -c` to create the initial configuration in the `/var/lib/mympd/config/` directory.