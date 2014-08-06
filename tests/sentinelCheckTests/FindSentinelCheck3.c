/**
 * This check tests we detect a non-optional sentinel check 
 * in a loop where there are no other reads from the array in 
 * more than one loop.
 *
 * We expect to find 14 loops with non-optional sentinel checks.
 * 
 **/
void print(char *string) {}
int bar(char string[], int distance) {
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
	}
	return 1;
}
int foo(char string[], int distance) {
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
	}
	return 1;
}
int baz(char string[], int distance) {
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
	}
	return 1;
}
int find(char string[], int distance) {
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
	}
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
	}
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
	}

	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
	}
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
	}
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
	}
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
	}
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
	}
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
	}
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
	}
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
	}
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
	}
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
	}
	for (int i = 0; i < distance; i++) {
		if (string[i] == '\0') {
			break;
		}
	}
	return 1;
}
