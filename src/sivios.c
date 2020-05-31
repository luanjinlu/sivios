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

static char revision[] = "";

#include "siviosapi.h"

static char base[1024] = "/data/f";
static int max_fp_len = MAX_FP_LEN;
static int mode = 1;
static int enc = 1;

static void usage(char *prog) {
    printf("Usage (server): %s [-a <addr>] [-p port] [-b <base>] [-n <max>] [-hrv]\n",
           prog);
    printf("       -a <addr>, the host addr binding, default is 0.0.0.0.\n");
    printf("       -p <port>, the host port binding, default is 7000.\n");
    printf("       -b <base>, base path name for fingerprint.\n");
    printf("       -n <max>, the max length of fingerprint.\n");
    printf("       -r, readonly mode.\n");
    printf("       -d, non encryt mode.\n");
    printf("       -h, show help info.\n");
    printf("       -v, show program's version.\n\n");
    printf("Usage (client): curl -s http://localhost:port/[cmd]\n");
    printf("       info?show=all[quota], show capacity info.\n");
    return;
}

int main(int argc, char *argv[]) {
    int port = 7000;
    char addr[16] = "0.0.0.0";
    char ch;
    int ret;

    while((ch = getopt(argc, argv, "a:p:b:n:hrdv")) != -1) {
        switch (ch) {
        case 'a':
            sscanf(optarg, "%s", addr);
            break;
        case 'p':
            sscanf(optarg, "%d", &port);
            break;
        case 'b':
            sscanf(optarg, "%s", base);
            break;
        case 'n':
            sscanf(optarg, "%d", &max_fp_len);
            break;
        case 'r':
            mode = 0;
            break;
        case 'd':
            enc = 0;
            break;
        case 'v':
            printf("%s\n", revision);
            return 0;
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

    ret = siviosapi_start(addr, port, base, max_fp_len, mode, enc);
    return ret;
}
