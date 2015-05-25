#include "structHeader.h"
int find(struct foo *f, char dest) {
    for (int i = 0; f->string[i]; i++) {
        if (dest == f->string[i]) {
            return 1;
        }
    }
    return 0;
}

void setString(struct foo *f, char* string) {
    f->string = string;
}