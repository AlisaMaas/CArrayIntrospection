#include "structHeader.h"
/**
 * This check tests we detect a non-optional sentinel check 
 * in a loop where there is also an optional sentinel check.
 *
 * We expect to find one non-optional sentinel check and one
 * optional sentinel check.
 **/
void print(char *string) {}
int find(struct foo *f, int distance) {
	for (int i = 0; f->string[i] != '\0'; i++) {
		if (distance == 8)
			if (f->string[i] == '\0') {
				break;
			}
	}
	return 1;
}

int findArrayAccesses(struct foo *f, int distance) {
	for (int i = 0; f[0].string[i] != '\0'; i++) {
		if (distance == 8)
			if (f[0].string[i] == '\0') {
				break;
			}
	}
	return 1;
}

int findIndirectArrayAccesses(struct foo *f, int distance) {
	for (int i = 0; f[0].foo->string[i] != '\0'; i++) {
		if (distance == 8)
			if (f[0].foo->string[i] == '\0') {
				break;
			}
	}
	return 1;
}
