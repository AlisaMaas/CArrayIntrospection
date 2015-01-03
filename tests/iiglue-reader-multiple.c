#include "iiglue-reader-peek.h"
#include "iiglue-reader-variadic.h"

int multiple(int *values)
{
	variadic(values);
	return peek(values, 0);
}
