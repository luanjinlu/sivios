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

#include <stdio.h>
#include <unistd.h>
#include "filht.h"
#include "memfin.h"

#define MAX_LEN 100

static memfin_t mfs[MAX_LEN];

void usage(char *prog) {
    printf("Usage: %s [-b <base path>] [-n <max>] [-h]\n", prog);
    printf("       -b <base path>, the base path where to store fingerprints.\n");
    printf("       -n <max>, the max length of fingerprint.\n");
    printf("       -h, show help info.\n");
    return;
}

int init_filht(char *b) {
    char fn[1024];
    int ret;
    if (!b) return -1;
    sprintf(fn, "%s/filht.tbl\0", b);
    ret = filht_new(fn);
    return ret;
}

int init_mf(int max) {
    int i, ret, level;
    if (max < 1 || max > MAX_LEN) return -1;
    for (i = 0; i < max; i++) {
        switch (i) {
        case 0:
            level = 1;
            break;
        case 1:
            level = 2;
            break;
        default:
            level = 3;
            break;
        }
        ret = memfin_init(&mfs[i], i+1, level);
        if (ret == -1) return -1;
    }
    return 0;
}

int init_mf_files(char *b, int max) {
    int i, ret;
    char fn[1024];
    if (!b) return -1;
    if (max < 1 || max > MAX_LEN) return -1;
    for (i = 0; i < max; i++ ) {
        sprintf(fn, "%s/%03d\0", b, i+1);
        ret = memfin_save(fn, &mfs[i]);
        if (ret == -1) return -1;
    }
    return 0;
}

void free_mf(int max) {
    int i;
    if (max < 1 || max > MAX_LEN) return;
    for (i = 0; i < max; i++)
        memfin_free(&mfs[i]);
    return;
}

int main(int argc, char *argv[]) {
    char base[1024] = "/data/f";
    int num = 100;
    int ret;
    char ch;

    while ((ch = getopt(argc, argv, "b:n:h")) != -1) {
        switch (ch) {
        case 'b':
            sscanf(optarg, "%s", base);
            break;
        case 'n':
            sscanf(optarg, "%d", &num);
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
    ret = init_filht(base);
    if (ret == -1) {
        printf("init filht fail.\n");
        return 1;
    }
    ret = init_mf(num);
    if (ret == -1) {
        printf("init memfin fail.\n");
        return 1;
    }
    ret = init_mf_files(base, num);
    if (ret == -1) {
        printf("init memfin file fail.\n");
        return 1;
    }
    free_mf(num);
    return 0;
}
