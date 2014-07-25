/**
* This check tests we detect a non-optional sentinel check 
* in a loop where there is another read from the array.
*
* We expect to find one non-optional sentinel check.
**/
void print(char* string){}
void foo(int distance){
	for(int i = 0; i < distance; i++){
		print(i);
	}
}
int find(char string[], char goal)
{
  for(int i = 0; string[i] != goal; i++){
		if(string[i] == '\0'){
			break;
			}
	}
  return 1;
}
