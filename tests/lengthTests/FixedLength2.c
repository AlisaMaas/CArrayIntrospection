#include <stdlib.h>

int bar() {
   return 4*rand()%30;
}

/**
* Check that randomness is properly treated as random.
**/
int foo(int* array, int x){
	int index = 10;
	int sum = 0;
	for (int i = 0; i < index; i++) {
		sum += array[i];
	}
	array[bar()] = 5;
	return sum;
	
}

