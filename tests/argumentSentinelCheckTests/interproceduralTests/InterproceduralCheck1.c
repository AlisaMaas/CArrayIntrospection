/**
 * This is a simple interproceedural test that ensures with three functions,
 * it recognizes that foo calls the null-terminated print though it contains no
 * null checks. The results should note that string is null terminated in both foo
 * and find, but not print.
 **/
int find(char[]);
void print(char *string) {}
int foo(char string[]) {
	for (int i = 0;; i++) {
		if (string[i] != '\0') {
			print("About to break from loop");
			break;
		}
	}
	return find(string);
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

int bar(char* a, int n) {
  for (int i = 0; i < n; i++) {
    print(&a[i]);
  }
  return 1;
}