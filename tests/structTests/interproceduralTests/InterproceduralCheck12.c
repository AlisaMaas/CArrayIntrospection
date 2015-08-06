#include "structHeader.h"

/**
* Fixed length check - should find
* that both is not fixed length.
**/

int foo (struct baz *b, int z) 
{
    int sum = 0;
    for (int i = 0; i < 30; i++) 
    {
        sum += (int) b->both[i];
    }
    for (int i = 0; i < z*z; i++) {
        sum += (int) b->both[i];
    }
    return sum;
}