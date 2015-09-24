void find (int *a, int len) {
    int *end = a + len;
    while (a++ < end) {
        (void) *a;
    }
    if (a != end) 
    ;
}