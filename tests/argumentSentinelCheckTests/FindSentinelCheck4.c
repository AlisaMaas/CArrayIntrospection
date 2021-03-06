/**
 * This check tests we detect an optional sentinel check 
 * in a loop where there are no other reads from the array and no other branches.
 *
 * We expect to find one optional sentinel check.
 **/
void print(char *string) {}
int find(char string[], int flag) {
	for (int i = 0;; i++) {
		if (flag)
			if (string[i] == '\0') {
				break;
			}
	}
	return 1;
}
