#include "structHeader.h"
/**
 * This check tests we detect a non-optional sentinel check 
 * in a loop where there is a nested loop after the sentinel
 * check reading from the array (but not past the sentinel value).
 *
 * We expect to find one non-optional sentinel check.
 **/
void print(char *string) {}
void printc(char c) {}
int find(struct foo *f, int distance) {
	for (int i = 0; i < distance; i++) {
		if (f->string[i] == '\0') {
			for (int j = 0; j < i; j++) {
				printc(f->string[j]);
			}
			break;
		}
	}
	return 1;
}
