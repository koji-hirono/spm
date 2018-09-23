#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include "stream.h"
#include "stringex.h"
#include "buf.h"
#include "str.h"
#include "ini.h"
#include "cfg.h"


int
cfg_load(Cfg *cfg)
{
	const char *home;
	Buf path;
	int r;

	if ((home = getenv("HOME")) == NULL)
		return -1;

	buf_init(&path);
	buf_pushstr(&path, home);
	buf_pushc(&path, '/');
	buf_pushstrz(&path, ".spmrc");

	if (access(path.b, F_OK) == 0)
		r = cfg_load_file(cfg, path.b);
	else
		r = 0;

	buf_free(&path);

	return r;
}

int
cfg_load_file(Cfg *cfg, const char *fname)
{
	Stream in;
	FILE *fp;
	int r;

	if ((fp = fopen(fname, "r")) == NULL)
		return -1;

	stream_fileinit(&in, fp);

	r = cfg_load_stream(cfg, &in);

	fclose(fp);

	return r;
}

int
cfg_load_stream(Cfg *cfg, Stream *in)
{
	INIScanner scanner;
	Cfg *sect;
	Cfg **sect_next;
	Cfg *entry;
	Cfg **entry_next;
	Str token;
	int t;

	if (iniscanner_create(&scanner, in) != 0)
		return -1;

	sect = NULL;
	sect_next = &cfg->child;
	entry = NULL;
	entry_next = NULL;
	while ((t = iniscanner_gettok(&scanner, &token)) != -1) {
		switch (t) {
		case INI_SECT:
			if ((sect = cfg_create(token.s, token.len)) == NULL)
				break;
			*sect_next = sect;
			sect_next = &sect->next;
			entry_next = &sect->child;
			break;
		case INI_KEY:
			if (entry_next == NULL)
				break;
			if ((entry = cfg_create(token.s, token.len)) == NULL)
				break;
			*entry_next = entry;
			entry_next = &entry->next;
			break;
		case INI_VAL:
			if (entry == NULL)
				break;
			if ((entry->value = strexndup(token.s, token.len)) == NULL)
				break;
			break;
		default:
			break;
		}
	}

	iniscanner_destroy(&scanner);

	return 0;
}

Cfg *
cfg_create(const char *name, size_t namelen)
{
	Cfg *cfg;

	if ((cfg = malloc(sizeof(Cfg))) == NULL)
		return NULL;

	if ((cfg->name = strexndup(name, namelen)) == NULL) {
		free(cfg);
		return NULL;
	}

	cfg->next = NULL;
	cfg->child = NULL;
	cfg->value = NULL;

	return cfg;
}

Cfg *
cfg_get(Cfg *cfg, const char *name)
{
	Cfg *c;

	for (c = cfg->child; c != NULL; c = c->next)
		if (strcmp(name, c->name) == 0)
			return c;

	return NULL;
}
