#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"

void
buf_init(Buf *buf)
{
	buf->b = NULL;
	buf->n = 0;
}

void
buf_free(Buf *buf)
{
	free(buf->b);
}

void
buf_clear(Buf *buf)
{
	buf->n = 0;
}

void *
buf_expand(Buf *buf, size_t add)
{
	void *p;

	if ((p = realloc(buf->b, buf->n + add)) == NULL)
		return NULL;
	buf->b = p;
	p = &buf->b[buf->n];
	buf->n += add;
	return p;
}

int
buf_pushc(Buf *buf, int c)
{
	unsigned char *bp;

	if ((bp = buf_expand(buf, 1)) == NULL)
		return -1;
	*bp = c;
	return 0;
}

int
buf_pushoct(Buf *buf, const void *oct, size_t len)
{
	unsigned char *bp;

	if ((bp = buf_expand(buf, len)) == NULL)
		return -1;
	memcpy(bp, oct, len);
	return 0;
}

int
buf_pushfmt(Buf *b, const char *fmt, ...)
{
	va_list ap;
	int r;

	va_start(ap, fmt);
	r = buf_pushvfmt(b, fmt, ap);
	va_end(ap);

	return r;
}

int
buf_pushvfmt(Buf *b, const char *fmt, va_list ap)
{
	ssize_t size;
	size_t off;
	int r;

	size = 128;
	off = b->n;
	for (;;) {
		if (buf_expand(b, size) == NULL)
			return -1;
		if ((r = vsnprintf(b->b + off, size, fmt, ap)) < 0)
			return -1;
		if (r < size) {
			b->n = off + r;
			break;
		}
		b->n = off;
		size = r + 1;
	}

	return 0;
}
