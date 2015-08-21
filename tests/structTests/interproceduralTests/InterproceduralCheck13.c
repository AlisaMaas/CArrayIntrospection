#include "structHeader.h"

/**
* Make sure that sentinel terminated
* beats fixed length in a fight.
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