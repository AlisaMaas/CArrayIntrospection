#include <stdio.h>
/**
* This tests that a simple loop with an updated pointer is corrected as expected.
**/
int find2(int* x, int y, int me){
  int i = 0;
  for(; y > 0; x++, y--, i++){
    if(*x == me)
      return i;
  }
  return -1;
}