int foo (char *s, int b) {
    char *p = "HELLO";
    if (!s)
       s = "THIS IS MORE PLACEHOLDER";
    if (b) {
      p = s;
    }
    int i;
    for (int i = 0; p[i]; i++) {
        if(p[i] == 'a')
            return i;
    }
    return i;
}