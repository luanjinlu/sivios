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

#pragma once

#include <stdio.h>

#include "htab.h"
#include "filht.h"
#include "memfin.h"

#define MAX_FP_LEN    50

typedef struct findb {
    char b[1024];
    int max;
    int mode;
    int uniq;
    memfin_t *m;
    filht_t *f;
    ht_t *h;
    htab_t *t;
} findb_t;

#ifdef __cplusplus
extern "C" {
#endif
    
int findb_open(findb_t *db, char *b, int max, int mode);
int findb_close(findb_t *db);
int findb_sync(findb_t *db);
int findb_query(findb_t *db, uint64_t key, uint64_t **v, int *len);
int findb_add(findb_t *db, uint64_t key, uint64_t *v, int len);
int findb_add_uniq(findb_t *db, uint64_t key, uint64_t *v, int len);
int findb_del(findb_t *db, uint64_t key);
int findb_compare(findb_t *db, uint64_t key, int dist, int c, uint64_t *k, int *d);
void findb_compare_all(findb_t *db, int dist);
int findb_direct_compare(findb_t *db, uint64_t *val, int len, int dist, int c, uint64_t *k, int *d);
int findb_output(findb_t *db, char *b, int max, int limit);

#ifdef __cplusplus
}
#endif
