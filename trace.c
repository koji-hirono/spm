#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "trace.h"


void
fatal(const char *fmt, ...)
{
	va_list ap;
	int last;
	int e;

	e = errno;

	fprintf(stderr, "*** Fatal error: ");

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	last = strlen(fmt) - 1;
	if (last < 0 || fmt[last] != '.') {
		fprintf(stderr, " %s.", strerror(e));
	}

	fprintf(stderr, "\n");

	exit(EXIT_FAILURE);
}

void
error(const char *fmt, ...)
{
	va_list ap;
	int last;
	int e;

	e = errno;

	fprintf(stderr, "*** Error: ");

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	last = strlen(fmt) - 1;
	if (last < 0 || fmt[last] != '.') {
		fprintf(stderr, " %s.", strerror(e));
	}

	fprintf(stderr, "\n");

	errno = e;
}

void
warn(const char *fmt, ...)
{
	va_list ap;
	int last;
	int e;

	e = errno;

	fprintf(stderr, "*** Warning: ");

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	last = strlen(fmt) - 1;
	if (last < 0 || fmt[last] != '.') {
		fprintf(stderr, " %s.", strerror(e));
	}

	fprintf(stderr, "\n");

	errno = e;
}
