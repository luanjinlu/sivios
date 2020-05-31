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
#include <stdlib.h>
#include <string.h>
#include "memfin.h"

static int levels[7] = {FP_MAX1,FP_MAX2,FP_MAX3,FP_MAX4,FP_MAX5,FP_MAX6,FP_MAX7};

int memfin_init(memfin_t *mf, int len, int level) {
    if (!mf || len <= 0) return -1;
    if (level < 1 || level > 7) return -1;

    mf->original = 0;
    mf->num = 0;
    mf->len = len;
    mf->modified = 0;
    int max = levels[level-1];
    mf->max = max;
    size_t sizeofval = len * sizeof(uint64_t);
    size_t sizeofkey = sizeof(uint64_t);
    size_t sizeoftag = sizeof(uint8_t);
    mf->val = calloc(max, sizeofval);
    if (!mf->val) return -1;
    mf->key = calloc(max, sizeofkey);
    if (!mf->key) {
        free(mf->val);
        return -1;
    }
    mf->tag = calloc(max, sizeoftag);
    if (!mf->tag) {
        free(mf->val);
        free(mf->key);
        return -1;
    }
    return 0;
}

void memfin_free(memfin_t *mf) {
    if (!mf) return;
    if (!mf->val || !mf->key || !mf->tag) return;
    free(mf->val);
    free(mf->key);
    free(mf->tag);
    mf->val = NULL;
    mf->key = NULL;
    mf->tag = NULL;
    return;
}

int memfin_add(memfin_t *mf, int len, uint64_t key, uint64_t *val) {
    if (!mf || !val || len <= 0) return -1;
    if (len != mf->len) return -1;
    if (mf->num >= mf->max) return -1;
    mf->tag[mf->num] = 0;
    mf->key[mf->num] = key;
    memcpy(&(mf->val[mf->num * mf->len]), val, len*sizeof(uint64_t));
    mf->num++;
    mf->modified = 1;
    return 0;
}

int memfin_load_readonly(char *fn, memfin_t *mf) {
    if (!fn || !mf) return -1;
    char fnval[1024], fnkey[1024], fntag[1024];
    sprintf(fnval, "%s.val", fn);
    sprintf(fnkey, "%s.key", fn);
    sprintf(fntag, "%s.tag", fn);
    FILE *fval;
    FILE *fkey;
    FILE *ftag;
    fval = fopen(fnval, "r");
    if (!fval) return -1;
    fkey = fopen(fnkey, "r");
    if (!fkey) return -1;
    ftag = fopen(fntag, "r");
    if (!ftag) return -1;
    int len;
    fread(&len, sizeof(uint32_t), 1, fval);
    mf->len = len;
    mf->max = FP_MAX1;
    long filesize;
    fseek(fval, 0, SEEK_END);
    filesize = ftell(fval);
    size_t sizeofval = len * sizeof(uint64_t);
    size_t num = (filesize - sizeof(uint32_t))/sizeofval;
    mf->original = num;
    mf->num = num;
    if (num == 0) {
        mf->val = NULL;
        mf->key = NULL;
        mf->tag = NULL;
        fclose(fval);
        fclose(fkey);
        fclose(ftag);
        return 0;
    }
    mf->val = calloc(num, sizeofval);
    if (!(mf->val)) {
        fclose(fval);
        fclose(fkey);
        fclose(ftag);
        return -1;
    }
    mf->key = calloc(num, sizeof(uint64_t));
    if (!(mf->key)) {
        fclose(fval);
        fclose(fkey);
        fclose(ftag);
        free(mf->val);
        return -1;
    }
    mf->tag = calloc(num, sizeof(uint8_t));
    if (!(mf->tag)) {
        fclose(fval);
        fclose(fkey);
        fclose(ftag);
        free(mf->val);
        free(mf->key);
        return -1;
    }
    fseek(fval, sizeof(uint32_t), SEEK_SET);
    fread(mf->val, sizeofval, num, fval);
    fread(mf->key, sizeof(uint64_t), num, fkey);
    fread(mf->tag, sizeof(uint8_t), num, ftag);
    fclose(fval);
    fclose(fkey);
    fclose(ftag);
    return 0;
}

int memfin_load(char *fn, memfin_t *mf) {
    if (!fn || !mf) return -1;
    char fnval[1024], fnkey[1024], fntag[1024];
    sprintf(fnval, "%s.val", fn);
    sprintf(fnkey, "%s.key", fn);
    sprintf(fntag, "%s.tag", fn);
    FILE *fval;
    FILE *fkey;
    FILE *ftag;
    fval = fopen(fnval, "r");
    if (!fval) return -1;
    fkey = fopen(fnkey, "r");
    if (!fkey) return -1;
    ftag = fopen(fntag, "r");
    if (!ftag) return -1;
    int len;
    fread(&len, sizeof(uint32_t), 1, fval);
    if (len != mf->len) {
        fclose(fval);
        fclose(fkey);
        fclose(ftag);
        return -1;
    }
    long filesize;
    fseek(fval, 0, SEEK_END);
    filesize = ftell(fval);
    size_t sizeofval = len * sizeof(uint64_t);
    size_t num = (filesize - sizeof(uint32_t))/sizeofval;
    if (num > mf->max) {
        fclose(fval);
        fclose(fkey);
        fclose(ftag);
        return -1;
    }
    fseek(fval, sizeof(uint32_t), SEEK_SET);
    fread(mf->val, sizeofval, num, fval);
    fread(mf->key, sizeof(uint64_t), num, fkey);
    fread(mf->tag, sizeof(uint8_t), num, ftag);
    fclose(fval);
    fclose(fkey);
    fclose(ftag);
    mf->original = num;
    mf->num = num;
    return 0;
}

int memfin_save(char *fn, memfin_t *mf) {
    if (!fn || !mf) return -1;
    char fnval[1024], fnkey[1024], fntag[1024];
    sprintf(fnval, "%s.val", fn);
    sprintf(fnkey, "%s.key", fn);
    sprintf(fntag, "%s.tag", fn);
    FILE *fval;
    FILE *fkey;
    FILE *ftag;
    fval = fopen(fnval, "a");
    if (!fval) return -1;
    fkey = fopen(fnkey, "a");
    if (!fkey) return -1;
    ftag = fopen(fntag, "a");
    if (!ftag) return -1;
    size_t sizeofval = mf->len * sizeof(uint64_t);
    size_t sizeofkey = sizeof(uint64_t);
    size_t sizeoftag = sizeof(uint8_t);
    fwrite(&(mf->len), sizeof(uint32_t), 1, fval);
    fwrite(mf->val, sizeofval, mf->num, fval);
    fwrite(mf->key, sizeofkey, mf->num, fkey);
    fwrite(mf->tag, sizeoftag, mf->num, ftag);
    fclose(fkey);
    fclose(fval);
    fclose(ftag);
    mf->original = mf->num;
    return 0;
}

int memfin_append(char *fn, memfin_t *mf) {
    if (!fn || !mf) return -1;
    if (mf->modified == 0) return 0;
    char fnval[1024], fnkey[1024], fntag[1024];
    sprintf(fnval, "%s.val", fn);
    sprintf(fnkey, "%s.key", fn);
    sprintf(fntag, "%s.tag", fn);
    FILE *fval;
    FILE *fkey;
    FILE *ftag;
    fval = fopen(fnval, "a");
    if (!fval) return -1;
    fkey = fopen(fnkey, "a");
    if (!fkey) return -1;
    ftag = fopen(fntag, "w");
    if (!ftag) return -1;
    size_t sizeofval = mf->len * sizeof(uint64_t);
    size_t sizeofkey = sizeof(uint64_t);
    size_t sizeoftag = sizeof(uint8_t);
    fwrite(&(mf->key[mf->original]), sizeofkey,
           mf->num - mf->original, fkey);
    fwrite(&(mf->val[(mf->original)*mf->len]), sizeofval,
           mf->num - mf->original, fval);
    fwrite(mf->tag, sizeoftag, mf->num, ftag);
    fclose(fkey);
    fclose(fval);
    fclose(ftag);
    mf->original = mf->num;
    return 0;
}
