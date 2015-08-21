#include "structHeader.h"

/**
* Fixed length check - should find
* that fixed is fixed length.
**/

int foo (struct baz *b) 
{
    int sum = 0;
    for (int i = 0; i < 30; i++) 
    {
        sum += (int) b->fixed[i];
    }
    return sum;
}