#include "structHeader.h"
/**
 * This check tests we print info about both
 * array arguments when two array arguments are
 * given, and both have different sentinel checks.
 * One check is optional, one is not.
 *
 * We expect to find one non-optional sentinel check for f 
 * and 1 optional check for b.
 **/
void printc(char c) {}
int find(struct foo *f, struct bar *b, int distance, int flag) {
	for (int i = 0; i < distance; i++) {
		if (f->string[i] == '\0') {
			break;
		}
		if (flag)
			if (b->nonString[i] == '\0')
				break;
	}
	return 1;
}

int findArrayAccesses(struct foo *f, struct bar *b, int distance, int flag) {
	for (int i = 0; i < distance; i++) {
		if (f[0].string[i] == '\0') {
			break;
		}
		if (flag)
			if (b[0].nonString[i] == '\0')
				break;
	}
	return 1;
}

int findIndirectArrayAccesses(struct foo *f, struct bar *b, int distance, int flag) {
	for (int i = 0; i < distance; i++) {
		if (f[0].foo->string[i] == '\0') {
			break;
		}
		if (flag)
			if (b[0].z->nonString[i] == '\0')
				break;
	}
	return 1;
}