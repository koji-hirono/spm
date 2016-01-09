#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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
buf_pushoct(Buf *buf, const void *oct, size_t len)
{
	unsigned char *bp;

	if ((bp = buf_expand(buf, len)) == NULL)
		return -1;
	memcpy(bp, oct, len);
	return 0;
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
buf_pushfmt(Buf *buf, const char *fmt, ...)
{
	va_list ap;
	int r;

	va_start(ap, fmt);
	r = buf_pushvfmt(buf, fmt, ap);
	va_end(ap);

	return r;
}

int
buf_pushvfmt(Buf *buf, const char *fmt, va_list ap)
{
	size_t off;
	char *bp;
	int len;
	int r;

	off = buf->n;
	len = 128;
	for (;;) {
		if ((bp = buf_expand(buf, len)) == NULL)
			return -1;
		r = vsnprintf(buf->b + off, len, fmt, ap);
		if (r < 0)
			return -1;
		if (r < len) {
			buf->n = off + r;
			break;
		}
		buf->n = off;
		len = r + 1;
	}

	return 0;
}
