#include "structHeader.h"
/**
 * This tests that we can't fool it by passing
 * a string literal in as the real null terminated
 * string argument of find and a possible null terminated
 * array as the non-string argument of find. We should find
 * that find's zeroth argument is a null-terminated string, the first
 * argument is not, and neither of foo's arguments is null-terminated.
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
