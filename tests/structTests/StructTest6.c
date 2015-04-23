/**
* Check that the results from multiple
* functions are correctly collated into 
* a single result per struct element,
* regardless of where it's found.
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

void accessBar(struct bar b) {
	b.x = 5;
	b.y = 4.5;
	b.z = &b; 
}

void accessFoo(struct foo f) {
	f.a = 32;
	f.b = 1.2;
	f.c = &f;
}

void accessBoth(struct foo f, struct bar b) {
	b.x = f.a;
	b.y = f.a * f.b;
	b.z = &b;
	b.first = f;
}

void accessArrays(struct foo *f, struct bar *b) {
	for (int i = 0; i < 40; i++) {
		f[i] = f[0];
	}
	f[2] = f[0];
	b[200] = b[0];
	b[0] = b[0];
	b[1] = b[0];
	
}

void accessArrayStructElements(struct foo *f, struct bar *b) {
	for (int i = 0; i < 40; i++) {
		f[i].a = 9;
	}
	f[2].b = 2.2;
	b[200].first = f[0];
	b[0].x = 3;
	b[1].y = 3.3;
	b[4].z = b;
	
}