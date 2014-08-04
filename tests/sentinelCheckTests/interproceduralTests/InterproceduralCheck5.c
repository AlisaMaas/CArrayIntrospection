/**
* Makes sure that if find calls foo with a null-terminated argument,
* the analysis still reports that find's argument is null-terminated even though
* it is being used as a non-null terminated string in foo.
**/
void print(char* string){}
int foo(char string[])
{
  for(int i = 0; ; i++){
		if(string[i] != '\0'){
            print("About to break from loop");
			break;
			}
	}
	return 0;
}
int find(char string[])
{
  for(int i = 0; ; i++){
		if(string[i] == '\0'){
            print("About to break from loop");
			break;
			}
	}
  return foo(string);
}
