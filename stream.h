#ifndef STREAM_H_
#define STREAM_H_

#include <stdio.h>
#include <stdarg.h>

#include "buf.h"

typedef struct Stream Stream;

enum {
	STREAM_IO,
	STREAM_FILE,
	STREAM_STRING
};

struct Stream {
	int type;
	int flags;
	union {
		int fd;
		FILE *fp;
		struct {
			const char *s;
			const char *p;
		};
	};
};

#define STREAM_END (1 << 0)

extern void stream_ioinit(Stream *, int);
extern void stream_fileinit(Stream *, FILE *);
extern void stream_strinit(Stream *, const char *);
extern int stream_getc(Stream *);
extern void stream_ungetc(int, Stream *);
extern int stream_peekc(Stream *);
extern int stream_gets(Stream *, Buf *);

extern int stream_putc(Stream *, int);
extern int stream_puts(Stream *, const char *);
extern int stream_nputs(Stream *, const char *, size_t);
extern int stream_fmtputs(Stream *, const char *, ...);
extern int stream_vfmtputs(Stream *, const char *, va_list);

#endif /* !STREAM_H_ */
