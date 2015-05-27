/**
* Access to a struct containing two 
* different kinds of struct fields.
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