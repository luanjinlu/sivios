/*
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 *   luanjinlu@126.com
 */

#include "siviosapi.h"

#define MAX_FP_STR_LEN   MAX_FP_LEN*sizeof(uint64_t)*2
static int dist = 10;
static int limit = 3;
static int mode = 1;
static int enc = 1;
static findb_t db;
static struct evhttp *server = NULL;

static void quit(int sig) {
    int ret;
    ret = findb_close(&db);
    if (ret == -1) {
        syslog(LOG_ERR, "findb_close error when quiting.");
    }
    syslog(LOG_INFO, "Quiting the service.");
    closelog();
    exit(0);
}

static void daemonize(void) {
    int     fd0, fd1, fd2;
    pid_t   pid;

    /* Clear file creation mask */
    umask(0);

    /* Become a session leader to lose controlling TTY */
    if ((pid = fork()) < 0) {
        fprintf(stderr, "Cannot fork!\n");
        exit(1);
    } else if (pid != 0) {
        /* The parent process */
        exit(0);
    }
    setsid();

    /* Make sure that future opens will not allocate controlling TTYs */
    signal(SIGHUP, SIG_IGN);
    if ((pid = fork()) < 0) {
        fprintf(stderr, "Cannot fork!\n");
        exit(1);
    } else if (pid != 0) {
        /* The parent process */
        exit(0);
    }
    /*-
     * Change the current working directory to the root so that
     * the daemon program can be run indepently in case of the
     * file system was unmounted.
     */
#if 0
    if (chdir("/") < 0) {
        fprintf(stderr, "Cannot change directory to /\n");
        exit(1);
    }
#endif
    
    /* Attach file descriptors 0, 1, and 2 to /dev/null */
    close(0);
    close(1);
    close(2);
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);
    return;
}

static void do_add_core(struct evhttp_request *req, int uniq) {
    struct evbuffer *answer = evbuffer_new();
    size_t len = EVBUFFER_LENGTH(req->input_buffer);
    char *buf = (char *)malloc(len+1);
    if (!buf) {
        evbuffer_add_printf(answer, "Sorry\n");
        evhttp_send_reply(req, HTTP_NOTFOUND, "Internal error", answer);
        syslog(LOG_ERR, "Internal error with malloc buf");
        evbuffer_free(answer);
        return;
    }
    memset(buf, 0, len+1);
    memcpy(buf, EVBUFFER_DATA(req->input_buffer), len);

    /* now begin processing */
    uint64_t key;
    int ret;
    sscanf(buf, "%lld", &key);
    /* next get the fingerprint */
    char *url;
    url = buf;
    while (*url != ' ' && url < buf+len) url++;
    if (url == buf) {
        while (*url == ' ' && url < buf+len) url++;
        while (*url != ' ' && url < buf+len) url++;
    }
    while (*url == ' ' && url < buf+len) url++;
    /* now url pointing to the first char of fingerprint */
    uint64_t val[MAX_FP_LEN];
    int vlen, urllen;

    urllen = strlen(url);
    if (urllen <= MAX_FP_STR_LEN) {
        vlen = mystr2lls(url, val, enc);
        if (vlen == -1 && enc) {
            vlen = mystr2lls(url, val, 0);
        }
    } else {
        char valstr[MAX_FP_STR_LEN+1];
        strncpy(valstr, url, MAX_FP_STR_LEN);
        valstr[MAX_FP_STR_LEN] = '\0';
        vlen = mystr2lls(valstr, val, enc);
        if (vlen == -1 && enc) {
            vlen = mystr2lls(valstr, val, 0);
        }
    }

    if (vlen == -1) {
        /* wrong fingerprint */
        evbuffer_add_printf(answer, "%lld has wrong fingperprint!\n", key);
        evhttp_send_reply(req, HTTP_NOTFOUND, "wrong fingerprint", answer);
        evbuffer_free(answer);
        free(buf);
        return;
    }
    /* now add this fingerprint */
    if (uniq) {
        ret = findb_add_uniq(&db, key, val, vlen);
    } else {
        ret = findb_add(&db, key, val, vlen);
    }
    if (ret == -1) {
        /* add fail */
        evbuffer_add_printf(answer, "%lld add fail.\n", key);
        evhttp_send_reply(req, HTTP_NOTFOUND, "add fail", answer);
        evbuffer_free(answer);
        free(buf);
        return;
    }
    /* finally, success. */
    evbuffer_add_printf(answer, "%lld OK\n", key);
    evhttp_send_reply(req, HTTP_OK, "OK", answer);
    evbuffer_free(answer);
    free(buf);
    return;
}

static void do_add(struct evhttp_request *req, void *arg) {
    do_add_core(req, 0);
}

static void do_Add(struct evhttp_request *req, void *arg) {
    do_add_core(req, 1);
}

static void do_del(struct evhttp_request *req, void *arg) {
    struct evbuffer *answer = evbuffer_new();
    size_t len = EVBUFFER_LENGTH(req->input_buffer);
    char *buf = (char *)malloc(len+1);
    if (!buf) {
        evbuffer_add_printf(answer, "Sorry\n");
        evhttp_send_reply(req, HTTP_NOTFOUND, "Internal error", answer);
        syslog(LOG_ERR, "Internal error with malloc buf");
        evbuffer_free(answer);
        return;
    }
    memset(buf, 0, len+1);
    memcpy(buf, EVBUFFER_DATA(req->input_buffer), len);
    /* begin processing del */
    uint64_t key;
    int ret;
    sscanf(buf, "%lld", &key);
    ret = findb_del(&db, key);
    if (ret == -1) {
        /* not found */
        evbuffer_add_printf(answer, "%lld 0\n", key);
        evhttp_send_reply(req, HTTP_NOTFOUND, "not found", answer);
    } else {
        /* del success */
        evbuffer_add_printf(answer, "%lld 1\n", key);
        evhttp_send_reply(req, HTTP_OK, "OK", answer);
    }
    evbuffer_free(answer);
    free(buf);
    return;
}

static void do_query(struct evhttp_request *req, void *arg) {
    struct evbuffer *answer = evbuffer_new();
    size_t len = EVBUFFER_LENGTH(req->input_buffer);
    char *buf = (char *)malloc(len+1);
    if (!buf) {
        evbuffer_add_printf(answer, "Sorry\n");
        evhttp_send_reply(req, HTTP_NOTFOUND, "Internal error", answer);
        syslog(LOG_ERR, "Internal error with malloc buf");
        evbuffer_free(answer);
        return;
    }
    memset(buf, 0, len+1);
    memcpy(buf, EVBUFFER_DATA(req->input_buffer), len);
    /* begin processing query */
    uint64_t key;
    int ret;
    uint64_t *val;
    int vlen;
    sscanf(buf, "%lld", &key);
    ret = findb_query(&db, key, &val, &vlen);
    if (ret == -1) {
        /* not found */
        evbuffer_add_printf(answer, "%lld not found.\n", key);
        evhttp_send_reply(req, HTTP_NOTFOUND, "not found", answer);
        evbuffer_free(answer);
        free(buf);
        return;
    }
    /* now output this fingerprint */
    int i;
    char outbuf[10240];
    char *p;
    if (!enc) {
        p = outbuf;
        for (i = 0; i < vlen; i++) {
            sprintf(p, "%016llx", val[i]);
            p += 16;
        }
        *p = '\0';
    } else {
        unsigned char enc[4096];
        int enclen;
        enclen = myencrypt(val, vlen, enc);
        p = outbuf;
        for (i = 0; i < enclen; i++) {
            sprintf(p, "%02x", enc[i]);
            p += 2;
        }
        *p = '\0';
    }
    evbuffer_add_printf(answer, "%s\n", outbuf);
    evhttp_send_reply(req, HTTP_OK, "OK", answer);
    evbuffer_free(answer);
    free(buf);
    return;
}

static void do_compare(struct evhttp_request *req, void *arg) {
    struct evbuffer *answer = evbuffer_new();
    size_t len = EVBUFFER_LENGTH(req->input_buffer);
    char *buf = (char *)malloc(len+1);
    if (!buf) {
        evbuffer_add_printf(answer, "Sorry\n");
        evhttp_send_reply(req, HTTP_NOTFOUND, "Internal error", answer);
        syslog(LOG_ERR, "Internal error with malloc buf");
        evbuffer_free(answer);
        return;
    }
    memset(buf, 0, len+1);
    memcpy(buf, EVBUFFER_DATA(req->input_buffer), len);
    uint64_t key, sk;
    int ret, total, cursor, d;
    sscanf(buf, "%lld", &key);
    total = 0;
    cursor = 0;
    while (total < limit) {
        ret = findb_compare(&db, key, dist, cursor, &sk, &d);
        if (ret == -1) {
            if (total != 0) break;
            evbuffer_add_printf(answer, "%lld 0\n", key);
            evhttp_send_reply(req, HTTP_NOTFOUND, "no similar", answer);
            evbuffer_free(answer);
            free(buf);
            return;
        }
        cursor = ret + 1;
        total++;
        evbuffer_add_printf(answer, "%lld %lld %3d\n", key, sk, d);
    }
    evhttp_send_reply(req, HTTP_OK, "OK", answer);
    evbuffer_free(answer);
    free(buf);
    return;
}

static void do_direct_compare(struct evhttp_request *req, void *arg) {
    struct evbuffer *answer = evbuffer_new();
    size_t len = EVBUFFER_LENGTH(req->input_buffer);
    char *buf = (char *)malloc(len+1);
    if (!buf) {
        evbuffer_add_printf(answer, "Sorry\n");
        evhttp_send_reply(req, HTTP_NOTFOUND, "Internal error", answer);
        syslog(LOG_ERR, "Internal error with malloc buf");
        evbuffer_free(answer);
        return;
    }
    memset(buf, 0, len+1);
    memcpy(buf, EVBUFFER_DATA(req->input_buffer), len);
    uint64_t sk;
    int ret, total, cursor, d;
    char valstr[MAX_FP_STR_LEN+1];
    uint64_t val[MAX_FP_LEN];
    int vallen, valstrlen;
    valstrlen = len > MAX_FP_STR_LEN ? MAX_FP_STR_LEN : len;
    strncpy(valstr, buf, valstrlen);
    valstr[valstrlen] = '\0';
    vallen = mystr2lls(valstr, val, enc);
    if (vallen == -1 && enc) {
        vallen = mystr2lls(valstr, val, 0);
    }
    if (vallen == -1) {
        evbuffer_add_printf(answer, "wrong fingerprint\n");
        evhttp_send_reply(req, HTTP_NOTFOUND, "Internal error", answer);
        syslog(LOG_ERR, "wrong fingerprint");
        evbuffer_free(answer);
        free(buf);
        return;
    }
    total = 0;
    cursor = 0;
    int mylimit = limit;
    int mydist = dist;
    char *uri = evhttp_decode_uri(req->uri);
    struct evkeyvalq params;
    evhttp_parse_query(uri, &params);
    const char *dist_str, *limit_str;
    dist_str = evhttp_find_header(&params, "dist");
    if (dist_str) sscanf(dist_str, "%d", &mydist);
    if (mydist < 0 || mydist > vallen*64) mydist = dist;
    limit_str = evhttp_find_header(&params, "limit");
    if (limit_str) sscanf(limit_str,"%d", &mylimit);
    if (mylimit < 0) mylimit = limit;
    while (total < mylimit || !mylimit) {
        ret = findb_direct_compare(&db, val, vallen, mydist, cursor, &sk, &d);
        if (ret == -1) {
            if (total != 0) break;
            evbuffer_add_printf(answer, "%s 0\n", valstr);
            evhttp_send_reply(req, HTTP_NOTFOUND, "no similar", answer);
            evbuffer_free(answer);
            free(buf);
            return;
        }
        cursor = ret + 1;
        total++;
        evbuffer_add_printf(answer, "%s %lld %3d\n", valstr, sk, d);
    }

    evhttp_send_reply(req, HTTP_OK, "OK", answer);
    evbuffer_free(answer);
    free(buf);
    return;
}

static void do_c2(struct evhttp_request *req, void *arg) {
    struct evbuffer *answer = evbuffer_new();
    size_t len = EVBUFFER_LENGTH(req->input_buffer);
    char *buf = (char *)malloc(len+1);
    if (!buf) {
        evbuffer_add_printf(answer, "Sorry\n");
        evhttp_send_reply(req, HTTP_NOTFOUND, "Internal error", answer);
        syslog(LOG_ERR, "Internal error with malloc buf");
        evbuffer_free(answer);
        return;
    }
    memset(buf, 0, len+1);
    memcpy(buf, EVBUFFER_DATA(req->input_buffer), len);
    uint64_t k1, k2;
    uint64_t *v1, *v2;
    int l1, l2;
    int ret, d;
    sscanf(buf, "%lld", &k1);
    ret = findb_query(&db, k1, &v1, &l1);
    if (ret == -1) {
        /* not found, output error */
        evbuffer_add_printf(answer, "key: %lld not found\n", k1);
        evhttp_send_reply(req, HTTP_NOTFOUND, "not found", answer);
        evbuffer_free(answer);
        free(buf);
        return;
    }
    char *p;
    p = buf;
    while (*p != ' ' && p < buf+len) p++;
    if (p == buf) {
        while (*p == ' ' && p < buf+len) p++;
        while (*p != ' ' && p < buf+len) p++;
    }
    while (*p == ' ' && p < buf+len) p++;
    sscanf(p, "%lld", &k2);
    ret = findb_query(&db, k2, &v2, &l2);
    if (ret == -1) {
        /* not found, output error */
        evbuffer_add_printf(answer, "key: %lld not found\n", k2);
        evhttp_send_reply(req, HTTP_NOTFOUND, "not found", answer);
        evbuffer_free(answer);
        free(buf);
        return;
    }
    if (l1 != l2) {
        evbuffer_add_printf(answer, "not same length\n");
        evhttp_send_reply(req, HTTP_NOTFOUND, "invalid", answer);
        evbuffer_free(answer);
        free(buf);
        return;
    }
    d = ham64s(v1, v2, l1);
    evbuffer_add_printf(answer, "%3d %4d %lld %lld\n", d, l1*64, k1, k2);
    evhttp_send_reply(req, HTTP_OK, "OK", answer);
    evbuffer_free(answer);
    free(buf);
    return;
}

static void do_info_quota(struct evhttp_request *req, void *arg){
    int total=0;
    int used=0;
    int left=0;
    struct evbuffer *answer = evbuffer_new();
    
    total = (db.h->size * 3)/4;
    used = db.h->item_no; 
    left = total - used;
    evbuffer_add_printf(answer, "%d %d %d\n", used, left, total);

    evhttp_send_reply(req, HTTP_OK, "OK", answer);
    evbuffer_free(answer);

    return;
}

static void do_info_all(struct evhttp_request *req, void *arg) {
    struct evbuffer *answer = evbuffer_new();
    evbuffer_add_printf(answer, "base: %s\n", db.b);
    evbuffer_add_printf(answer, "mode: %d\n", db.mode);
    evbuffer_add_printf(answer, "total: %d\n", db.h->item_no);
    int i;
    for (i = 0; i < db.max; i++) {
        if (db.m[i].num > 0) {
            evbuffer_add_printf(answer, "%d: %d\n", i+1, db.m[i].num);
        }
    }
    evhttp_send_reply(req, HTTP_OK, "OK", answer);
    evbuffer_free(answer);

    return;
}

static void do_info(struct evhttp_request *req, void *arg) {
    char *uri = evhttp_decode_uri(req->uri);
    struct evkeyvalq params;
    evhttp_parse_query(uri, &params);
    const char *show_str = NULL;

    show_str = evhttp_find_header(&params, "show");
    if (NULL == show_str) {
        do_info_all(req, arg);
        return;
    }
    if (0 == strcmp(show_str,"all")) {
        do_info_all(req, arg);
        return;
    } else if (0 == strcmp(show_str,"quota")) {
        do_info_quota(req, arg);
        return;
    }

    struct evbuffer *answer = evbuffer_new();
    evbuffer_add_printf(answer, "unknown input paras.\n");
    evhttp_send_reply(req, HTTP_NOTFOUND, "unknown input paras", answer);
    evbuffer_free(answer);
    return;
}

static void do_sync(struct evhttp_request *req, void *arg) {
    struct evbuffer *answer = evbuffer_new();
    int ret;
    ret = findb_sync(&db);
    if (ret) {
        evbuffer_add_printf(answer, "sync fail\n");
        evhttp_send_reply(req, HTTP_NOTFOUND, "sync fail", answer);
    } else {
        evbuffer_add_printf(answer, "sync OK\n");
        evhttp_send_reply(req, HTTP_OK, "OK", answer);
    }
    evbuffer_free(answer);

    return;
}

int siviosapi_start(char *addr, int port, char *b, int max, int m, int e) {
    int ret;

    ret = findb_open(&db, b, max, m);
    if (ret == -1) return -1;
    mode = m;
    enc = e;
    daemonize();
    signal(SIGPIPE, SIG_IGN);
    signal(SIGKILL, quit);
    signal(SIGTERM, quit);
    openlog("sivios", LOG_CONS | LOG_PERROR, LOG_USER);
    event_init();
    server = evhttp_start(addr, port);
    if (!server) return -1;
    syslog(LOG_INFO, "sivios(%d) is starting.", port);
    if (mode) {
        evhttp_set_cb(server, "/add", do_add, NULL);
        evhttp_set_cb(server, "/Add", do_Add, NULL);
        evhttp_set_cb(server, "/del", do_del, NULL);
        evhttp_set_cb(server, "/sync", do_sync, NULL);
    }
    evhttp_set_cb(server, "/q", do_query, NULL);
    evhttp_set_cb(server, "/c", do_compare, NULL);
    evhttp_set_cb(server, "/c2", do_c2, NULL);
    evhttp_set_cb(server, "/dc", do_direct_compare, NULL);
    evhttp_set_cb(server, "/info", do_info, NULL);
    event_dispatch();
    return 0;
}
