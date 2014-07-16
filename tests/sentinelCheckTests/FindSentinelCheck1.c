void print(char* string){}
int find(char string[])
{
  for(int i = 0; ; i++){
		if(string[i] == '\0'){
            print("About to break from loop");
			break;
			}
	}
  return 1;
}
