
#include "squirrel_config.h"
#include "squirrel_test.h"
#include <crtdbg.h>


void out(const char* buf, size_t len) {
  printf(buf);
}

int main(int argc, char* argv[]) {
  _CrtMemState s1,s2,s3;
  int r;

  if(argc == 2) {
    return RUN_TEST_BY_CATALOG(argv[1], &out);
  } else if(argc >= 3 ) {
    char name[512];
    strcpy(name, argv[1]);
    strcat(name, "_");
    strcat(name, argv[2]);
    return RUN_TEST_BY_NAME(name, &out);
  }
  RUN_ALL_TESTS(&out);
  _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE );
  _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT );
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE );
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT );


  _CrtMemCheckpoint( &s1 );
  malloc(3);
  r = RUN_ALL_TESTS(&out);
  _CrtMemCheckpoint( &s2 );

  if ( _CrtMemDifference( &s3, &s1, &s2) ) {
    _CrtMemDumpStatistics( &s3 );
  }

  return r;
}