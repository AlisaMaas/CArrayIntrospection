/**
 * This tests that function declarations and functions without arguments
 * don't cause issues.
 **/
void print(char *string);
void nonsense(){}
int find(char string[], char nonString[]) {
	print(nonString);
	for (int i = 0;; i++) {
		if (string[i] == '\0') {
			print("About to break from loop");
			break;
		}
	}
	nonsense();
	return 1;
}
int foo(char nonString[], char string[]) {
	print(nonString);
	for (int i = 0;; i++) {
		if (nonString[i] != '\0') {
			print("About to break from loop");
			break;
		}
	}
	return find(string, nonString);
}
