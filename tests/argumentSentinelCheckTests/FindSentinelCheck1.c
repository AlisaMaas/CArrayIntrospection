/**
 * This check tests we detect a non-optional sentinel check 
 * in a loop where there is no loop guard and no other reads 
 * from the array, but there is a statement between the sentinel
 * check and the break from the loop.
 *
 * We expect to find one non-optional sentinel check.
 **/
void print(char *string) {}
int foo(char string[]) {
	for (int i = 0;; i++) {
		if (string[i] == '\0') {
			print("About to break from loop");
			break;
		}
	}
	return 1;
}
int find(char string[]) {
	for (int i = 0;; i++) {
		if (string[i] == '\0') {
			print("About to break from loop");
			break;
		}
	}
	return 1;
}
