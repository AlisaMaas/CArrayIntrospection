/**
 * This check tests we detect a non-optional sentinel check 
 * in a loop where there are no other reads from the array
 * and there are statements between the sentinel check
 * and escaping from the loop.
 *
 * We expect to find one non-optional sentinel check.
 **/
void print(char *string) {}
int find(char string[], int distance) {
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			for (int j = 0; j < i; j++) {
				print("Hello");
			}
			break;
		}
	}
	return 1;
}
