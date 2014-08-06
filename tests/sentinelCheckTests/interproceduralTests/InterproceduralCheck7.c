/**
* This tests that the case in which the same formal argument
* is used as multiple arguments. It should find that string in foo
* and in find is a null-terminated string.
**/
void print(char* string){}
int find(char nonString[], char string[])
{
  print(nonString);
	for(int i = 0; ; i++){
		if(nonString[i] != '\0'){
            print("About to break from loop");
			break;
			}
	}
  for(int i = 0; ; i++){
		if(string[i] == '\0'){
            print("About to break from loop");
			break;
			}
	}
  return 1;
}
int foo(char nonString[], char string[])
{
  print(nonString);
  for(int i = 0; ; i++){
		if(nonString[i] != '\0'){
            print("About to break from loop");
			break;
			}
	}
	return find(string, string);
}
