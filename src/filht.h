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

#include "htab.h"

/*
 * The magic number of filht file. I choose a special magic number
 * is 33550336, which is a completed number.
 */

#define FILHT_MAGIC 1841617246

typedef struct filht_h {
    u_int32_t magic;        /* The magic number of filht file */
    u_int32_t ts0;          /* The time stamp when filht is created */
    u_int32_t ts1;          /* The time stamp when filht is closed */
    u_int32_t size;         /* The size of hash table, i.e. HTAB_SIZE */
    u_int32_t item_no;      /* The entry number in hash table */
    u_int32_t del_no;       /* The del entry number in hash table */
    u_int32_t reserved;     /* reserved */
    u_int32_t tag;          /* The tag of filht file */
} filht_h_t;

typedef struct {
    int     fd;
    int     mode;
    int     is_new;
    filht_h_t head;
    ht_t   *ht;
} filht_t;

/* Several status for filht header's tag */
#define FILHT_NEW       0x00    /* Totally fresh filht, untouched */
#define FILHT_OPENED    0x01    /* The filht file is in memory */
#define FILHT_CLOSED    0x02    /* The conent in memory already write back */

/* Some error number */
#define FILHT_ERR_OPEN     1    /* Failure open file */
#define FILHT_ERR_FORMAT   2    /* Bad file format */
#define FILHT_ERR_CLOSE    3    /* Failure close file */
#define FILHT_ERR_FULL     4    /* File is full */
#define FILHT_ERR_WRITE    5    /* Failure write file */
#define FILHT_ERR_READ     6    /* Failure read file */
#define FILHT_ERR_SYNC     7    /* This filht file not sync correctly */
#define FILHT_ERR_RDONLY   8    /* Try writing when open with readonly mode */

#ifdef __cplusplus
extern  "C" {
#endif

int filht_load(filht_t *filht, char *fn, int mode);
int filht_save(filht_t *filht, char *fn);

int filht_clear(char *fn);
int filht_new(char *fn);
int filht_undo(char *fn);

int filht_errno();
void filht_perror(char *msg);
void filht_print_header(filht_t *filht);
void filht_print_title();

#ifdef __cplusplus
}
#endif
