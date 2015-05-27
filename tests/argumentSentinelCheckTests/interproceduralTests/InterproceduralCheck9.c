/**
 * This tests that the case in which the second
 * of several calls determines that the argument
 * is a null-terminated array. foo and find's formal
 * parameters "string" should be null-terminated.
 **/
void print(char *string) {}
int find(char nonString[], char string[]) {
	print(nonString);
	for (int i = 0; i < 10; i++) {
		if (i == 4) {
			if (string[i] == '\0') {
				break;
			}
		}
	}
	return 1;
}
int foo(char nonString[], char string[]) {
	return find("hello", string);
}
