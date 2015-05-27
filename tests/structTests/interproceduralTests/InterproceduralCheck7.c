#include "structHeader.h"
/**
 * This tests that the case in which the same formal argument
 * is used as multiple arguments. It should find that string is non-null terminated.
 **/
void print(char *string) {}
int find(struct foo *f) {
	print(f->nonString);
	for (int i = 0;; i++) {
		if (f->nonString[i] != '\0') {
			print("About to break from loop");
			break;
		}
	}
	for (int i = 0;; i++) {
		if (f->string[i] == '\0') {
			print("About to break from loop");
			break;
		}
	}
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
