#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "trace.h"
#include "stream.h"
#include "board.h"
#include "boardfmt.h"
#include "movelog.h"


void
movelog_init(Movelog *log)
{
	log->list = NULL;
	log->n = 0;
}

void
movelog_destroy(Movelog *log)
{
	free(log->list);
}

int
movelog_add(Movelog *log, Move *add)
{
	Move *move;
	size_t size;

	size = (log->n + 1) * sizeof(Move);

	if ((log->list = realloc(log->list, size)) == NULL) {
		error("realloc");
		return -1;
	}

	move = log->list + log->n++;
	*move = *add;

	return 0;
}

int
movelog_save(Movelog *log, const char *fname)
{
	Stream s;
	Move *move;
	FILE *fp;
	int i;

	if ((fp = fopen(fname, "w")) == NULL) {
		error("fopen(%s)", fname);
		return -1;
	}

	stream_fileinit(&s, fp);

	for (i = 0; i < log->n; i++) {
		move = log->list + i;
		move_show(&s, move);
		stream_putc(&s, '\n');
	}

	fclose(fp);

	return 0;
}

int
movelog_load(Movelog *log, const char *fname)
{
	Stream s;
	FILE *fp;

	if ((fp = fopen(fname, "r")) == NULL) {
		error("fopen(%s)", fname);
		return -1;
	}

	stream_fileinit(&s, fp);

	if (movelog_load_stream(log, &s) != 0) {
		fclose(fp);
		return -1;
	}

	fclose(fp);

	return 0;
}

int
movelog_load_stream(Movelog *log, Stream *s)
{
	Move move;
	int c;

	while (stream_peekc(s) != EOF) {
		if (move_parse(&move, s) != 0) {
			error("illegal format.");
			return -1;
		}

		if (movelog_add(log, &move) != 0)
			return -1;

		do {
			c = stream_getc(s);
		} while (isspace(c));

		stream_ungetc(c, s);
	}

	return 0;
}
