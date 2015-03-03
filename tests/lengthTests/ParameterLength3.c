/*
* Tests a loop where one argument is length and
* one is an element to find. The length of x 
* should be y. 
*
* Example contributed by Peter Ohmann.
*/
int find(int* x, int y, int me){
  int i;
  for(i = 0; i < y; i++){
    if(x[i] == me)
      return i;
  }
  return -1;
}