#include <stdio.h>
/**
* This tests that constants are properly propagated and
* that the upper bound can be grabbed in a simple loop
* with no other conditionals.
**/
int foo(int* array){
	int sum = 0;
	for(int i = 0, j = 0;  40 > i;  i++){
		sum += array[i + 4];
		sum += array[10];
		sum += array[i];
		int blah = 30;
		sum += array[blah];
	}
	return sum;
	}