#include "structHeader.h"

/**
 * This check tests we detect a non-optional sentinel check 
 * in a loop from a struct element where there is another 
 * read from the array.
 *
 * We expect to find one non-optional sentinel check.
 **/
void print(int value) {}
int find(struct foo *f, char goal) {
	for (int i = 0; f->string[i] != goal; i++) {
		if (f->string[i] == '\0') {
			break;
		}
	}
	return 1;
}

int findArrayAccess(struct foo *f, char goal) {
	for (int i = 0; f[0].string[i] != goal; i++) {
		if (f[0].string[i] == '\0') {
			break;
		}
	}
	return 1;
}

int findIndirectArrayAccess(struct foo *f, char goal) {
	for (int i = 0; f->foo[0].string[i] != goal; i++) {
		if (f->foo[0].string[i] == '\0') {
			break;
		}
	}
	return 1;
}