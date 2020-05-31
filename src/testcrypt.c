#include <stdint.h>
#include "utils.h"

int main(int argc, char *argv[]) {
  unsigned char enc[2048];
  uint64_t data;
  int ret, i;
  if (argc != 2) {
    printf("Usage: %s <int64>\n", argv[0]);
    return 1;
  }
  sscanf(argv[1], "%lld", &data);
  ret = myencrypt(&data, 1, enc);
  if (ret == -1) {
    printf("error.\n");
    return 1;
  }
  for (i = 0; i < ret; i++)
    printf("%02x", enc[i]);
  printf("\n");
  return 0;
}
