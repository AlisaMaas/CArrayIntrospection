#include "headers.h"

int bar (char *f, char *i, char *s, int l) {
    return foo(f, i, s, l);
}

void baz(char *d, char *s, int l) {
    memcpy(d, s, l);
}