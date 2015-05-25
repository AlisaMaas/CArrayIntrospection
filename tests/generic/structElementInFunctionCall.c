#include "structHeader.h"
int find(char *string, char dest) {
    for (int i = 0; string[i]; i++) {
        if (dest == string[i]) {
            return 1;
        }
    }
    return 0;
}

void call(struct foo *f, char c) {
    find(f->string, c);
}