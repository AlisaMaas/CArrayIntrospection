struct foo {
	char *string;
	char *nonString;
	struct foo *foo;
};

struct bar {
	struct foo first;
	char* string;
	char* nonString;
	struct bar *z;
};

struct baz {
    char fixed[30];
    char *symbolic;
    char *both;
    int len;
};