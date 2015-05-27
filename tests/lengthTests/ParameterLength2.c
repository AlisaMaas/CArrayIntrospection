/**
* This tests traditional what happens if two parameters are the length.
**/
int foo(int* array, int x, int y){
	int sum = 0;
	for(int i = 0;  x > i;  i++){
	    sum += array[i];
	}
	for(int i = 0;  y > i;  i++){
	    sum += array[i];
	}
	return sum;
}
