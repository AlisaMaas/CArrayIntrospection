#include "structHeader.h"
/**
 * This tests that the element gets correctly matched to the parameter if traced through a call.
 * It does this by having two struct elements each accessed in foo and find, where the 
 * element are in opposite order in the argument list in each one. Each has one
 * null-terminated and one non-null-terminated struct element.
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
