#include "structHeader.h"
/**
 * Basic tests that sentinel checks for
 * struct accesses can be found in a variety of types
 * of accesses.
 *
 * We expect to find one non-optional sentinel check
 * per function.
 **/
void print(char *string) {}
int foo(struct bar * b) {
	for (int i = 0;; i++) {
		if (b->string[i] == '\0') {
			print("About to break from loop");
			break;
		}
	}
	return 1;
}
int find(struct foo* f) {
	for (int i = 0;; i++) {
		if (f->string[i] == '\0') {
			print("About to break from loop");
			break;
		}
	}
	return 1;
}

int findArrayAccess(struct foo* f) {
	for (int i = 0;; i++) {
		if (f[0].string[i] == '\0') {
			print("About to break from loop");
			break;
		}
	}
	return 1;
}

int findIndirectArrayAccess(struct foo* f) {
	for (int i = 0;; i++) {
		if (f[0].foo->string[i] == '\0') {
			print("About to break from loop");
			break;
		}
	}
	return 1;
}