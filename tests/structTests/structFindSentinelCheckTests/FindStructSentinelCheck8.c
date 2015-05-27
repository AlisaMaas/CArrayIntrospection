#include "structHeader.h"
/**
 * This check tests we detect a non-optional sentinel check 
 * in a loop where there are no other reads from the array,
 * but there is a loop between the sentinel check and exiting
 * the loop.
 *
 * We expect to find one non-optional sentinel check.
 **/
void print(char *string) {}
int find(struct foo *f, int distance) {
	for (int i = 0; i < distance; i++) {
		if (f->string[i] == '\0') {
			for (int j = 0; j < i; j++) {
				print("Hello");
			}
			break;
		}
	}
	return 1;
}
