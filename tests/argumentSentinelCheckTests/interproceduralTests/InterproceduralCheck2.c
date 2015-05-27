/**
 * This is a simple interproceedural test that ensures with three functions,
 * it recognizes that foo is not null terminated, but find is.
 *
 **/
void print(char *string) {}
int foo(char string[]) {
	for (int i = 0;; i++) {
		if (string[i] != '\0') {
			print("About to break from loop");
			break;
		}
	}
	return 0;
}
int find(char string[]) {
	for (int i = 0;; i++) {
		if (string[i] == '\0') {
			print("About to break from loop");
			break;
		}
	}
	return 1;
}
