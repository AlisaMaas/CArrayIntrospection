/**
 * This tests that the argument gets correctly matched to the parameter.
 * It does this by having two array arguments each in foo and find, where the 
 * arguments are in opposite order in the argument list in each one. Each has one
 * null-terminated and one non-null-terminated array argument.
 **/
void print(char *string) {}
int find(char string[], char nonString[]) {
	print(nonString);
	for (int i = 0;; i++) {
		if (nonString[i] != '\0') {
			print("About to break from loop");
			break;
		}
	}
	for (int i = 0;; i++) {
		if (string[i] == '\0') {
			print("About to break from loop");
			break;
		}
	}
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
