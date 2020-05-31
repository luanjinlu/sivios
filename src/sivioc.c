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
static char revision[] = "";

#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "htab.h"
#include "filht.h"
#include "memfin.h"
#include "findb.h"
#include "utils.h"

#define MAX_FP_STR_LEN   MAX_FP_LEN*sizeof(uint64_t)*2
static int max_fp_len = MAX_FP_LEN;
static char base[1024] = "/data/f";
static int dist = 10;
static int limit = 3;
static int mode = 0;

static findb_t db;

enum Tasks {Add, Del, Query, Compare, CompareAll, C2, DirectCompare, AddAfterCompare, Init, Output, Info};

static void usage(char *prog) {
    printf("Usage: %s [-aAqc2CDorhviU][-d <dist>] [-l <limit>] [-b <base>] [-n <max>]\n",
           prog);
    printf("       -a, add a fingerprint.\n");
    printf("       -A, add a fingerprint after comparing.\n");
    printf("       -q, query if fingerprint exist.\n");
    printf("       -c, compare if fingerprint has similar ones.\n");
    printf("       -0, compare all similar pairs.\n");
    printf("       -2, compare two fingerprints.\n");
    printf("       -D, compare directly by fingerprints, not ID.\n");
    printf("       -C, create/init fingerprint database.\n");
    printf("       -o, output (dump) fingerprint database.\n");
    printf("       -r, remove fingerprint by keys.\n");
    printf("       -d <dist>, shortest distance when compare.\n");
    printf("       -l <limit>, limit of similar output.\n");
    printf("       -b <base>, base path name for fingerprint.\n");
    printf("       -n <max>, the max length of fingerprint.\n");
    printf("       -h, show help info.\n");
    printf("       -v, show program's version.\n");
    printf("       -i, show info.\n");
    printf("       -U, create a uniq fingerprint database.\n");
    return;
}

static void do_add(int uniq) {
    char buf[20481];
    uint64_t key;
    int ret;
    uint64_t val[MAX_FP_LEN];
    int len;
    char valstr[MAX_FP_STR_LEN+1];
    //uint64_t val[MAX_FP_LEN];
    int vallen, valstrlen;

    while (scanf("%lld", &key) != EOF) {
        /* now begin processing */
        /* next get the fingerprint */
        scanf("%20480s", buf);
        len = strlen(buf);
        valstrlen = len > MAX_FP_STR_LEN ? MAX_FP_STR_LEN : len;
        strncpy(valstr, buf, valstrlen);
        valstr[valstrlen] = '\0';
        vallen = str2lls(valstr, val);
        if (vallen == -1) {
            /* wrong fingerprint */
            printf("%lld -1\n", key);
            continue;
        }
        /* now add this fingerprint */
        if (uniq) {
            ret = findb_add_uniq(&db, key, val, vallen);
        } else {
            ret = findb_add(&db, key, val, vallen);
        }
        if (ret == -1) {
            /* add to findb fail */
            printf("%lld 0\n", key);
            continue;
        }
        /* finally, success. */
        printf("%lld 1\n", key);
    }
}

static void do_del() {
    uint64_t key;
    int ret;

    while (scanf("%lld", &key) != EOF) {
        ret = findb_del(&db, key);
        if (ret == -1) {
            printf("%lld 0\n", key);
            continue;
        }
        printf("%lld 1\n", key);
    }
}

static void do_query() {
    uint64_t key;
    uint64_t *v;
    int ret;
    int len;

    while (scanf("%lld", &key) != EOF) {
        /* now begin processing */
        ret = findb_query(&db, key, &v, &len);
        if (ret == -1) {
            /* not exist, so return error */
            printf("%lld 0\n", key);
            continue;
        }
        /* next get the fingerprint */
        int i;
        printf("%lld ", key);
        for (i = 0; i < len; i++)
            //printf("%016llx", mfs[vlen-1].val[where+i]);
            printf("%016llx", v[i]);
        printf("\n");
    }
}

static void do_c2() {
    uint64_t k1, k2;
    uint64_t *v1, *v2;
    int l1, l2;
    int ret, d;

    while (scanf("%lld %lld", &k1, &k2) != EOF) {
        ret = findb_query(&db, k1, &v1, &l1);
        if (ret == -1) {
            /* not exist, try next two keys */
            continue;
        }
        ret = findb_query(&db, k2, &v2, &l2);
        if (ret == -1) continue;
        if (l1 != l2) continue;
        d = ham64s(v1, v2, l1);
        printf("%3d %4d %3d %lld %lld\n", d, l1*64, d*100/(l1*64), k1, k2);
    }
}

static int do_compare(int dist, int limit) {
    uint64_t key, sk;
    int ret;
    int total;
    int cursor;
    int d;

    if (dist < 0 || limit < 0) return -1;
    while (scanf("%lld", &key) != EOF) {
        total = 0;
        cursor = 0;
        while (total < limit || !limit) {
            ret = findb_compare(&db, key, dist, cursor, &sk, &d);
            if (ret == -1) break;
            /* found one */
            cursor = ret + 1;
            printf("%lld %lld %3d\n", key, sk, d);
            total++;
        }
    }
    return 0;
}

static int do_compare_all(int dist) {
    if (dist < 0 || dist > 100) return -1;
    findb_compare_all(&db, dist);
    return 0;
}

static int do_direct_compare(int dist, int limit) {
    uint64_t sk;
    int ret, len;
    int total;
    int cursor;
    int d;
    char buf[2048];
    uint64_t val[MAX_FP_LEN];

    if (dist < 0 || limit < 0) return -1;
    while (scanf("%s", buf) != EOF) {
        len = str2lls(buf, val);
        if (len == -1) continue;
        total = 0;
        cursor = 0;
        while (total < limit || !limit) {
            ret = findb_direct_compare(&db, val, len, dist, cursor, &sk, &d);
            if (ret == -1) break;
            /* found one */
            cursor = ret + 1;
            printf("%s %lld %3d\n", buf, sk, d);
            total++;
        }
    }
    return 0;
}

static int do_output() {
    return findb_output(&db, base, max_fp_len, limit);
#if 0    
    int i, j, k, len;
    for (i = 0; i < db.max; i++) {
        len = db.m[i].len;
        for (j = 0; j < db.m[i].num; j++) {
            printf("%lld ", db.m[i].key[j]);
            for (k = 0; k < len; k++)
                printf("%016llx", db.m[i].val[j*len+k]);
            printf("\n");
        }
    }
    return 0;
#endif
}

static int do_info() {
    printf("base: %s\n", db.b);
    printf("mode: %d\n", db.mode);
    printf("total: %d\n", db.f->head.item_no);
    int i;
    for (i = 0; i < db.max; i++) {
        if (db.m[i].num > 0) {
            printf("%d: %d\n", i+1, db.m[i].num);
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    char ch;
    int ret;
    enum Tasks t;
    
    while((ch = getopt(argc, argv, "aAqc02CDord:l:b:n:hviU")) != -1) {
        switch (ch) {
        case 'a':
            t = Add;
            mode = 1;
            break;
        case 'A':
            t = AddAfterCompare;
            mode = 1;
            break;
        case 'q':
            t = Query;
            break;
        case 'c':
            t = Compare;
            break;
        case '0':
            t = CompareAll;
            break;
        case '2':
            t = C2;
            break;
        case 'D':
            t = DirectCompare;
            break;
        case 'C':
            t = Init;
            mode = 2;
            break;
        case 'U':
            t = Init;
            mode = 3;
            break;
        case 'o':
            t = Output;
            break;
        case 'r':
            t = Del;
            mode = 1;
            break;
        case 'd':
            sscanf(optarg, "%d", &dist);
            break;
        case 'l':
            sscanf(optarg, "%d", &limit);
            break;
        case 'b':
            sscanf(optarg, "%s", base);
            break;
        case 'n':
            sscanf(optarg, "%d", &max_fp_len);
            break;
        case 'v':
            printf("%s\n", revision);
            return 0;
        case 'i':
            t = Info;
            break;
        case 'h':
        case '?':
        default:
            usage(argv[0]);
            return 1;
        }
    }
    argc -= optind;
    if (argc > 0) {
        usage(argv[0]);
        return 1;
    }

    if (t == Output) {
        ret = findb_output(&db, base, max_fp_len, limit);
        if (ret == -1) {
            printf("output/dump error!\n");
            return 1;
        }
        return 0;
    }
    ret = findb_open(&db, base, max_fp_len, mode);
    if (ret == -1) {
        printf("findb_open error!\n");
        return 1;
    }

    switch (t) {
    case Add:
        do_add(0);
        break;
    case AddAfterCompare:
        do_add(1);
        break;
    case Del:
        do_del();
        break;
    case Query:
        do_query();
        break;
    case Compare:
        do_compare(dist, limit);
        break;
    case CompareAll:
        do_compare_all(dist);
        break;
    case C2:
        do_c2();
        break;
    case DirectCompare:
        do_direct_compare(dist, limit);
        break;
    case Info:
        do_info();
        break;
    case Init:
    default:
        break;
    }

    ret = findb_close(&db);
    if (ret == -1) {
        printf("findb_close error!\n");
        return 1;
    }

    return 0;
}
