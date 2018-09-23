#include <stdio.h>
#include <stdlib.h>

#include "str.h"
#include "ini.h"


int
iniscanner_create(INIScanner *p, Stream *in)
{
	buf_init(&p->buf);
	p->in = in;
	p->i = 0;

	return 0;
}

void
iniscanner_destroy(INIScanner *p)
{
	buf_free(&p->buf);
}

int
iniscanner_gettok(INIScanner *p, Str *token)
{
	if (p->i > 0) {
		if (str_substr(token, &p->line, p->i + 1, -1) != 0)
			*token = STR_L("");
		else
			str_trim(token, token, NULL);
		p->i = 0;
		return INI_VAL;
	}

	while (stream_gets(p->in, &p->buf) == 0) {
		str_trim(&p->line, &STR_S(p->buf.b), NULL);

		/* blank line */
		if (str_equals(&p->line, &STR_L("")))
			continue;

		/* comment */
		if (str_startswith(&p->line, &STR_L(";")) ||
		    str_startswith(&p->line, &STR_L("#")))
			continue;

		/* section */
		if (str_startswith(&p->line, &STR_L("["))) {
			str_trim_prefix(token, &p->line, &STR_L("["));
			str_trim_suffix(token, token, &STR_L("]"));
			str_trim(token, token, NULL);
			return INI_SECT;
		}

		/* key */
		p->i = str_indexany(&p->line, &STR_L("=:"));
		if (p->i == -1)
			continue;
		str_substr(token, &p->line, 0, p->i);
		str_trim(token, token, NULL);
		return INI_KEY;
	}

	return -1;
}
