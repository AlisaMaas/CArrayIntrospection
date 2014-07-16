void print(char* string);
int find(char string[], int distance)
{
  for(int i = 0; i < distance; i++){
		if(string[i] == '\0'){
			for(int j = 0; j < i; j++){
				print("Hello");
			}
			break;
		}
	}
  return 1;
}
