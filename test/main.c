#include "squirrel_test.h"


void out(const char* buf, size_t len) {
  printf(buf);
}

int main(int argc, char* argv[]) {
  if(argc == 2) {
    RUN_TEST_BY_CATALOG(argv[1], &out);
    return;
  } else if(argc >= 3 ) {
    char name[512];
    strcpy(name, argv[1]);
    strcat(name, "_");
    strcat(name, argv[2]);
    RUN_TEST_BY_NAME(name, &out);
    return;
  }
  RUN_ALL_TESTS(&out);
  RUN_ALL_TESTS(&out);
  return 0;
}