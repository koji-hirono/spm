#ifndef CMDS_H_
#define CMDS_H_

#include "interp.h"

typedef struct Cmd Cmd;

struct Cmd {
	int ch;
	const char *desc;
	int (*exec)(Interp *, const char *);
};

extern Cmd *cmd_find(int);

#endif /* !CMDS_H_ */
