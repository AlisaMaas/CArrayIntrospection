#include "structHeader.h"
/**
 * This is a simple interproceedural test that ensures with three functions,
 * it recognizes that foo calls the null-terminated print though it contains no
 * null checks. The results should note that f->string is null terminated
 **/
int find(struct foo *f);
void print(char *string) {}
int foo(struct foo *f) {
	for (int i = 0;; i++) {
		if (f->string[i] != '\0') {
			print("About to break from loop");
			break;
		}
	}
	return find(f);
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
