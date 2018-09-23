#ifndef STR_H_
#define STR_H_

#include <stddef.h>
#include <string.h>

typedef struct Str Str;

struct Str {
	char *s;
	size_t len;
};

#define STR_L(x) (Str){""x, sizeof(""x) - 1}
#define STR_S(x) (Str){x, strlen(x)}

extern int str_substr(Str *, const Str *, int, int);

extern int str_equals(const Str *, const Str *);
extern int str_contains(const Str *, const Str *);
extern int str_startswith(const Str *, const Str *);
extern int str_endswith(const Str *, const Str *);

extern int str_containschar(const Str *, int);

extern int str_index(const Str *, const Str *);
extern int str_lastindex(const Str *, const Str *);
extern int str_indexany(const Str *, const Str *);
extern int str_lastindexany(const Str *, const Str *);
extern int str_indexchar(const Str *, int);

extern void str_trim_prefix(Str *, const Str *, const Str *);
extern void str_trim_suffix(Str *, const Str *, const Str *);
extern void str_trim(Str *, const Str *, const Str *);
extern void str_trim_left(Str *, const Str *, const Str *);
extern void str_trim_right(Str *, const Str *, const Str *);

#endif
