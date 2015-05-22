#include "structHeader.h"
/**
 * This tests that the case in which the second
 * of several calls determines that the argument
 * is a null-terminated array. foo and find's formal
 * parameters "string" should be null-terminated.
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
