#include <stdio.h>
#include <string.h>

#include "trace.h"
#include "buf.h"
#include "stream.h"
#include "board.h"
#include "boardfmt.h"
#include "engine.h"
#include "cmds.h"
#include "interp.h"

void
interp_init(Interp *interp)
{
	interp->in = NULL;
	interp->out = NULL;
	interp->game = NULL;
	interp->engine = NULL;
}

void
interp_loop(Interp *interp)
{
	Buf line;
	Cmd *cmd;
	int end;
	int len;
	int p;

	end = 0;
	p = '>';

	buf_init(&line);
	while (!end) {
		stream_puts(interp->out, side_name(cur_side(interp->game)));
		stream_fmtputs(interp->out, "(%d/%d)? ",
			interp->game->turn + 1, interp->game->movelog.n);
		buf_clear(&line);
		if (stream_gets(interp->in, &line) != 0)
			break;

		if (line.b[0] == '\0')
			line.b[0] = p;

		if ((cmd = cmd_find(line.b[0])) == NULL) {
			stream_puts(interp->out, "unknown command.\n");
			continue;
		}

		len = strspn(line.b + 1, " ");

		end = (*cmd->exec)(interp, line.b + len + 1);

		p = line.b[0];
	}
	buf_free(&line);
}
