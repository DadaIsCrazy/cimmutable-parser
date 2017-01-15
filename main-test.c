#include <stdio.h>
#include "parser.h"

int main (int argc, char* argv[]) {

  char* test_file;
  test_file = argc > 1 ? argv[1] : "test.bench";
  
  Prog* prog = read_file(test_file);
  debug_print(prog);
  
  return 0;
}
