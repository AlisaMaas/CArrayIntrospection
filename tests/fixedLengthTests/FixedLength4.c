/**
* This tests the range analysis results when there are conditionals in place.
* May be used at some future point when we have support for predicated types.
**/
int foo(int* array, int x){
	int index = 0;
	if (x > 3) {
		index = 8;
	}
	else {
		index = 2; 
	}
	int sum = 0;
	for (int i = 0; i < index; i++) {
		sum += array[i];
	}
	return sum;
	
}