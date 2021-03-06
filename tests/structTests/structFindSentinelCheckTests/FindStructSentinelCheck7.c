#include "structHeader.h"
/**
 * This check tests we detect an optional sentinel check 
 * in a loop where there are no other reads from the array.
 *
 * We expect to find one optional sentinel check.
 **/
void print(char *string) {}
int find(struct foo *f, int distance, int flag) {
	for (int i = 0; i < distance; i++) {
		if (flag)
			if (f->nonString[i] == '\0') {
				break;
			}
	}
	return 1;
}
