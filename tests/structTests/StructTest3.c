/**
* Try doing some accesses to two 
* separate structs, some of which set
* struct fields based on other struct 
* fields.
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

void accessBoth(struct foo f, struct bar b) {
	b.x = f.a;
	b.y = f.a * f.b;
	b.z = &b;
	b.first = f;
}