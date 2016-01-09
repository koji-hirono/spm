#include <stdlib.h>
#include <string.h>

#include "stringex.h"


char *
strexndup(const char *s, size_t len)
{
	char *p;

	if ((p = malloc(len + 1)) == NULL)
		return NULL;

	memcpy(p, s, len);
	p[len] = '\0';

	return p;
}
