#include "structHeader.h"
/**
 * This simple test should recognize that functions a-e and 
 * find all contain the null-terminated "string".
 **/
void print(char *string) {}
int find(struct foo *f) {
	for (int i = 0;; i++) {
		if (f->string[i] == '\0') {
			print("About to break from loop");
			break;
		}
	}
	return 1;
}
int e(struct foo *f) {
	return find(f);
}
int d(struct foo *f) {
	return e(f);
}
int c(struct foo *f) {
	return d(f);
}
int b(struct foo *f) {
	return c(f);
}
int a(struct foo *f) {
	return b(f);
}
