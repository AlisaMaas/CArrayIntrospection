/**
* This check tests we detect a non-optional sentinel check 
* in a loop where there are no other reads from the array,
* but there is one statement between the sentinel check
* and exiting the loop.
*
* We expect to find one non-optional sentinel check.
**/
void print(char* string){}
int find(char string[], int distance)
{
  for(int i = 0; i < distance; i++){
		if(string[i] == '\0'){
			print("About to break from loop");
			break;
		}
	}
  return 1;
}
