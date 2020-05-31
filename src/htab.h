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
#include <stdint.h>

#ifndef HTAB_SIZE
#define HTAB_SIZE   67108859
#endif

#define HTAB_EMPTY         0
#define HTAB_DELETED       1
#define HTAB_USED          2

/*
 * I choose the following structure carefully to fit 2 64-bits,
 * so there is no need to use '#pragma pack(1)' for less memory
 * usage.
 */
typedef struct htab {
    uint64_t key;
    uint8_t tag;                /* the tag of slot, empty, deleted, or used */
    uint8_t flag;               /* some properties of the key */
    uint16_t len;               /* length of fingerprint (with 64-bits unit) */
    uint32_t index;             /* index of fingerprint stored in the block */
} htab_t;

typedef uint32_t(*htab_hf) (uint64_t);

typedef struct ht {
    int     size;               /* The limit of the hash table, HTAB_SIZE */
    int     del_no;             /* The number of items that already deleted */
    int     item_no;            /* The normal number of items in hash table */
    htab_hf hf;                 /* The hash function used in hash table */
    htab_t *tbl;                /* The base address of hash table */
} ht_t;

#ifdef __cplusplus
extern  "C" {
#endif

void    htab_init(ht_t *ht, htab_t *tbl, htab_hf hf, int mode, ...);
void    htab_empty(ht_t *ht);

int     htab_find_with_hash(ht_t *ht, htab_t *item, uint32_t hv);
int     htab_find(ht_t *ht, htab_t *item);
int     htab_insert_with_hash(ht_t *ht, htab_t *item, uint32_t hv);
int     htab_insert(ht_t *ht, htab_t *item);
int     htab_remove_with_hash(ht_t *ht, htab_t *item, uint32_t hv);
int     htab_remove(ht_t *ht, htab_t *item);

#ifdef __cplusplus
}
#endif
