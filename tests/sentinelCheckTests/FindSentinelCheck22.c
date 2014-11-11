#include <stddef.h>
int foo(char* a) {
   /*if (a == NULL) {
      a = "";
   }*/
   for (int i = 0; a[i] != '\0'; ++i) {
      if (a[i] == 'a') {
          return 1;
      }
   } 
   return -1;
}
