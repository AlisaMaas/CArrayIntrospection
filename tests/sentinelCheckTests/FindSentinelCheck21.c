/**
 * This check tests we correctly do not mix up array
 * arguments across functions.
 *
 * We expect to find one non-optional sentinel check for string 
 * and 1 for string2 in both foo and find.
 **/

void printc(char c) {}
int foo(char string[], char string2[], int distance) {
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
		if (string2[i] == '\0')
			break;
	}
	return 1;
}
int find(char string[], char string2[], int distance) {
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
		if (string2[i] == '\0')
			break;
	}
	return 1;
}
