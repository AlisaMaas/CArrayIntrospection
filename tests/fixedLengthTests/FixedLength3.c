/**
* This tests that variables indexed both by constants and by variables are not considered fixed length.
**/
int foo(int* array, int x){
	int sum = 0;
	for(int i = 0;  40 > i;  i++){
	    sum += array[4];
	    sum += array[x];
	}
	return sum;
}
		
