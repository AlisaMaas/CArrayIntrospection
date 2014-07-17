/**
* This check tests we detect a non-optional sentinel check 
* in a loop where there are no other reads from the array.
*
* We expect to find one non-optional sentinel check.
* @Duplicate?
**/
void print(char* string){}
int find(char string[], int distance)
{
  for(int i = 0; i < distance; i++){
		if(string[i] == '\0'){
			break;
			}
	}
  return 1;
}
