/**
* Ensure that array accesses don't
* fool us into adding incorrect results.
**/

struct foo {
	int a;
	double b;
	struct foo *c;
};

struct bar {
	struct foo first;
	int x;
	double y;
	struct bar *z;
};

void accessArrays(struct foo *f, struct bar *b) {
	for (int i = 0; i < 40; i++) {
		f[i] = f[0];
	}
	f[2] = f[0];
	b[200] = b[0];
	b[0] = b[0];
	b[1] = b[0];
	
}