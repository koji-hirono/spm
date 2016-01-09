#ifndef TRACE_H_
#define TRACE_H_

#include <stdarg.h>

extern void fatal(const char *, ...);
extern void error(const char *, ...);
extern void warn(const char *, ...);

#endif /* !TRACE_H_ */
