/**
 * This check tests we detect a non-optional sentinel check 
 * in a loop where there are no other reads from the array
 * and the sentinel check is done using a stored result from
 * the array.
 *
 * We expect to find one non-optional sentinel check.
 **/
void print(char *string) {}
int find(char string[], int distance) {
	for (int i = 0; i < distance; i++) {
		char element = string[i];
		print("blah, blah");
		print(string);

		if (element == '\0') {
			break;
		}
	}
	return 1;
}
