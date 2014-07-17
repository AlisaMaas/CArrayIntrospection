/**
* This check tests we detect an optional sentinel check 
* in a loop where there is another read from the array.
*
* We expect to find one optional sentinel check.
**/
void print(char* string){}
int find(char string[], char goal, int flag)
{
  for(int i = 0; string[i] != goal; i++){
		if(flag)
			if(string[i] == '\0'){
				break;
				}
	}
  return 1;
}
