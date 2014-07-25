/**
* This check tests we detect no sentinel checks in a loop with
* a "backwards" sentinel check (breaking if not null)
*
* We expect to find no sentinel check.
**/
void print(char* string){}
int find(char string[], int distance)
{
  for(int i = 0; i < distance; i++){
		if(string[i] != '\0'){
			break;
		}
	}
  return 1;
}
