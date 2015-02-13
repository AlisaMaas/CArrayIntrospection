/**
* This tests that non-induction variables are ignored
* (for now - this test may be later used to make sure they aren't!)
**/
int foo(int* array){
	int sum = 0;
	for(int i = 0, j = 0;  40 > i && j < 50;  i++, j++){
		sum += array[i + 4];
		sum += array[j];
	}
	return sum;
	}
		