#include "structHeader.h"
int foo (struct foo *f, int b) {
    char *p = "HELLO";
    if (!f->string)
       f->string = "THIS IS MORE PLACEHOLDER";
    if (b) {
      p = f->string;
    }
    int i;
    for (int i = 0; p[i]; i++) {
        if(p[i] == 'a')
            return i;
    }
    return i;
}