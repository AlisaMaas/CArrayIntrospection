/**
* Ensure that we still catch
* struct accesses that are done via
* an array and a struct field access.
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

void accessArrayStructElements(struct foo *f, struct bar *b) {
	for (int i = 0; i < 40; i++) {
		f[i].a = 9;
	}
	f[2].b = 2.2;
	b[200].first = f[0];
	b[0].x = 3;
	b[1].y = 3.3;
	b[4].z = b;
	f[0].c[4].a = 3;
	
}