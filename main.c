#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include "trace.h"
#include "stream.h"
#include "board.h"
#include "movelog.h"
#include "game.h"
#include "engine.h"
#include "interp.h"

static void usage(const char *);

int
main(int argc, char **argv)
{
	const char *progname;
	const char *fname;
	const char *engine_path;
	const char *sfen;
	Interp interp;
	Game game;
	Engine engine;
	Stream in;
	Stream out;
	int debug;
	int opt;

	progname = argv[0];
	debug = 0;
	engine_path = "";
	sfen = NULL;
	while ((opt = getopt(argc, argv, "d:e:s:")) != -1) {
		switch (opt) {
		case 'd':
			debug = strtol(optarg, NULL, 10);
			break;
		case 'e':
			engine_path = optarg;
			break;
		case 's':
			sfen = optarg;
			break;
		default:
			usage(progname);
			return EXIT_FAILURE;
		}
	}
	argc -= optind;
	argv += optind;

	fname = (argc == 1) ? argv[0] : NULL;

	if (game_init(&game, sfen) != 0)
		return EXIT_FAILURE;

	if (fname)
		if (movelog_load(&game.movelog, fname) != 0)
			return EXIT_FAILURE;

	if (engine_open(&engine, engine_path, debug) != 0)
		return EXIT_FAILURE;

	if (engine_init(&engine) != 0)
		return EXIT_FAILURE;

	stream_fileinit(&in, stdin);
	stream_fileinit(&out, stdout);

	interp_init(&interp);
	interp.in = &in;
	interp.out = &out;
	interp.game = &game;
	interp.engine = &engine;

	interp_loop(&interp);

	engine_close(&engine);

	return EXIT_SUCCESS;
}

static void
usage(const char *progname)
{
	fprintf(stderr, "usage: %s [-d debug level] [-e engine path]"
		" [-s sfen] [FILE]\n", progname);
	fprintf(stderr, "    -d: debug level(0-2)\n");
	fprintf(stderr, "    -e: engine path\n");
	fprintf(stderr, "    -s: SFEN\n");
}
