#ifndef CFG_H_
#define CFG_H_

#include "stream.h"

typedef struct Cfg Cfg;

struct Cfg {
	Cfg *next;
	Cfg *child;
	char *name;
	char *value;
};

extern int cfg_load(Cfg *);
extern int cfg_load_file(Cfg *, const char *);
extern int cfg_load_stream(Cfg *, Stream *);
extern Cfg *cfg_create(const char *, size_t);
extern Cfg *cfg_get(Cfg *, const char *);

#endif
