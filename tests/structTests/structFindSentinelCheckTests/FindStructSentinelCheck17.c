#include "structHeader.h"
/**
 * This check tests we print info about both
 * array arguments when two array arguments are
 * given, but only one sentinel check is found.
 *
 * We expect to find one non-optional sentinel check for string 
 * and 0 for nonString.
 **/
void printc(char c) {}
int find(struct foo* f, int distance) {
	for (int i = 0; i < distance; i++) {
		printc(f->nonString[i]);
		if (f->string[i] == '\0') {
			break;
		}
	}
	return 1;
}

int findArrayAccesses(struct foo* f, int distance) {
	for (int i = 0; i < distance; i++) {
		printc(f[0].nonString[i]);
		if (f[0].string[i] == '\0') {
			break;
		}
	}
	return 1;
}

int findIndirectArrayAccesses(struct foo* f, int distance) {
	for (int i = 0; i < distance; i++) {
		printc(f[0].foo->nonString[i]);
		if (f[0].foo->string[i] == '\0') {
			break;
		}
	}
	return 1;
}

