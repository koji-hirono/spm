#ifndef BUF_H_
#define BUF_H_

#include <stddef.h>
#include <stdarg.h>

typedef struct {
	char *b;
	int n;
} Buf;

extern void buf_init(Buf *);
extern void buf_free(Buf *);
extern void buf_clear(Buf *);
extern void *buf_expand(Buf *, size_t);
extern int buf_pushoct(Buf *, const void *, size_t);
extern int buf_pushc(Buf *, int);
extern int buf_pushfmt(Buf *, const char *, ...);
extern int buf_pushvfmt(Buf *, const char *, va_list);

#endif /* !BUF_H_ */
