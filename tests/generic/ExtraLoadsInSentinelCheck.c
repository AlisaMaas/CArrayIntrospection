/**
*
* Make sure we don't consider things to be null
* terminated if elements in the array are null terminated,
* but the array itself is not guaranteed null terminated.
*
**/

void find(char **arg) {
    find(arg + 7);
    for (int i = 0; *arg[i]; i++){
        find(arg);
    }
}