#ifndef INI_H_
#define INI_H_

#include <stdio.h>

#include "str.h"
#include "buf.h"
#include "stream.h"

typedef struct INIScanner INIScanner;

enum {
	INI_SECT,
	INI_KEY,
	INI_VAL
};

struct INIScanner {
	Stream *in;
	Buf buf;
	Str line;
	int i;
};

extern int iniscanner_create(INIScanner *, Stream *);
extern void iniscanner_destroy(INIScanner *);
extern int iniscanner_gettok(INIScanner *, Str *);

#endif
