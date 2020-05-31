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

#include "findb.h"
#include "utils.h"

#define myhamming(m,a,b,l) ham64s(&(m[l-1].val[a]),&(m[l-1].val[b]),l)

static htab_t tbl[HTAB_SIZE];
static ht_t htbl;
static filht_t fht;
static memfin_t mfs[MAX_FP_LEN];

static int stages[7] = {1, 2, 5, 9, 19, 50, 100};

static int which_level(int n) {
    int i;
    if (n < 1 || n > MAX_FP_LEN) return -1;
    for (i = 0; i < 7; i++)
        if (n <= stages[i]) return i;
    return -1;
}

static int init_filht(char *b) {
    char fn[1024];
    int ret;
    if (!b) return -1;
    sprintf(fn, "%s/filht.tbl\0", b);
    ret = filht_new(fn);
    return ret;
}

static int init_mf(int max) {
    int i, ret, level;
    if (max < 1 || max > MAX_FP_LEN) return -1;
    for (i = 1; i <= max; i++) {
        level = which_level(i) + 1;
        ret = memfin_init(&mfs[i-1], i, level);
        if (ret == -1) return -1;
    }
    return 0;
}

static int init_mf_files(char *b, int max) {
    int i, ret;
    char fn[1024];

    if (!b) return -1;
    if (max < 1 || max > MAX_FP_LEN) return -1;
    for (i = 0; i < max; i++ ) {
        sprintf(fn, "%s/%03d", b, i+1);
        ret = memfin_save(fn, &mfs[i]);
        if (ret == -1) return -1;
    }
    return 0;
}

static void free_mf(int max) {
    int i;
    if (max < 1 || max > MAX_FP_LEN) return;
    for (i = 0; i < max; i++)
        memfin_free(&mfs[i]);
    return;
}

static int init_all(findb_t *db, char *b, int max, int uniq) {
    int ret;

    if (!db) return -1;
    ret = init_filht(b);
    if (ret == -1) return -1;
    ret = init_mf(max);
    if (ret == -1) return -1;
    ret = init_mf_files(b, max);
    if (ret == -1) return -1;

    FILE *uf;
    char fn[1024];
    sprintf(fn, "%s/.uniq", b);
    uf = fopen(fn, "w");
    if (!uf) return -1;
    fprintf(uf, "%d\n", uniq);
    fclose(uf);
    db->uniq = uniq;

    /* finially, all ok */
    strncpy(db->b, (const char*)b, strlen((const char*)b));
    db->max = max;
    db->mode = 2;
    db->m = mfs;
    db->f = &fht;
    db->h = &htbl;
    db->t = tbl;
    
    return 0;
}

int findb_open(findb_t *db, char *b, int max, int mode) {
    if (!db || !b || max < 1 || max > MAX_FP_LEN) return -1;
    int i, ret;
    char fn[1024];
    FILE *uf;

    if (mode == 3) {
        ret = init_all(db, b, max, 1);
        return ret;
    }
    if (mode == 2) {
        ret = init_all(db, b, max, 0);
        return ret;
    }
    /* open uniq file and set tag of uniq */
    sprintf(fn, "%s/.uniq", b);
    uf = fopen(fn, "r");
    if (!uf) return -1;
    fscanf(uf, "%d", &(db->uniq));
    fclose(uf);

    fht.ht = &htbl;
    fht.ht->tbl = tbl;
    if (!mode) {
        /* open hash table first */
        sprintf(fn, "%s/filht.tbl", b);
        ret = filht_load(&fht, fn, 0);
        if (ret == -1) return -1;

        /* open memfin data then */
        for (i = 0; i < max; i++) {
            sprintf(fn, "%s/%03d", b, i+1);
            ret = memfin_load_readonly(fn, &mfs[i]);
            if (ret == -1) return -1;
        }
    } else {
        ret = init_mf(max);
        if (ret == -1) return -1;
        /* open hash table first */
        sprintf(fn, "%s/filht.tbl", b);
        ret = filht_load(&fht, fn, 1);
        if (ret == -1) return -1;

        /* open memfin data then */
        for (i = 0; i < max; i++) {
            sprintf(fn, "%s/%03d", b, i+1);
            ret = memfin_load(fn, &mfs[i]);
            if (ret == -1) return -1;
        }
    }
    /* finally, ok */
    strncpy(db->b, (const char*)b, strlen((const char*)b));
    db->max = max;
    db->mode = mode;
    db->m = mfs;
    db->f = &fht;
    db->h = &htbl;
    db->t = tbl;
    return 0;
}

int findb_close(findb_t *db) {
    if (!db) return -1;
    if (db->mode == 2) {
        free_mf(db->max);
        return 0;
    }
    if (!(db->mode)) {
        free_mf(db->max);
        return 0;
    }
    /* save hash table first */
    char fn[1024];
    int i, ret;
    sprintf(fn, "%s/filht.tbl", db->b);
    ret = filht_save(db->f, fn);
    if (ret == -1) return -1;

    /* save memfin data then */
    for (i = 0; i < db->max; i++) {
        sprintf(fn, "%s/%03d", db->b, i+1);
        ret = memfin_append(fn, &(db->m[i]));
        if (ret == -1) return -1;
    }
    free_mf(db->max);
    return 0;
}

int findb_sync(findb_t *db) {
    if (!db) return -1;

    /* not for readonly mode */
    if (!(db->mode)) return -1;
    
    /* sync hash table first */
    char fn[1024];
    int i, ret;
    sprintf(fn, "%s/filht.tbl", db->b);
    ret = filht_save(db->f, fn);
    if (ret == -1) return -1;

    /* sync memfin data then */
    for (i = 0; i < db->max; i++) {
        sprintf(fn, "%s/%03d", db->b, i+1);
        ret = memfin_append(fn, &(db->m[i]));
        if (ret == -1) return -1;
    }
    return 0;
}

int findb_query(findb_t *db, uint64_t key, uint64_t **v, int *len) {
    if (!db || !v || !len) return -1;
    int i, ret, vlen;
    uint32_t where;
    htab_t hi;

    hi.key = key;
    ret = htab_find(db->h, &hi);
    if (ret == -1) return -1;
    vlen = db->t[ret].len;
    where = db->t[ret].index * vlen;
    *len = vlen;
    *v = &(db->m[vlen-1].val[where]);
    return 0;
}

int findb_add(findb_t *db, uint64_t key, uint64_t *v, int len) {
    int ret;
    htab_t hi;
    
    if (!db || !v || len < 1 || len > MAX_FP_LEN) return -1;
    if (db->uniq) return findb_add_uniq(db, key, v, len);
    hi.key = key;
    ret = htab_find(db->h, &hi);
    if (ret != -1) return -1;
    /* first add to memfin */
    ret = memfin_add(&(db->m[len-1]), len, hi.key, v);
    if (ret == -1) return -1;
    /* then add to htab */
    hi.len = len;
    hi.index = db->m[len-1].num - 1;
    ret = htab_insert(db->h, &hi);
    if (ret == -1) return -1;
    /* finally, success */
    return 0;
}

int findb_add_uniq(findb_t *db, uint64_t key, uint64_t *v, int len) {
    int i, ret;
    htab_t hi;

    if (!db || !v || len < 1 || len > MAX_FP_LEN) return -1;
    hi.key = key;
    ret = htab_find(db->h, &hi);
    if (ret != -1) return -1;
    /* check if a same val exist */
    for (i = 0; i < db->m[len-1].num; i++) {
        if (ham64s(&(db->m[len-1].val[i]), v, len) == 0) {
            /* exist same val, so quit */
            return -1;
        }
    }
    /* first add to memfin */
    ret = memfin_add(&(db->m[len-1]), len, hi.key, v);
    if (ret == -1) return -1;
    /* then add to htab */
    hi.len = len;
    hi.index = db->m[len-1].num - 1;
    ret = htab_insert(db->h, &hi);
    if (ret == -1) return -1;
    /* finally, success */
    return 0;
}

int findb_del(findb_t *db, uint64_t key) {
    int ret;
    htab_t hi;

    if (!db) return -1;
    hi.key = key;
    ret = htab_find(db->h, &hi);
    if (ret == -1) return -1;
    int vlen = db->t[ret].len;
    int where = db->t[ret].index;
    db->m[vlen-1].tag[where] = 1;
    db->m[vlen-1].modified = 1;
    htab_remove(db->h, &hi);
    return 0;
}

int findb_compare(findb_t *db, uint64_t key, int dist, int c, uint64_t *k, int *d) {
    htab_t hi;
    int i, ret, len, index;
    uint32_t where;
    int hd;
    
    if (!db || dist < 0 || c < 0 || !d) return -1;
    hi.key = key;
    ret = htab_find(db->h, &hi);
    if (ret == -1) return -1;
    len = db->t[ret].len;
    index = db->t[ret].index;
    where = index * len;
    i = c;
    while (i < db->m[len-1].num) {
        if (i == index || db->m[len-1].tag[i] == 1) {
            i++;
            continue;
        }
        hd = myhamming(db->m, where, i*len, len);
        hd = hd*100/(len*64);
        if (hd <= dist) {
            /* OK, find a similar one */
            *d = hd;
            *k = db->m[len-1].key[i];
            return i;
        }
        i++;
    }
    return -1;
}

void findb_compare_all(findb_t *db, int dist) {
    int i, j, len, hd;
    if (!db || dist < 0 || dist > 100) return;
    for (len = 1; len <= MAX_FP_LEN; len++) {
        for (i = 0; i < db->m[len-1].num; i++) {
            if (db->m[len-1].tag[i] == 1) continue;
            for (j = 0; j < i; j++) {
                if (db->m[len-1].tag[j] == 1) continue;
                hd = myhamming(db->m, i*len, j*len, len);
                hd = hd*100/(len*64);
                if (hd <= dist) {
                    printf("%lld %lld %d\n", db->m[len-1].key[i], db->m[len-1].key[j], hd);
                }
            }
        }
    }
    return;
}

int findb_direct_compare(findb_t *db, uint64_t *val, int len, int dist, int c, uint64_t *k, int *d) {
    int i, hd;
    if (!db || !val || len <= 0 || c < 0 || !k || !d) return -1;
    i = c;
    while (i < db->m[len-1].num) {
        if (db->m[len-1].tag[i] == 1) {
            i++;
            continue;
        }
        hd = ham64s(&(db->m[len-1].val[i*len]), val, len);
        hd = hd*100/(len*64);
        if (hd <= dist) {
            *d = hd;
            *k = db->m[len-1].key[i];
            return i;
        }
        i++;
    }
    return -1;
}

int findb_output(findb_t *db, char *b, int max, int limit) {
    if (!db || !b || max < 1 || max > MAX_FP_LEN) return -1;
    int i, j, k, len, ret, total = 0;
    char fn[1024];

    for (i = 0; i < max; i++) {
        sprintf(fn, "%s/%03d", b, i+1);
        ret = memfin_load_readonly(fn, &mfs[i]);
        if (ret == -1) return -1;
    }
    strncpy(db->b, (const char*)b, strlen((const char*)b));
    db->max = max;
    db->m = mfs;
    for (i = 0; i < db->max; i++) {
        len = db->m[i].len;
        for (j = 0; j < db->m[i].num; j++) {
            if (db->m[i].tag[j] == 1) continue;
            printf("%lld ", db->m[i].key[j]);
            for (k = 0; k < len; k++)
                printf("%016llx", db->m[i].val[j*len+k]);
            printf("\n");
            total++;
            if (total >= limit && limit) return 0;
        }
    }
    return 0;
}
