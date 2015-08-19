int m_strlen(char *a) {
    int i = 0;
    while (a[i++]);
    return i;
}
int foo (char *a, int len) {
    if (len < 0) {
        len = m_strlen(a);
    }
    int sum = 0;
    for (int i = 0; i < len; i++) {
        sum += (int)a[i];
    }
    return sum;
}