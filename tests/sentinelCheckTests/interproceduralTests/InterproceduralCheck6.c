/**
 * This tests that we can't fool it by passing
 * a string literal in as the real null terminated
 * string argument of find and a possible null terminated
 * array as the non-string argument of find. We should find
 * that find's zeroth argument is a null-terminated string, the first
 * argument is not, and neither of foo's arguments is null-terminated.
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
	return find("hello", string);
}
