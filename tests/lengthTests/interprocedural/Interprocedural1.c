/**
* Basic interprocedural test, nothing fancy.
**/
void print (char c){
}

int bar(char *x, int y) {
    for (int i = 0; i < y; i++) {
       print(x[i]); 
    }
    return 1;
}

int foo(char *x, int y) {
    return bar(x, y);
}