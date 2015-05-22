#include "structHeader.h"
/**
 * This tests that the case in which the second
 * of several calls determines that the argument
 * is a null-terminated array. "string" should be null-terminated.
 **/
void print(char *string) {}
int find(struct foo *f) {
	print(f->nonString);
	for (int i = 0; i < 10; i++) {
		if (i == 4) {
			if (f->string[i] == '\0') {
				break;
			}
		}
	}
	return 1;
}
int foo(struct foo *f) {
	return find(f);
}
