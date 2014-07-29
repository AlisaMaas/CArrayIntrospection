/**
* This check tests we print info about both
* array arguments when two array arguments are
* given, and both have different sentinel checks.
* One check is optional, one is not.
*
* We expect to find one non-optional sentinel check for string 
* and 1 optional check for string2.
**/
void printc(char c){}
int find(char string[], char string2[], int distance, int flag)
{
  for(int i = 0; i < distance; i++){
		if(string[i] == '\0'){
			break;
		}
		if(flag)
			if(string2[i] == '\0')
				break;
	}
  return 1;
}
