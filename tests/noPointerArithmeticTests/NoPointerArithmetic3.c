#include <stdio.h>
/**
* This tests that a simple loop with an updated pointer is corrected as expected.
**/
int find2(int* x, int y, int me){
  int *end = x + y;
  for(; x != end; x++){
    if(*x == me)
      return me;
  }
  return -1;
}