/**
* This tests traditional parameter-length.
**/
int foo(int* array, int x){
	int sum = 0;
	for(int i = 0;  x > i;  i++){
	    sum += array[4];
		sum += array[i];
	}
	return sum;
}
