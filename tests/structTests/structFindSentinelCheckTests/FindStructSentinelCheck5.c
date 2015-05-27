#include "structHeader.h"
/**
 * This check tests we detect an optional sentinel check 
 * in a loop where there is another read from the array.
 *
 * We expect to find one optional sentinel check.
 **/
void print(char *string) {}
int find(struct foo *f, char goal, int flag) {
	for (int i = 0; f->string[i] != goal; i++) {
		if (flag)
			if (f->nonString[i] == '\0') {
				break;
			}
	}
	return 1;
}
