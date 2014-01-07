#include "test.h"


void out(const char* buf, size_t len) {
  printf(buf);
}

int main() {
  RUN_ALL_TESTS(&out);
  RUN_ALL_TESTS(&out);
  return 0;
}