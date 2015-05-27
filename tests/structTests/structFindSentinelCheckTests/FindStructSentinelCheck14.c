#include "structHeader.h"
/**
 * This check tests we detect a non-optional sentinel check 
 * in a loop where the sentinel check is inside the for condition.
 *
 * We expect to find one non-optional sentinel check.
 **/
void print(char *string) {}
int find(struct foo *f, int distance) {
	for (int i = 0; f->string[i] != '\0'; i++) {
		print("Hello");
	}
	return 1;
}

int findArrayAccesses(struct foo *f, int distance) {
	for (int i = 0; f[0].string[i] != '\0'; i++) {
		print("Hello");
	}
	return 1;
}

int findIndirectArrayAccesses(struct foo *f, int distance) {
	for (int i = 0; f[0].foo->string[i] != '\0'; i++) {
		print("Hello");
	}
	return 1;
}