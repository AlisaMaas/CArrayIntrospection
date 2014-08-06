/**
 * This check tests we do not spurously detect sentinel
 * checks in a loop with no reads in the array.
 *
 * We expect to find no sentinel checks.
 **/
void print(char *string) {}
int find(char string[], int distance) {
	for (int i = 0; i < distance; i++) {
		print("hello, world");
	}
	return 1;
}
