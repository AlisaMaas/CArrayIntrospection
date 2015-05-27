#include "structHeader.h"
/**
 * This tests that function declarations and functions without arguments
 * don't cause issues.
 **/
void print(char *string);
void nonsense(){}
int find(struct foo *f) {
	print(f->nonString);
	for (int i = 0;; i++) {
		if (f->string[i] == '\0') {
			print("About to break from loop");
			break;
		}
	}
	nonsense();
	return 1;
}
int foo(struct foo *f) {
	print(f->nonString);
	for (int i = 0;; i++) {
		if (f->nonString[i] != '\0') {
			print("About to break from loop");
			break;
		}
	}
	return find(f);
}
