#include "structHeader.h"

void print(char *string) {}
int foo(struct foo *f) {
	for (int i = 0;; i++) {
		if (f->nonString[i] != '\0') {
			print("About to break from loop");
			break;
		}
	}
	return 0;
}
int find(struct foo *f) {
	for (int i = 0;; i++) {
		if (f->string[i] == '\0') {
			print("About to break from loop");
			break;
		}
	}
	return 1;
}
