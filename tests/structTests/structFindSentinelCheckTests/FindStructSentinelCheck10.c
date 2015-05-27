#include "structHeader.h"
/**
 * This check tests we detect a non-optional sentinel check 
 * in a loop in a struct element where there are no other 
 * reads from the array, but there is one statement between 
 * the sentinel check and exiting the loop.
 *
 * We expect to find one non-optional sentinel check per function.
 **/
void print(char *string) {}
int find(struct foo *f, int distance) {
	for (int i = 0; i < distance; i++) {
		if (f->string[i] == '\0') {
			print("About to break from loop");
			break;
		}
	}
	return 1;
}

int findArrayAccess(struct foo *f, int distance) {
	for (int i = 0; i < distance; i++) {
		if (f[0].string[i] == '\0') {
			print("About to break from loop");
			break;
		}
	}
	return 1;
}

int findIndirectArrayAccess(struct foo *f, int distance) {
	for (int i = 0; i < distance; i++) {
		if (f->foo[0].string[i] == '\0') {
			print("About to break from loop");
			break;
		}
	}
	return 1;
}
