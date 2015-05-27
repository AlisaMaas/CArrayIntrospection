/**
*
* Make sure we don't consider things to be null
* terminated if there's an extra load blocking our
* path.
*
**/

void find(char *arg) {
    for (int i = 0; arg[i]; i++){
        find(arg);
    }
}

void load(char **argv) {
    find(*argv);
}