#include "structHeader.h"
/**
 * This check tests we detect no sentinel checks in a loop with
 * a "backwards" sentinel check (breaking if not null)
 *
 * We expect to find no sentinel check.
 **/
void print(char *string) {}
int find(struct foo *f, int distance) {
	for (int i = 0; i < distance; i++) {
		if (f->nonString[i] != '\0') {
			break;
		}
	}
	return 1;
}

int findArrayAccesses(struct foo *f, int distance) {
	for (int i = 0; i < distance; i++) {
		if (f[0].nonString[i] != '\0') {
			break;
		}
	}
	return 1;
}

int findIndirectArrayAccesses(struct foo *f, int distance) {
	for (int i = 0; i < distance; i++) {
		if (f[0].foo->nonString[i] != '\0') {
			break;
		}
	}
	return 1;
}