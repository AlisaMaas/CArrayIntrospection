void print(char* string){}
int find(char string[], char goal)
{
  for(int i = 0; string[i] != goal; i++){
		if(string[i] == '\0'){
			break;
			}
	}
  return 1;
}
