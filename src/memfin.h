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

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#ifndef HTAB_SIZE
#define HTAB_SIZE    67108859
#endif

/* max numbers of fingerprint, it is related with the size of HTAB_SIZE */
#define FP_MAX1 HTAB_SIZE*3/5
#define FP_MAX2 HTAB_SIZE*3/20
#define FP_MAX3 HTAB_SIZE*9/80
#define FP_MAX4 HTAB_SIZE*3/80
#define FP_MAX5 HTAB_SIZE*3/400
#define FP_MAX6 HTAB_SIZE*3/800
#define FP_MAX7 HTAB_SIZE*3/8000

typedef struct memfin {
    int original;         /* original numbers of fingerprint */
    int num;              /* numbers of fingerprint */
    int max;              /* the  maximium fingerprint */
    int len;              /* lenght of fingerprint, in 64bit unit. */
    int modified;         /* if add new of remove a key */
    uint64_t* val;        /* storage pointer of fingerprint */
    uint64_t* key;        /* the key or vid of fingerprint */
    uint8_t* tag;         /* tag of fingerprint */
} memfin_t;

#ifdef __cplusplus
extern "C" {
#endif

int memfin_init(memfin_t *mf, int len, int level);
void memfin_free(memfin_t *mf);
int memfin_add(memfin_t *mf, int len, uint64_t key, uint64_t *val);
int memfin_load(char *fn, memfin_t *mf);
int memfin_save(char *fn, memfin_t *mf);
int memfin_append(char *fn, memfin_t *mf);

#ifdef __cplusplus
}
#endif
