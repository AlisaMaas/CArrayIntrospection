/*
* Tests a loop where one argument is length and
* one is an element to find. The length of x 
* should be y. More tricky than #3 because
* the array is incremented along the way.
*
* Example contributed by Peter Ohmann.
*/
int find(int* x, int y, int me){
  int i = 0;
  for(; y > 0; x++, y--, i++){
    if(*x == me)
      return i;
  }
  return -1;
}