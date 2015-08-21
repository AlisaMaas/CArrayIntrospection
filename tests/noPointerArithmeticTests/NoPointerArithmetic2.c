#include <stdio.h>
/**
* This tests that a simple loop with an indexed pointer is unchanged.
**/
int find2(int* x, int y, int me){
  for(int i = 0; i < y; i++){
    if(x[i] == me)
      return i;
  }
  return -1;
}