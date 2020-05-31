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
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "filht.h"

static int _errno = 0;          /* error number */
static char *_errmsg[] = {      /* the error message */
    "No error",
    "Failure open file",
    "Bad file format",
    "Failure close file",
    "File is full",
    "Failure write file",
    "Failure read file",
    "Not been sync correctly",
    "Try writing but readonly"
};

int filht_load(filht_t *filht, char *fn, int mode) {
    int     ret;
    int     item_no;
    int     del_no;

    assert(filht);
    assert(filht->ht);
    assert(filht->ht->tbl);
    assert(fn);

    filht->mode = mode;
    filht->is_new = 0;
    if (!mode) {
        filht->fd = open(fn, O_RDONLY);
    } else {
        filht->fd = open(fn, O_RDWR);
    }
    if (-1 == filht->fd) {
        _errno = FILHT_ERR_OPEN;
        return -1;
    }
    ret = read(filht->fd, (void *)&(filht->head), sizeof(filht_h_t));
    if (ret != sizeof(filht_h_t)) {
        _errno = FILHT_ERR_FORMAT;
        close(filht->fd);
        return -1;
    }
    if (filht->head.magic != FILHT_MAGIC && filht->head.magic != 0) {
        /* Not a filht file */
        _errno = FILHT_ERR_FORMAT;
        close(filht->fd);
        return -1;
    }

    if (filht->head.tag == FILHT_NEW && mode) {
        struct timeval t0;

        gettimeofday(&t0, NULL);
        filht->head.magic = FILHT_MAGIC;
        filht->head.ts0 = t0.tv_sec;
        filht->head.size = HTAB_SIZE;
        filht->head.tag = FILHT_OPENED;
        filht->head.reserved = 0;
        filht->head.item_no = 0;
        filht->head.del_no = 0;

        /* Initialize hash table */
        htab_init(filht->ht, filht->ht->tbl, NULL, 0);

        /* write back the header immidiatly */
        ret = lseek(filht->fd, 0, SEEK_SET);
        if (-1 == ret) {
            _errno = FILHT_ERR_WRITE;
            close(filht->fd);
            return -1;
        }
        ret = write(filht->fd, (void *)&(filht->head), sizeof(filht_h_t));
        if (-1 == ret) {
            _errno = FILHT_ERR_WRITE;
            close(filht->fd);
            return -1;
        }
        filht->is_new = 1;
    } else if (filht->head.tag == FILHT_NEW && !mode) {
        /*
         * It is no use when the filht file is new, while application
         * program open it in readonly mode.
         */
        _errno = FILHT_ERR_OPEN;
        close(filht->fd);
        return -1;
    }

    if (mode) {
        filht->head.tag = FILHT_OPENED;
    }
    if (filht->is_new) {
        /* Already initialized hash table when open the filht file. */
        close(filht->fd);
        return 0;
    }
    item_no = filht->head.item_no;
    del_no = filht->head.del_no;
    assert(item_no >= 0);
    assert(del_no >= 0);

    lseek(filht->fd, sizeof(filht_h_t), SEEK_SET);
    ret = read(filht->fd,
               (void *)filht->ht->tbl,
               sizeof(htab_t) * filht->head.size);

    /* Initialize hash table with mode 1 */
    htab_init(filht->ht, filht->ht->tbl, NULL, 1, item_no, del_no);

    if (ret != (int)(sizeof(htab_t) * filht->head.size)) {
        _errno = FILHT_ERR_READ;
        return -1;
    }
    close(filht->fd);

    return 0;
}

int filht_save(filht_t *filht, char *fn) {
    int     ret;
    struct timeval t1;

    assert(filht);
    assert(filht->ht);
    assert(filht->ht->tbl);
    assert(fn);

    gettimeofday(&t1, NULL);
    filht->head.ts1 = t1.tv_sec;

    if (!filht->mode) {
        _errno = FILHT_ERR_RDONLY;
        return -1;
    }
    filht->fd = open(fn, O_RDWR);
    if (-1 == filht->fd) {
        _errno = FILHT_ERR_WRITE;
        return -1;
    }
    filht->head.tag = FILHT_CLOSED;
    filht->head.item_no = filht->ht->item_no;
    filht->head.del_no = filht->ht->del_no;
    ret = write(filht->fd, (void *)&(filht->head), sizeof(filht_h_t));
    if (-1 == ret) {
        _errno = FILHT_ERR_CLOSE;
        close(filht->fd);
        return -1;
    }
    ret = write(filht->fd,
                (void *)filht->ht->tbl,
                sizeof(htab_t) * filht->head.size);
    if (ret != (int)(sizeof(htab_t) * filht->head.size)) {
        _errno = FILHT_ERR_WRITE;
        return -1;
    }
    close(filht->fd);

    return 0;
}

int filht_clear(char *fn) {
    int     fd;
    int     ret;
    filht_h_t head;

    assert(fn);

    fd = open(fn, O_RDWR);
    if (-1 == fd) {
        return -1;
    }
    head.magic = FILHT_MAGIC;
    head.size = HTAB_SIZE;
    head.item_no = 0;
    head.del_no = 0;
    head.reserved = 0;
    head.tag = FILHT_NEW;
    ret = write(fd, (void *)&head, sizeof(head));
    if (ret != sizeof(head)) {
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}

int filht_new(char *fn) {
    int     fd;
    int     ret;
    int     i;
    filht_h_t head;
    uint8_t *zeros;
        
    assert(fn);

    fd = open(fn, O_RDWR | O_CREAT, 0644);
    if (-1 == fd) {
        return -1;
    }

    /* Make a new header. */
    head.magic = FILHT_MAGIC;
    head.size = HTAB_SIZE;
    head.item_no = 0;
    head.del_no = 0;
    head.reserved = 0;
    head.tag = FILHT_NEW;
    ret = write(fd, (void *)&head, sizeof(head));
    if (ret != sizeof(head)) {
        close(fd);
        return -1;
    }

    /* Filled file with zero. */
    zeros = (uint8_t *)malloc(HTAB_SIZE);
    if (!zeros) {
        close(fd);
        return -1;
    }
    for (i = 0; i < sizeof(htab_t); i++) {
        ret = write(fd, (void *)zeros, HTAB_SIZE);
        if (ret != HTAB_SIZE) {
            close(fd);
            return -1;
        }
    }
    close(fd);

    return 0;
}

int filht_undo(char *fn) {
    filht_h_t head;
    int     fd;
    int     i;
    int     ret;
    int     item_no = 0;
    int     del_no = 0;
    htab_t  item;
    struct timeval t0, t1;

    assert(fn);

    gettimeofday(&t0, NULL);
    fd = open(fn, O_RDWR);
    if (-1 == fd) {
        return -1;
    }
    ret = lseek(fd, sizeof(head), SEEK_SET);
    if (-1 == ret) {
        close(fd);
        return -1;
    }
    for (i = 0; i < HTAB_SIZE; i++) {
        ret = read(fd, (void *)&item, sizeof(item));
        if (ret != sizeof(item)) {
            close(fd);
            return -1;
        }
        if (item.tag == HTAB_USED) ++item_no;
        if (item.tag == HTAB_DELETED) ++del_no;
    }

    /* OK, now write back the header. */
    gettimeofday(&t1, NULL);
    head.magic = FILHT_MAGIC;
    head.ts0 = t0.tv_sec;
    head.ts1 = t1.tv_sec;
    head.size = HTAB_SIZE;
    head.item_no = item_no;
    head.del_no = del_no;
    head.reserved = 0;
    head.tag = FILHT_CLOSED;

    ret = lseek(fd, 0, SEEK_SET);
    if (-1 == ret) {
        close(fd);
        return -1;
    }
    ret = write(fd, (void *)&head, sizeof(head));
    if (ret != sizeof(head)) {
        close(fd);
        return -1;
    }

    return 0;
}

int filht_errno() {
    return _errno;
}

void filht_perror(char *msg) {
    if (msg) {
        fprintf(stderr, "%s: ", msg);
    } else {
        fprintf(stderr, ":: ");
    }
    fprintf(stderr, "%s\n", _errmsg[_errno]);
}

void filht_print_header(filht_t *filht) {
    assert(filht);

    switch (filht->head.tag) {
    case FILHT_NEW:
        printf("%5.5s", "NEW");
        break;
    case FILHT_OPENED:
        printf("%5.5s", "OPEN");
        break;
    case FILHT_CLOSED:
        printf("%5.5s", "CLOSE");
        break;
    default:
        printf("%5.5s", "ERR");
        break;
    }

    printf("%12d%12d%12d\n",
           filht->head.size,
           filht->head.item_no,
           filht->head.del_no);
}

void filht_print_title() {
    printf("%5s", "STAT");
    printf("%12s%12s%12s\n",
           "Size",
           "Item #",
           "Del #");
    printf("%5s%12s%12s%12s\n",
           "====",
           "====",
           "======",
           "=====");
}
