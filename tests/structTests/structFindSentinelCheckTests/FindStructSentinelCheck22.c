#include "structHeader.h"
int foo(struct foo *f) {
   for (int i = 0; f->string[i] != '\0'; ++i) {
      if (f->string[i] == 'a') {
          return 1;
      }
   } 
   return -1;
}
