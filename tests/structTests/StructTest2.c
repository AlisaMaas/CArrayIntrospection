/**
* Access a struct with a recursive element 
* and no other structs around.
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

void accessFoo(struct foo f) {
	f.a = 32;
	f.b = 1.2;
	f.c = &f;
}