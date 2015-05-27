#include "structHeader.h"
/**
 * This check tests we print info about both
 * array arguments when two array arguments are
 * given, and both have the same sentinel check
 *
 * We expect to find one non-optional sentinel check for f 
 * and 1 for f2.
 **/
void printc(char c) {}
int find(struct foo *f, struct foo *f2, int distance) {
	for (int i = 0; i < distance; i++) {
		if (f->string[i] == '\0' || f2->string[i] == '\0') {
			break;
		}
	}
	return 1;
}

int findArrayAccesses(struct foo *f, struct foo *f2, int distance) {
	for (int i = 0; i < distance; i++) {
		if (f[0].string[i] == '\0' || f2->string[i] == '\0') {
			break;
		}
	}
	return 1;
}

int findIndirectArrayAccesses(struct foo *f, struct foo *f2, int distance) {
	for (int i = 0; i < distance; i++) {
		if (f[0].foo->string[i] == '\0' || f2[0].foo->string[i] == '\0') {
			break;
		}
	}
	return 1;
}
