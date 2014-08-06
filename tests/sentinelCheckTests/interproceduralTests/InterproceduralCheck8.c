/**
* This tests that the case in which the second
* of several calls determines that the argument
* is a null-terminated array. foo and find's formal
* parameters "string" should be null-terminated.
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
	find(string, "hello");
	return find("hello", string);
}
