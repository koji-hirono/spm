#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <unistd.h>

#include "buf.h"
#include "stream.h"

void
stream_ioinit(Stream *s, int fd)
{
	s->type = STREAM_IO;
	s->flags = 0;
	s->fd = fd;
}

void
stream_fileinit(Stream *s, FILE *fp)
{
	s->type = STREAM_FILE;
	s->flags = 0;
	s->fp = fp;
}

void
stream_strinit(Stream *s, const char *str)
{
	s->type = STREAM_STRING;
	s->flags = 0;
	s->s = str;
	s->p = str;
}

int
stream_getc(Stream *s)
{
	int c;

	switch (s->type) {
	case STREAM_IO:
		{
			unsigned char uc;
			int r;

			r = read(s->fd, &uc, 1);
			if (r < 0) {
				c = EOF;
			} else if (r == 0) {
				c = EOF;
			} else {
				c = uc;
			}
		}
		break;
	case STREAM_FILE:
		c = fgetc(s->fp);
		break;
	case STREAM_STRING:
		if (*s->p == '\0')
			c = EOF;
		else
			c = *s->p++ & 0xff;
		break;
	default:
		c = EOF;
		break;
	}

	return c;
}

void
stream_ungetc(int c, Stream *s)
{
	if (c == EOF)
		return;

	switch (s->type) {
	case STREAM_IO:
		break;
	case STREAM_FILE:
		ungetc(c, s->fp);
		break;
	case STREAM_STRING:
		if (s->p > s->s)
			s->p--;
		break;
	default:
		break;
	}
}

int
stream_peekc(Stream *s)
{
	int c;

	switch (s->type) {
	case STREAM_IO:
		c = EOF;
		break;
	case STREAM_FILE:
		c = fgetc(s->fp);
		ungetc(c, s->fp);
		break;
	case STREAM_STRING:
		if (*s->p == '\0')
			c = EOF;
		else
			c = *s->p & 0xff;
		break;
	default:
		c = EOF;
		break;
	}

	return c;
}

int
stream_gets(Stream *s, Buf *buf)
{
	int c;

	if (s->flags & STREAM_END)
		return -1;

	if ((c = stream_getc(s)) == EOF) {
		s->flags |= STREAM_END;
		return -1;
	}

	for (;;) {
		if (c == EOF) {
			s->flags |= STREAM_END;
			break;
		}
		if (c == '\n')
			break;

		/* XXX */
		if (c == '\r')
			c = '\0';

		buf_pushc(buf, c);
		c = stream_getc(s);
	}
	buf_pushc(buf, '\0');

	return 0;
}

int
stream_putc(Stream *s, int c)
{
	int r;

	switch (s->type) {
	case STREAM_IO:
		{
			unsigned char uc;

			uc = c;
			r = write(s->fd, &uc, 1);
		}
		break;
	case STREAM_FILE:
		if (fputc(c, s->fp) == EOF)
			r = -1;
		else
			r = 1;
		break;
	case STREAM_STRING:
		r = -1;
		break;
	default:
		r = -1;
		break;
	}

	return r;
}

int
stream_puts(Stream *s, const char *str)
{
	return stream_nputs(s, str, strlen(str));
}

int
stream_nputs(Stream *s, const char *str, size_t len)
{
	int r;

	switch (s->type) {
	case STREAM_IO:
		r = write(s->fd, str, len);
		break;
	case STREAM_FILE:
		r = fwrite(str, 1, len, s->fp);
		break;
	case STREAM_STRING:
		r = -1;
		break;
	default:
		r = -1;
		break;
	}

	return r;
}


int
stream_fmtputs(Stream *s, const char *fmt, ...)
{
	va_list ap;
	int r;

	va_start(ap, fmt);
	r = stream_vfmtputs(s, fmt, ap);
	va_end(ap);

	return r;
}

int
stream_vfmtputs(Stream *s, const char *fmt, va_list ap) 
{
	int r;

	switch (s->type) {
	case STREAM_IO:
		{
			Buf b;

			buf_init(&b);
			if (buf_pushvfmt(&b, fmt, ap) != 0)
				return -1;
			write(s->fd, b.b, b.n);
			r = b.n;
			buf_free(&b);
		}
		break;
	case STREAM_FILE:
		r = vfprintf(s->fp, fmt, ap);
		break;
	case STREAM_STRING:
		r = 0;
		break;
	default:
		r = 0;
		break;
	}

	return r;
}
