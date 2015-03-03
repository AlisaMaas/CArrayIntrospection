/*
* Tests a broken loop where *x is incorrectly
* tested against the length instead of properly checking
* that the counter is less than the length.
*
* x should not have a length.
*
* Example contributed by Peter Ohmann.
*/

int brokenFind(int* x, int y, int me){
  int i = 0;
  while(*x != y){
    if(*x == me)
      return i;
    i++, x++;
  }
  return -1;
}