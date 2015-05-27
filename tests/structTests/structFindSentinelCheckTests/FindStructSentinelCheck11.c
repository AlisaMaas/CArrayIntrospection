#include "structHeader.h"
/**
 * This check tests we detect a non-optional sentinel check 
 * in a loop from a struct element where there are no other reads 
 * from the array and the sentinel check is done using a stored result from
 * the array.
 *
 * We expect to find one non-optional sentinel check.
 **/
void print(char *string) {}
int find(struct foo *f, int distance) {
	for (int i = 0; i < distance; i++) {
		char element = f->string[i];
		print("blah, blah");

		if (element == '\0') {
			break;
		}
	}
	return 1;
}

int findArrayAccess(struct foo *f, int distance) {
	for (int i = 0; i < distance; i++) {
		char element = f[0].string[i];
		print("blah, blah");

		if (element == '\0') {
			break;
		}
	}
	return 1;
}
int findIndirectArrayAccess(struct foo *f, int distance) {
	for (int i = 0; i < distance; i++) {
		char element = f->foo[0].string[i];
		print("blah, blah");

		if (element == '\0') {
			break;
		}
	}
	return 1;
}