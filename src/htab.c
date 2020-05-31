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

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "htab.h"

/*
 * The default hash function, Phong's linear congruential hash
 */
static uint32_t htab_hash_default(uint64_t key);

/*
 * when mode=0, empty the hash table, and no va args;
 * when mode=1, keep the hash table, and has more args.
 */
void htab_init(ht_t *ht, htab_t *tbl, htab_hf hf, int mode, ...)
{
    va_list ap;
    int     item_no;
    int     del_no;

    assert(ht);
    assert(tbl);

    ht->tbl = tbl;
    ht->hf = hf ? hf : htab_hash_default;

    if (!mode) {
        htab_empty(ht);
        return;
    }

    /* begin variable arguement parameter */
    va_start(ap, mode);
    item_no = va_arg(ap, int);
    del_no = va_arg(ap, int);
    va_end(ap);

    assert(item_no >= 0);
    assert(del_no >= 0);

    ht->item_no = item_no;
    ht->del_no = del_no;
    ht->size = HTAB_SIZE;
    return;
}

void htab_empty(ht_t *ht)
{
    int     i;

    assert(ht);

    ht->del_no = 0;
    ht->item_no = 0;
    ht->size = HTAB_SIZE;
    ht->hf = htab_hash_default;
    for (i = 0; i < ht->size; i++)
        ht->tbl[i].tag = HTAB_EMPTY;

    return;
}

int htab_find_with_hash(ht_t *ht, htab_t *item, uint32_t hv)
{
    int     hv2;
    int     idx;

    assert(ht);
    assert(item);

    hv2 = 1 + hv % (ht->size - 2);
    idx = hv % ht->size;

 loop:
    if (ht->tbl[idx].tag == HTAB_EMPTY) {
        /* not in this hash table */
        return -1;
    }
    if (ht->tbl[idx].tag == HTAB_DELETED) {
        if (item->key == ht->tbl[idx].key) {
            /* already deleted in hash table */
            return -1;
        }
    } else if (item->key == ht->tbl[idx].key) {
        /* OK, found it */
        return idx;
    }

    /* In this case, a collision happened */
    idx += hv2;
    idx = (idx >= ht->size) ? (idx - ht->size) : idx;
    goto loop;
}

int htab_find(ht_t *ht, htab_t *item)
{
    assert(ht);
    assert(item);
    assert(ht->hf);

    return htab_find_with_hash(ht, item, ht->hf(item->key));
}

int htab_insert_with_hash(ht_t *ht, htab_t *item, uint32_t hv)
{
    int     hv2;
    int     idx;
    int     first_del = -1;

    assert(ht);
    assert(item);

    hv2 = 1 + hv % (ht->size - 2);
    idx = hv % ht->size;

    if (ht->size * 3 <= ht->item_no * 4) {
        /* Sorry, the hash table nearly full */
        return -1;
    }

 loop:
    if (ht->tbl[idx].tag == HTAB_EMPTY) {
        ht->item_no++;
        if (first_del != -1) {
            ht->tbl[first_del].tag = HTAB_EMPTY;
            idx = first_del;
            ht->del_no--;
        }
        ht->tbl[idx].tag = HTAB_USED;
        ht->tbl[idx].key = item->key;
        ht->tbl[idx].flag = item->flag;
        ht->tbl[idx].len = item->len;
        ht->tbl[idx].index = item->index;
        return idx;
    }
    if (ht->tbl[idx].tag == HTAB_DELETED) {
        if (first_del == -1)
            first_del = idx;
    } else if (item->key == ht->tbl[idx].key) {
        /* OK, I find it, and overlay it directly */
        ht->tbl[idx].tag = HTAB_USED;
        ht->tbl[idx].key = item->key;
        ht->tbl[idx].flag = item->flag;
        ht->tbl[idx].len = item->len;
        ht->tbl[idx].index = item->index;
        return idx;
    }

    idx += hv2;
    idx = (idx >= ht->size) ? (idx - ht->size) : idx;
    goto loop;
}

int htab_insert(ht_t *ht, htab_t *item)
{
    assert(ht);
    assert(item);
    assert(ht->hf);

    return htab_insert_with_hash(ht, item, ht->hf(item->key));
}

int htab_remove_with_hash(ht_t *ht, htab_t *item, uint32_t hv)
{
    int     idx;

    assert(ht);
    assert(item);

    idx = htab_find_with_hash(ht, item, hv);
    if (idx == -1)
        return -1;
    ht->tbl[idx].tag = HTAB_DELETED;
    ht->del_no++;
    ht->item_no--;
    return idx;
}

int htab_remove(ht_t *ht, htab_t *item)
{
    int     idx;

    assert(ht);
    assert(item);

    idx = htab_find(ht, item);
    if (idx == -1)
        return -1;
    ht->tbl[idx].tag = HTAB_DELETED;
    ht->del_no++;
    ht->item_no--;
    return idx;
}

/* Phong's linear congruential hash function. */
#define dcharhash(h, c) ((h) = 0x63c63cd9*(h) + 0x9c39c33d + (c))

static uint32_t htab_hash_default(uint64_t key)
{
    uint32_t h = 0;
    uint8_t c;

    c = key & 0xff;
    dcharhash(h, c);
    c = (key >> 8) & 0xff;
    dcharhash(h, c);
    c = (key >> 16) & 0xff;
    dcharhash(h, c);
    c = (key >> 24) & 0xff;
    dcharhash(h, c);
    c = (key >> 32) & 0xff;
    dcharhash(h, c);
    c = (key >> 40) & 0xff;
    dcharhash(h, c);
    c = (key >> 48) & 0xff;
    dcharhash(h, c);
    c = (key >> 56) & 0xff;
    dcharhash(h, c);

    return h;
}

