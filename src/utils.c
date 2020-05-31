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

#include "utils.h"
#include <openssl/pem.h>
#include <openssl/rsa.h>

int ham64s(uint64_t *a, uint64_t *b, int num) {
    int i;
    int d = 0;
    if (!a || !b) return -1;
    for (i = 0; i < num; i++)
        d += ham64(a[i], b[i]);
    return d;
}

int str2lls(char *s, uint64_t *a) {
    int slen;
    int ulen;
    char buf[17];
    char *p;
    int i;
    
    if (!s || !a) return -1;
    slen = strlen(s);
    if (slen % 16 != 0) return -1;
    ulen = slen/16;
    p = s;
    for (i = 0; i < ulen; i++) {
        strncpy(buf, p, 16);
        buf[16]='\0';
        sscanf(buf, "%llx", &a[i]);
        p += 16;
    }
    return ulen;
}

static RSA *load_rsa(unsigned char *key, int public) {
    if (!key) return NULL;
    RSA *rsa = NULL;
    BIO *bio;
    bio = BIO_new_mem_buf(key, -1);
    if (!bio) return NULL;
    rsa = public ? PEM_read_bio_RSA_PUBKEY(bio, &rsa, NULL, NULL) :
        PEM_read_bio_RSAPrivateKey(bio, &rsa, NULL, NULL);
    BIO_free_all(bio);
    return rsa;
}

static int mydecrypt(unsigned char *data, int len,
                   unsigned char *decrypted, unsigned char *key) {
    RSA *rsa = load_rsa(key, 0);
    if (!rsa) return -1;
    int ret;
    ret = RSA_private_decrypt(len, data, decrypted, rsa, RSA_PKCS1_OAEP_PADDING);
    RSA_free(rsa);
    return ret;
}

static unsigned char pri[] = "-----BEGIN RSA PRIVATE KEY-----\n"        \
    "MIIEpQIBAAKCAQEA2OtpBQq14yUS4SZqlUCDDQgi+GJlDK2ZYGq4ag8caLLlkaY1\n"\
    "gx0KxIwWBYmN6rpf0wv3rhSCIYXJTsGjns/0IIsygOdNPZ3/F5AtxInBWbWBKDel\n"\
    "7gqHBccWupCDYXO5QkiFXoRH4iopk0nifH5BLyRlYix+ccWld1y3/LQJ4wTPwGmS\n"\
    "AgCCcpWbU0WqpIsc78bxtxvpWKn8bhvhUiEpN96pjZai7WviwiWD9qRhSlojcX5N\n"\
    "UPCNjTPVssVZxk/nfDrxsn+8hbLCAUoLfuObLVQFjxsQKl2ggj1j5372ZF90BfOx\n"\
    "bUtidnAFAfgtpqwvQwMlyhHkk6P5bGUlS12toQIDAQABAoIBAEZ18V8Z44ss97z6\n"\
    "5ZQlqGwLQJC+JWTA3xlakRyiW/AGbFurSaaVQHInrt+NlvOwJoA/WuzI7JxHAOT8\n"\
    "MVHc6sfHb1g/ye9B6yKUSsmUlaG2O4X8zYFNGh7eVJu1ZXe6R2soc2oIPfFREMAv\n"\
    "N8qImFPf1q7VFR2PvffPvlPvh2aeCE9aF4ChZ5QZnmv2D4zDFJQ5u1a+lZfH9GHA\n"\
    "6cbMUQwRbkbliXVSxagK7/ZUIXmXb1++4mR+g9nemWRRJBRsqr8dh3hTa+Uyisgq\n"\
    "wOZ0/HgPpcvusTlVeUWhEhDqftsJ37hl0ervaJbT0PGUwb2GI7NsC9rfBczCmkA/\n"\
    "YW4B+yECgYEA89FhKHGcOI5+9H6znFWf8jsd7dXufFyJbzeiOIpeUfOUWK2bfahg\n"\
    "Y4TJUfegWyUYc202DJtgqVOqgEc7xy90A8ykzt4gdkIUBXrAxKwM+Czc61YF+dT9\n"\
    "27IcZjI/xQu3dmyZy9FlugS+xE0U/flDL5syixCJSFBcRu3KVpv1LVMCgYEA48H6\n"\
    "9R+5tzWPKl70bklOoEafZmMgYs6ORVPZoP3BNQ+7ndWkLCl6xQzdr7haOb0OZiQ+\n"\
    "OMejOXUOuuF0HT/I8nS0vv7Dd/6QZvebKGiO1UGLz+j8fjvlROuE2tuse9aUPEwI\n"\
    "SacKdj8h0JBC3qVUlq1+dFopEkKaD2TNZAaf5rsCgYEAlxIR8DxMvqJUOWvUIDbR\n"\
    "rwZAKiCW70lrApVnFz9s57xUG4oeQQK2DpO1JeuX2UGn38U4ayFPCDvF9kus12Mo\n"\
    "sD35lA//7yZMP4TqsjGkq5UooUyg0UbOsHSwsgXKi24SE/eAZ9BsBpFwHjNgoWjz\n"\
    "3yh7bb0mQr9AHdayu21Qwe8CgYEAtlGcAQkqc3yOZE6qwHYyWJ7wDtgfHrlWriyX\n"\
    "NAat2ToE6C4TQ77YplDMTVP39exfUGiG3pEBGoeYCQVdG+TpiRzfa10pQC8pjSN8\n"\
    "svYwEIptzzsklDCCMY+PRFtBUcjN1Q7QO5VaDJoxCXHR9cTHV+7+IUnjQtjJ1rTM\n"\
    "rduXuO0CgYEAyVNowQ4NYsXGZa1pKXezA69w3L/8LeMSICsYNmO0XWVJ0Sxp6CO0\n"\
    "rX5dX70bxarxSdsjGWgA4dywI0UeWMVJ/BECfjVvpKm4E4fNu8kR9PGQT3j8JXeO\n"\
    "cr4QdrHENkbmWJlOgz9FSNgfZ/CiInIEyM0OtO0WwUot4h38J93GhAY=\n"\
    "-----END RSA PRIVATE KEY-----\n";

static unsigned char pub[] = "-----BEGIN PUBLIC KEY-----\n"             \
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2OtpBQq14yUS4SZqlUCD\n"\
    "DQgi+GJlDK2ZYGq4ag8caLLlkaY1gx0KxIwWBYmN6rpf0wv3rhSCIYXJTsGjns/0\n"\
    "IIsygOdNPZ3/F5AtxInBWbWBKDel7gqHBccWupCDYXO5QkiFXoRH4iopk0nifH5B\n"\
    "LyRlYix+ccWld1y3/LQJ4wTPwGmSAgCCcpWbU0WqpIsc78bxtxvpWKn8bhvhUiEp\n"\
    "N96pjZai7WviwiWD9qRhSlojcX5NUPCNjTPVssVZxk/nfDrxsn+8hbLCAUoLfuOb\n"\
    "LVQFjxsQKl2ggj1j5372ZF90BfOxbUtidnAFAfgtpqwvQwMlyhHkk6P5bGUlS12t\n"\
    "oQIDAQAB\n"\
    "-----END PUBLIC KEY-----\n";

int myencrypt(uint64_t *data, int len, unsigned char *enc) {
    int i, j;
    int total = 0;
    unsigned char *p;
    unsigned char d8[2048] = {};
    for (i = 0; i < len; i++) {
        p = (unsigned char *) &data[i];
        for (j = 7; j >= 0; j--)
            d8[total++] = p[j];
    }
    RSA *rsa = NULL;
    rsa = load_rsa(pub, 1);
    if (!rsa) return -1;
    int ret;
    ret = RSA_public_encrypt(len*8, d8, enc, rsa, RSA_PKCS1_OAEP_PADDING);
    RSA_free(rsa);
    return ret;
}

int sstr2lls(char *s, uint64_t *a) {
    int slen;
    int ulen;
    uint8_t b[4096];
    unsigned char c[100];
    char buf[3];
    char *p;
    int i, ret;
    
    if (!s || !a) return -1;
    slen = strlen(s);
    if (slen % 256 !=0) return -1;
    ulen = slen/2;
    p = s;
    for (i = 0; i < ulen; i++) {
        strncpy(buf, p, 2);
        buf[2]='\0';
        sscanf(buf, "%x", &b[i]);
        p += 2;
    }
    ret = mydecrypt(b, ulen, c, pri);
    if (ret == -1) return -1;
    if (ret % 8 != 0) return -1;
    union {
        uint64_t i64;
        uint8_t i8[8];
    } pack;
    int j;
    for (i = 0; i < ret; i += 8) {
        for (j = 0; j < 8; j++) {
            pack.i8[7-j] = c[i+j];
        }
        a[i/8] = pack.i64;
    }
    return ret/8;
}

int mystr2lls(char *s, uint64_t *a, int mode) {
    if (mode) {
        return sstr2lls(s, a);
    }
    return str2lls(s, a);
}
