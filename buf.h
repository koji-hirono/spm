#ifndef BUF_H_
#define BUF_H_

#include <stddef.h>
#include <stdarg.h>

typedef struct {
	char *b;
	int n;
} Buf;

#if !defined(__GNUC__)
#define __attribute__(x)
#endif

extern void buf_init(Buf *);
extern void buf_free(Buf *);
extern void buf_clear(Buf *);
extern void *buf_expand(Buf *, size_t);
extern int buf_pushoct(Buf *, const void *, size_t);
extern int buf_pushc(Buf *, int);
extern int buf_pushfmt(Buf *, const char *, ...)
	__attribute__((__format__(__printf__, 2, 3)));
extern int buf_pushvfmt(Buf *, const char *, va_list);

#endif /* !BUF_H_ */
