int foo(int *a, int len) {
    int sum = 0;
    int *a_end = a + len;
    while (a != a_end) {
        sum += *a;
        a++;
   } 
   return sum;
}