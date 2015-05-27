#include <stdio.h>

/*
* Tests a loop where one argument is length and
* one is an element to find. The length of x 
* should be y. More tricky than #3 because
* the length check is in a call to an 
* external function.
*
* Example contributed by Peter Ohmann.
*/
void print(char* x, int y){
  int i;
  for(i = 0; i < y; i++)
    printf("%c", x[i]);
}