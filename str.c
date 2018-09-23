#include "str.h"

int
str_substr(Str *dst, const Str *src, int start, int len)
{
	if (start < 0)
		start += src->len;
	if (start < 0)
		return -1;
	if (len < 0)
		len = src->len - start;
	if (len < 0)
		return -1;
	if ((size_t)start + len > src->len)
		return -1;

	dst->s = src->s + start;
	dst->len = len;

	return 0;
}

int
str_equals(const Str *a, const Str *b)
{
	size_t i;

	if (a->len != b->len)
		return 0;

	for (i = 0; i < a->len; i++)
		if (a->s[i] != b->s[i])
			return 0;

	return 1;
}

int
str_contains(const Str *a, const Str *b)
{
	return str_index(a, b) != -1;
}

int
str_startswith(const Str *text, const Str *word)
{
	Str sub;

	if (str_substr(&sub, text, 0, word->len) != 0)
		return 0;

	return str_equals(&sub, word);
}

int
str_endswith(const Str *text, const Str *word)
{
	Str sub;

	if (str_substr(&sub, text, -word->len, word->len) != 0)
		return 0;

	return str_equals(&sub, word);
}

int
str_containschar(const Str *text, int ch)
{
	return str_indexchar(text, ch) != -1;
}

int
str_index(const Str *text, const Str *word)
{
	Str sub;
	int i;

	for (i = 0; str_substr(&sub, text, i, word->len) == 0; i++)
		if (str_equals(&sub, word))
			return i;

	return -1;
}

int
str_lastindex(const Str *text, const Str *word)
{
	Str sub;
	int i;

	for (i = text->len - word->len;
	     i >= 0 && str_substr(&sub, text, i, word->len) == 0;
	     i--)
		if (str_equals(&sub, word))
			return i;

	return -1;
}

int
str_indexany(const Str *text, const Str *any)
{
	size_t i;

	for (i = 0; i < text->len; i++)
		if (str_containschar(any, text->s[i]))
			return i;

	return -1;
}

int
str_lastindexany(const Str *text, const Str *any)
{
	size_t i;

	for (i = text->len; i > 0; i--)
		if (str_containschar(any, text->s[i - 1]))
			return i - 1;

	return -1;
}

int
str_indexchar(const Str *text, int ch)
{
	size_t i;

	for (i = 0; i < text->len; i++)
		if (text->s[i] == ch)
			return i;

	return -1;
}

int
str_lastindexchar(const Str *text, int ch)
{
	size_t i;

	for (i = text->len; i > 0; i--)
		if (text->s[i - 1] == ch)
			return i - 1;

	return -1;
}

void
str_trim_prefix(Str *dst, const Str *src, const Str *word)
{
	if (str_startswith(src, word))
		str_substr(dst, src, word->len, -1);
}

void
str_trim_suffix(Str *dst, const Str *src, const Str *word)
{
	if (str_endswith(src, word))
		str_substr(dst, src, 0, src->len - word->len);
}

void
str_trim(Str *dst, const Str *src, const Str *any)
{
	if (any == NULL)
		any = &STR_L(" \t\r\n");

	str_trim_left(dst, src, any);
	str_trim_right(dst, dst, any);
}

void
str_trim_left(Str *dst, const Str *src, const Str *any)
{
	size_t i;

	if (any == NULL)
		any = &STR_L(" \t\r\n");

	for (i = 0; i < src->len; i++)
		if (!str_containschar(any, src->s[i]))
			break;

	dst->s = src->s + i;
	dst->len = src->len - i;
}

void
str_trim_right(Str *dst, const Str *src, const Str *any)
{
	size_t i;

	if (any == NULL)
		any = &STR_L(" \t\r\n");

	for (i = src->len; i > 0; i--)
		if (!str_containschar(any, src->s[i - 1]))
			break;

	dst->s = src->s;
	dst->len = i;
}
