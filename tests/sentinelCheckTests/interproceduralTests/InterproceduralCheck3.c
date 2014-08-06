/**
* This simple test should recognize that functions a-e and 
* find all contain the null-terminated parameter "string".
**/
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
int e(char string[])
{
  return find(string);
}
int d(char string[])
{
  return e(string);
}
int c(char string[])
{
  return d(string);
}
int b(char string[])
{
  return c(string);
}
int a(char string[])
{
  return b(string);
}
