/* Copyright (c) 2013, Vsevolod Stakhov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *       * Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 *       * Redistributions in binary form must reproduce the above copyright
 *         notice, this list of conditions and the following disclaimer in the
 *         documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ucl.h"
#include "ucl_internal.h"
#include "ucl_chartable.h"

/**
 * @file rcl_parser.c
 * The implementation of rcl parser
 */

struct ucl_parser_saved_state {
	unsigned int line;
	unsigned int column;
	size_t remain;
	const unsigned char *pos;
};

/**
 * Move up to len characters
 * @param parser
 * @param begin
 * @param len
 * @return new position in chunk
 */
#define ucl_chunk_skipc(chunk, p)    do{					\
    if (*(p) == '\n') {										\
        (chunk)->line ++;									\
        (chunk)->column = 0;								\
    }														\
    else (chunk)->column ++;								\
    (p++);													\
    (chunk)->pos ++;										\
    (chunk)->remain --;										\
    } while (0)

/**
 * Save parser state
 * @param chunk
 * @param s
 */
static inline void
ucl_chunk_save_state (struct ucl_chunk *chunk, struct ucl_parser_saved_state *s)
{
	s->column = chunk->column;
	s->pos = chunk->pos;
	s->line = chunk->line;
	s->remain = chunk->remain;
}

/**
 * Restore parser state
 * @param chunk
 * @param s
 */
static inline void
ucl_chunk_restore_state (struct ucl_chunk *chunk, struct ucl_parser_saved_state *s)
{
	chunk->column = s->column;
	chunk->pos = s->pos;
	chunk->line = s->line;
	chunk->remain = s->remain;
}

static inline void
ucl_set_err (struct ucl_chunk *chunk, int code, const char *str, UT_string **err)
{
	if (chunk->pos < chunk->end) {
		if (isgraph (*chunk->pos)) {
			ucl_create_err (err, "error on line %d at column %d: '%s', character: '%c'",
					chunk->line, chunk->column, str, *chunk->pos);
		}
		else {
			ucl_create_err (err, "error on line %d at column %d: '%s', character: '0x%02x'",
					chunk->line, chunk->column, str, (int)*chunk->pos);
		}
	}
	else {
		ucl_create_err (err, "error at the end of chunk: %s", str);
	}
}

/**
 * Skip all comments from the current pos resolving nested and multiline comments
 * @param parser
 * @return
 */
static bool
ucl_skip_comments (struct ucl_parser *parser)
{
	struct ucl_chunk *chunk = parser->chunks;
	const unsigned char *p;
	int comments_nested = 0;

	p = chunk->pos;

start:
	if (*p == '#') {
		if (parser->state != UCL_STATE_SCOMMENT &&
				parser->state != UCL_STATE_MCOMMENT) {
			while (p < chunk->end) {
				if (*p == '\n') {
					ucl_chunk_skipc (chunk, p);
					goto start;
				}
				ucl_chunk_skipc (chunk, p);
			}
		}
	}
	else if (*p == '/' && chunk->remain >= 2) {
		if (p[1] == '*') {
			ucl_chunk_skipc (chunk, p);
			comments_nested ++;
			ucl_chunk_skipc (chunk, p);

			while (p < chunk->end) {
				if (*p == '*') {
					ucl_chunk_skipc (chunk, p);
					if (*p == '/') {
						comments_nested --;
						if (comments_nested == 0) {
							ucl_chunk_skipc (chunk, p);
							goto start;
						}
					}
					ucl_chunk_skipc (chunk, p);
				}
				else if (p[0] == '/' && chunk->remain >= 2 && p[1] == '*') {
					comments_nested ++;
					ucl_chunk_skipc (chunk, p);
					ucl_chunk_skipc (chunk, p);
					continue;
				}
				ucl_chunk_skipc (chunk, p);
			}
			if (comments_nested != 0) {
				ucl_set_err (chunk, UCL_ENESTED, "unfinished multiline comment", &parser->err);
				return false;
			}
		}
	}

	return true;
}

/**
 * Return multiplier for a character
 * @param c multiplier character
 * @param is_bytes if true use 1024 multiplier
 * @return multiplier
 */
static inline unsigned long
ucl_lex_num_multiplier (const unsigned char c, bool is_bytes) {
	const struct {
		char c;
		long mult_normal;
		long mult_bytes;
	} multipliers[] = {
			{'m', 1000 * 1000, 1024 * 1024},
			{'k', 1000, 1024},
			{'g', 1000 * 1000 * 1000, 1024 * 1024 * 1024}
	};
	int i;

	for (i = 0; i < 3; i ++) {
		if (tolower (c) == multipliers[i].c) {
			if (is_bytes) {
				return multipliers[i].mult_bytes;
			}
			return multipliers[i].mult_normal;
		}
	}

	return 1;
}


/**
 * Return multiplier for time scaling
 * @param c
 * @return
 */
static inline double
ucl_lex_time_multiplier (const unsigned char c) {
	const struct {
		char c;
		double mult;
	} multipliers[] = {
			{'m', 60},
			{'h', 60 * 60},
			{'d', 60 * 60 * 24},
			{'w', 60 * 60 * 24 * 7},
			{'y', 60 * 60 * 24 * 7 * 365}
	};
	int i;

	for (i = 0; i < 5; i ++) {
		if (tolower (c) == multipliers[i].c) {
			return multipliers[i].mult;
		}
	}

	return 1;
}

/**
 * Return true if a character is a end of an atom
 * @param c
 * @return
 */
static inline bool
ucl_lex_is_atom_end (const unsigned char c)
{
	return ucl_test_character (c, UCL_CHARACTER_VALUE_END);
}

static inline bool
ucl_lex_is_comment (const unsigned char c1, const unsigned char c2)
{
	if (c1 == '/') {
		if (c2 == '*') {
			return true;
		}
	}
	else if (c1 == '#') {
		return true;
	}
	return false;
}

/**
 * Check variable found
 * @param parser
 * @param ptr
 * @param remain
 * @param out_len
 * @param strict
 * @param found
 * @return
 */
static inline const char *
ucl_check_variable_safe (struct ucl_parser *parser, const char *ptr, size_t remain,
		size_t *out_len, bool strict, bool *found)
{
	struct ucl_variable *var;

	LL_FOREACH (parser->variables, var) {
		if (strict) {
			if (remain == var->var_len) {
				if (memcmp (ptr, var->var, var->var_len) == 0) {
					*out_len += var->value_len;
					*found = true;
					return (ptr + var->var_len);
				}
			}
		}
		else {
			if (remain >= var->var_len) {
				if (memcmp (ptr, var->var, var->var_len) == 0) {
					*out_len += var->value_len;
					*found = true;
					return (ptr + var->var_len);
				}
			}
		}
	}

	return ptr;
}

/**
 * Check for a variable in a given string
 * @param parser
 * @param ptr
 * @param remain
 * @param out_len
 * @param vars_found
 * @return
 */
static const char *
ucl_check_variable (struct ucl_parser *parser, const char *ptr, size_t remain, size_t *out_len, bool *vars_found)
{
	const char *p, *end, *ret = ptr;
	bool found = false;

	if (*ptr == '{') {
		/* We need to match the variable enclosed in braces */
		p = ptr + 1;
		end = ptr + remain;
		while (p < end) {
			if (*p == '}') {
				ret = ucl_check_variable_safe (parser, ptr + 1, p - ptr - 1, out_len, true, &found);
				if (found) {
					/* {} must be excluded actually */
					ret ++;
					if (!*vars_found) {
						*vars_found = true;
					}
				}
				else {
					*out_len += 2;
				}
				break;
			}
			p ++;
		}
	}
	else if (*ptr != '$') {
		/* Not count escaped dollar sign */
		ret = ucl_check_variable_safe (parser, ptr, remain, out_len, false, &found);
		if (found && !*vars_found) {
			*vars_found = true;
		}
		if (!found) {
			(*out_len) ++;
		}
	}
	else {
		ret ++;
		(*out_len) ++;
	}

	return ret;
}

/**
 * Expand a single variable
 * @param parser
 * @param ptr
 * @param remain
 * @param dest
 * @return
 */
static const char *
ucl_expand_single_variable (struct ucl_parser *parser, const char *ptr,
		size_t remain, unsigned char **dest)
{
	unsigned char *d = *dest;
	const char *p = ptr + 1, *ret;
	struct ucl_variable *var;
	bool found = false;

	ret = ptr + 1;
	remain --;

	if (*p == '$') {
		*d++ = *p++;
		*dest = d;
		return p;
	}
	else if (*p == '{') {
		p ++;
		ret += 2;
		remain -= 2;
	}

	LL_FOREACH (parser->variables, var) {
		if (remain >= var->var_len) {
			if (memcmp (p, var->var, var->var_len) == 0) {
				memcpy (d, var->value, var->value_len);
				ret += var->var_len;
				d += var->value_len;
				found = true;
				break;
			}
		}
	}
	if (!found) {
		memcpy (d, ptr, 2);
		d += 2;
		ret --;
	}

	*dest = d;
	return ret;
}

/**
 * Expand variables in string
 * @param parser
 * @param dst
 * @param src
 * @param in_len
 * @return
 */
static ssize_t
ucl_expand_variable (struct ucl_parser *parser, unsigned char **dst,
		const char *src, size_t in_len)
{
	const char *p, *end = src + in_len;
	unsigned char *d;
	size_t out_len = 0;
	bool vars_found = false;

	p = src;
	while (p != end) {
		if (*p == '$') {
			p = ucl_check_variable (parser, p + 1, end - p - 1, &out_len, &vars_found);
		}
		else {
			p ++;
			out_len ++;
		}
	}

	if (!vars_found) {
		/* Trivial case */
		*dst = NULL;
		return in_len;
	}

	*dst = UCL_ALLOC (out_len + 1);
	if (*dst == NULL) {
		return in_len;
	}

	d = *dst;
	p = src;
	while (p != end) {
		if (*p == '$') {
			p = ucl_expand_single_variable (parser, p, end - p, &d);
		}
		else {
			*d++ = *p++;
		}
	}

	*d = '\0';

	return out_len;
}

/**
 * Store or copy pointer to the trash stack
 * @param parser parser object
 * @param src src string
 * @param dst destination buffer (trash stack pointer)
 * @param dst_const const destination pointer (e.g. value of object)
 * @param in_len input length
 * @param need_unescape need to unescape source (and copy it)
 * @param need_lowercase need to lowercase value (and copy)
 * @param need_expand need to expand variables (and copy as well)
 * @return output length (excluding \0 symbol)
 */
static inline ssize_t
ucl_copy_or_store_ptr (struct ucl_parser *parser,
		const unsigned char *src, unsigned char **dst,
		const char **dst_const, size_t in_len,
		bool need_unescape, bool need_lowercase, bool need_expand)
{
	ssize_t ret = -1, tret;
	unsigned char *tmp;

	if (need_unescape || need_lowercase ||
			(need_expand && parser->variables != NULL) ||
			!(parser->flags & UCL_PARSER_ZEROCOPY)) {
		/* Copy string */
		*dst = UCL_ALLOC (in_len + 1);
		if (*dst == NULL) {
			ucl_set_err (parser->chunks, 0, "cannot allocate memory for a string", &parser->err);
			return false;
		}
		if (need_lowercase) {
			ret = ucl_strlcpy_tolower (*dst, src, in_len + 1);
		}
		else {
			ret = ucl_strlcpy_unsafe (*dst, src, in_len + 1);
		}

		if (need_unescape) {
			ret = ucl_unescape_json_string (*dst, ret);
		}
		if (need_expand) {
			tmp = *dst;
			tret = ret;
			ret = ucl_expand_variable (parser, dst, tmp, ret);
			if (*dst == NULL) {
				/* Nothing to expand */
				*dst = tmp;
				ret = tret;
			}
		}
		*dst_const = *dst;
	}
	else {
		*dst_const = src;
		ret = in_len;
	}

	return ret;
}

/**
 * Create and append an object at the specified level
 * @param parser
 * @param is_array
 * @param level
 * @return
 */
static inline ucl_object_t *
ucl_add_parser_stack (ucl_object_t *obj, struct ucl_parser *parser, bool is_array, int level)
{
	struct ucl_stack *st;

	if (!is_array) {
		if (obj == NULL) {
			obj = ucl_object_typed_new (UCL_OBJECT);
		}
		else {
			obj->type = UCL_OBJECT;
		}
		obj->value.ov = ucl_hash_create ();
		parser->state = UCL_STATE_KEY;
	}
	else {
		if (obj == NULL) {
			obj = ucl_object_typed_new (UCL_ARRAY);
		}
		else {
			obj->type = UCL_ARRAY;
		}
		parser->state = UCL_STATE_VALUE;
	}

	st = UCL_ALLOC (sizeof (struct ucl_stack));
	st->obj = obj;
	st->level = level;
	LL_PREPEND (parser->stack, st);
	parser->cur_obj = obj;

	return obj;
}

int
ucl_maybe_parse_number (ucl_object_t *obj,
		const char *start, const char *end, const char **pos, bool allow_double, bool number_bytes)
{
	const char *p = start, *c = start;
	char *endptr;
	bool got_dot = false, got_exp = false, need_double = false,
			is_date = false, valid_start = false, is_hex = false,
			is_neg = false;
	double dv = 0;
	int64_t lv = 0;

	if (*p == '-') {
		is_neg = true;
		c ++;
		p ++;
	}
	while (p < end) {
		if (is_hex && isxdigit (*p)) {
			p ++;
		}
		else if (isdigit (*p)) {
			valid_start = true;
			p ++;
		}
		else if (!is_hex && (*p == 'x' || *p == 'X')) {
			is_hex = true;
			allow_double = false;
			c = p + 1;
		}
		else if (allow_double) {
			if (p == c) {
				/* Empty digits sequence, not a number */
				*pos = start;
				return EINVAL;
			}
			else if (*p == '.') {
				if (got_dot) {
					/* Double dots, not a number */
					*pos = start;
					return EINVAL;
				}
				else {
					got_dot = true;
					need_double = true;
					p ++;
				}
			}
			else if (*p == 'e' || *p == 'E') {
				if (got_exp) {
					/* Double exp, not a number */
					*pos = start;
					return EINVAL;
				}
				else {
					got_exp = true;
					need_double = true;
					p ++;
					if (p >= end) {
						*pos = start;
						return EINVAL;
					}
					if (!isdigit (*p) && *p != '+' && *p != '-') {
						/* Wrong exponent sign */
						*pos = start;
						return EINVAL;
					}
					else {
						p ++;
					}
				}
			}
			else {
				/* Got the end of the number, need to check */
				break;
			}
		}
		else {
			break;
		}
	}

	if (!valid_start) {
		*pos = start;
		return EINVAL;
	}

	errno = 0;
	if (need_double) {
		dv = strtod (c, &endptr);
	}
	else {
		if (is_hex) {
			lv = strtoimax (c, &endptr, 16);
		}
		else {
			lv = strtoimax (c, &endptr, 10);
		}
	}
	if (errno == ERANGE) {
		*pos = start;
		return ERANGE;
	}

	/* Now check endptr */
	if (endptr == NULL || ucl_lex_is_atom_end (*endptr) || *endptr == '\0') {
		p = endptr;
		goto set_obj;
	}

	if (endptr < end && endptr != start) {
		p = endptr;
		switch (*p) {
		case 'm':
		case 'M':
		case 'g':
		case 'G':
		case 'k':
		case 'K':
			if (end - p >= 2) {
				if (p[1] == 's' || p[1] == 'S') {
					/* Milliseconds */
					if (!need_double) {
						need_double = true;
						dv = lv;
					}
					is_date = true;
					if (p[0] == 'm' || p[0] == 'M') {
						dv /= 1000.;
					}
					else {
						dv *= ucl_lex_num_multiplier (*p, false);
					}
					p += 2;
					goto set_obj;
				}
				else if (number_bytes || (p[1] == 'b' || p[1] == 'B')) {
					/* Bytes */
					if (need_double) {
						need_double = false;
						lv = dv;
					}
					lv *= ucl_lex_num_multiplier (*p, true);
					p += 2;
					goto set_obj;
				}
				else if (ucl_lex_is_atom_end (p[1])) {
					if (need_double) {
						dv *= ucl_lex_num_multiplier (*p, false);
					}
					else {
						lv *= ucl_lex_num_multiplier (*p, number_bytes);
					}
					p ++;
					goto set_obj;
				}
				else if (end - p >= 3) {
					if (tolower (p[0]) == 'm' &&
							tolower (p[1]) == 'i' &&
							tolower (p[2]) == 'n') {
						/* Minutes */
						if (!need_double) {
							need_double = true;
							dv = lv;
						}
						is_date = true;
						dv *= 60.;
						p += 3;
						goto set_obj;
					}
				}
			}
			else {
				if (need_double) {
					dv *= ucl_lex_num_multiplier (*p, false);
				}
				else {
					lv *= ucl_lex_num_multiplier (*p, number_bytes);
				}
				p ++;
				goto set_obj;
			}
			break;
		case 'S':
		case 's':
			if (p == end - 1 || ucl_lex_is_atom_end (p[1])) {
				if (!need_double) {
					need_double = true;
					dv = lv;
				}
				p ++;
				is_date = true;
				goto set_obj;
			}
			break;
		case 'h':
		case 'H':
		case 'd':
		case 'D':
		case 'w':
		case 'W':
		case 'Y':
		case 'y':
			if (p == end - 1 || ucl_lex_is_atom_end (p[1])) {
				if (!need_double) {
					need_double = true;
					dv = lv;
				}
				is_date = true;
				dv *= ucl_lex_time_multiplier (*p);
				p ++;
				goto set_obj;
			}
			break;
		}
	}

	*pos = c;
	return EINVAL;

	set_obj:
	if (allow_double && (need_double || is_date)) {
		if (!is_date) {
			obj->type = UCL_FLOAT;
		}
		else {
			obj->type = UCL_TIME;
		}
		obj->value.dv = is_neg ? (-dv) : dv;
	}
	else {
		obj->type = UCL_INT;
		obj->value.iv = is_neg ? (-lv) : lv;
	}
	*pos = p;
	return 0;
}

/**
 * Parse possible number
 * @param parser
 * @param chunk
 * @return true if a number has been parsed
 */
static bool
ucl_lex_number (struct ucl_parser *parser,
		struct ucl_chunk *chunk, ucl_object_t *obj)
{
	const unsigned char *pos;
	int ret;

	ret = ucl_maybe_parse_number (obj, chunk->pos, chunk->end, (const char **)&pos, true, false);

	if (ret == 0) {
		chunk->remain -= pos - chunk->pos;
		chunk->column += pos - chunk->pos;
		chunk->pos = pos;
		return true;
	}
	else if (ret == ERANGE) {
		ucl_set_err (chunk, ERANGE, "numeric value out of range", &parser->err);
	}

	return false;
}

/**
 * Parse quoted string with possible escapes
 * @param parser
 * @param chunk
 * @return true if a string has been parsed
 */
static bool
ucl_lex_json_string (struct ucl_parser *parser,
		struct ucl_chunk *chunk, bool *need_unescape, bool *ucl_escape, bool *var_expand)
{
	const unsigned char *p = chunk->pos;
	unsigned char c;
	int i;

	while (p < chunk->end) {
		c = *p;
		if (c < 0x1F) {
			/* Unmasked control character */
			if (c == '\n') {
				ucl_set_err (chunk, UCL_ESYNTAX, "unexpected newline", &parser->err);
			}
			else {
				ucl_set_err (chunk, UCL_ESYNTAX, "unexpected control character", &parser->err);
			}
			return false;
		}
		else if (c == '\\') {
			ucl_chunk_skipc (chunk, p);
			c = *p;
			if (p >= chunk->end) {
				ucl_set_err (chunk, UCL_ESYNTAX, "unfinished escape character", &parser->err);
				return false;
			}
			else if (ucl_test_character (c, UCL_CHARACTER_ESCAPE)) {
				if (c == 'u') {
					ucl_chunk_skipc (chunk, p);
					for (i = 0; i < 4 && p < chunk->end; i ++) {
						if (!isxdigit (*p)) {
							ucl_set_err (chunk, UCL_ESYNTAX, "invalid utf escape", &parser->err);
							return false;
						}
						ucl_chunk_skipc (chunk, p);
					}
					if (p >= chunk->end) {
						ucl_set_err (chunk, UCL_ESYNTAX, "unfinished escape character", &parser->err);
						return false;
					}
				}
				else {
					ucl_chunk_skipc (chunk, p);
				}
			}
			*need_unescape = true;
			*ucl_escape = true;
			continue;
		}
		else if (c == '"') {
			ucl_chunk_skipc (chunk, p);
			return true;
		}
		else if (ucl_test_character (c, UCL_CHARACTER_UCL_UNSAFE)) {
			*ucl_escape = true;
		}
		else if (c == '$') {
			*var_expand = true;
		}
		ucl_chunk_skipc (chunk, p);
	}

	ucl_set_err (chunk, UCL_ESYNTAX, "no quote at the end of json string", &parser->err);
	return false;
}

/**
 * Parse a key in an object
 * @param parser
 * @param chunk
 * @return true if a key has been parsed
 */
static bool
ucl_parse_key (struct ucl_parser *parser, struct ucl_chunk *chunk, bool *next_key, bool *end_of_object)
{
	const unsigned char *p, *c = NULL, *end, *t;
	const char *key = NULL;
	bool got_quote = false, got_eq = false, got_semicolon = false,
			need_unescape = false, ucl_escape = false, var_expand = false,
			got_content = false, got_sep = false;
	ucl_object_t *nobj, *tobj;
	ucl_hash_t *container;
	ssize_t keylen;

	p = chunk->pos;

	if (*p == '.') {
		/* It is macro actually */
		ucl_chunk_skipc (chunk, p);
		parser->prev_state = parser->state;
		parser->state = UCL_STATE_MACRO_NAME;
		return true;
	}
	while (p < chunk->end) {
		/*
		 * A key must start with alpha, number, '/' or '_' and end with space character
		 */
		if (c == NULL) {
			if (chunk->remain >= 2 && ucl_lex_is_comment (p[0], p[1])) {
				if (!ucl_skip_comments (parser)) {
					return false;
				}
				p = chunk->pos;
			}
			else if (ucl_test_character (*p, UCL_CHARACTER_WHITESPACE_UNSAFE)) {
				ucl_chunk_skipc (chunk, p);
			}
			else if (ucl_test_character (*p, UCL_CHARACTER_KEY_START)) {
				/* The first symbol */
				c = p;
				ucl_chunk_skipc (chunk, p);
				got_content = true;
			}
			else if (*p == '"') {
				/* JSON style key */
				c = p + 1;
				got_quote = true;
				got_content = true;
				ucl_chunk_skipc (chunk, p);
			}
			else if (*p == '}') {
				/* We have actually end of an object */
				*end_of_object = true;
				return true;
			}
			else if (*p == '.') {
				ucl_chunk_skipc (chunk, p);
				parser->prev_state = parser->state;
				parser->state = UCL_STATE_MACRO_NAME;
				return true;
			}
			else {
				/* Invalid identifier */
				ucl_set_err (chunk, UCL_ESYNTAX, "key must begin with a letter", &parser->err);
				return false;
			}
		}
		else {
			/* Parse the body of a key */
			if (!got_quote) {
				if (ucl_test_character (*p, UCL_CHARACTER_KEY)) {
					got_content = true;
					ucl_chunk_skipc (chunk, p);
				}
				else if (ucl_test_character (*p, UCL_CHARACTER_KEY_SEP)) {
					end = p;
					break;
				}
				else {
					ucl_set_err (chunk, UCL_ESYNTAX, "invalid character in a key", &parser->err);
					return false;
				}
			}
			else {
				/* We need to parse json like quoted string */
				if (!ucl_lex_json_string (parser, chunk, &need_unescape, &ucl_escape, &var_expand)) {
					return false;
				}
				/* Always escape keys obtained via json */
				end = chunk->pos - 1;
				p = chunk->pos;
				break;
			}
		}
	}

	if (p >= chunk->end && got_content) {
		ucl_set_err (chunk, UCL_ESYNTAX, "unfinished key", &parser->err);
		return false;
	}
	else if (!got_content) {
		return true;
	}
	*end_of_object = false;
	/* We are now at the end of the key, need to parse the rest */
	while (p < chunk->end) {
		if (ucl_test_character (*p, UCL_CHARACTER_WHITESPACE)) {
			ucl_chunk_skipc (chunk, p);
		}
		else if (*p == '=') {
			if (!got_eq && !got_semicolon) {
				ucl_chunk_skipc (chunk, p);
				got_eq = true;
			}
			else {
				ucl_set_err (chunk, UCL_ESYNTAX, "unexpected '=' character", &parser->err);
				return false;
			}
		}
		else if (*p == ':') {
			if (!got_eq && !got_semicolon) {
				ucl_chunk_skipc (chunk, p);
				got_semicolon = true;
			}
			else {
				ucl_set_err (chunk, UCL_ESYNTAX, "unexpected ':' character", &parser->err);
				return false;
			}
		}
		else if (chunk->remain >= 2 && ucl_lex_is_comment (p[0], p[1])) {
			/* Check for comment */
			if (!ucl_skip_comments (parser)) {
				return false;
			}
			p = chunk->pos;
		}
		else {
			/* Start value */
			break;
		}
	}

	if (p >= chunk->end && got_content) {
		ucl_set_err (chunk, UCL_ESYNTAX, "unfinished key", &parser->err);
		return false;
	}

	got_sep = got_semicolon || got_eq;

	if (!got_sep) {
		/*
		 * Maybe we have more keys nested, so search for termination character.
		 * Possible choices:
		 * 1) key1 key2 ... keyN [:=] value <- we treat that as error
		 * 2) key1 ... keyN {} or [] <- we treat that as nested objects
		 * 3) key1 value[;,\n] <- we treat that as linear object
		 */
		t = p;
		*next_key = false;
		while (ucl_test_character (*t, UCL_CHARACTER_WHITESPACE)) {
			t ++;
		}
		/* Check first non-space character after a key */
		if (*t != '{' && *t != '[') {
			while (t < chunk->end) {
				if (*t == ',' || *t == ';' || *t == '\n' || *t == '\r') {
					break;
				}
				else if (*t == '{' || *t == '[') {
					*next_key = true;
					break;
				}
				t ++;
			}
		}
	}

	/* Create a new object */
	nobj = ucl_object_new ();
	keylen = ucl_copy_or_store_ptr (parser, c, &nobj->trash_stack[UCL_TRASH_KEY],
			&key, end - c, need_unescape, parser->flags & UCL_PARSER_KEY_LOWERCASE, false);
	if (keylen == -1) {
		ucl_object_free(nobj);
		return false;
	}
	else if (keylen == 0) {
		ucl_set_err (chunk, UCL_ESYNTAX, "empty keys are not allowed", &parser->err);
		ucl_object_free(nobj);
		return false;
	}

	container = parser->stack->obj->value.ov;
	nobj->key = key;
	nobj->keylen = keylen;
	tobj = ucl_hash_search_obj (container, nobj);
	if (tobj == NULL) {
		container = ucl_hash_insert_object (container, nobj);
		nobj->prev = nobj;
		nobj->next = NULL;
		parser->stack->obj->len ++;
	}
	else {
		DL_APPEND (tobj, nobj);
	}

	if (ucl_escape) {
		nobj->flags |= UCL_OBJECT_NEED_KEY_ESCAPE;
	}
	parser->stack->obj->value.ov = container;

	parser->cur_obj = nobj;

	return true;
}

/**
 * Parse a cl string
 * @param parser
 * @param chunk
 * @return true if a key has been parsed
 */
static bool
ucl_parse_string_value (struct ucl_parser *parser,
		struct ucl_chunk *chunk, bool *var_expand, bool *need_unescape)
{
	const unsigned char *p;
	enum {
		UCL_BRACE_ROUND = 0,
		UCL_BRACE_SQUARE,
		UCL_BRACE_FIGURE
	};
	int braces[3][2] = {{0, 0}, {0, 0}, {0, 0}};

	p = chunk->pos;

	while (p < chunk->end) {

		/* Skip pairs of figure braces */
		if (*p == '{') {
			braces[UCL_BRACE_FIGURE][0] ++;
		}
		else if (*p == '}') {
			braces[UCL_BRACE_FIGURE][1] ++;
			if (braces[UCL_BRACE_FIGURE][1] <= braces[UCL_BRACE_FIGURE][0]) {
				/* This is not a termination symbol, continue */
				ucl_chunk_skipc (chunk, p);
				continue;
			}
		}
		/* Skip pairs of square braces */
		else if (*p == '[') {
			braces[UCL_BRACE_SQUARE][0] ++;
		}
		else if (*p == ']') {
			braces[UCL_BRACE_SQUARE][1] ++;
			if (braces[UCL_BRACE_SQUARE][1] <= braces[UCL_BRACE_SQUARE][0]) {
				/* This is not a termination symbol, continue */
				ucl_chunk_skipc (chunk, p);
				continue;
			}
		}
		else if (*p == '$') {
			*var_expand = true;
		}
		else if (*p == '\\') {
			*need_unescape = true;
			ucl_chunk_skipc (chunk, p);
			if (p < chunk->end) {
				ucl_chunk_skipc (chunk, p);
			}
			continue;
		}

		if (ucl_lex_is_atom_end (*p) || (chunk->remain >= 2 && ucl_lex_is_comment (p[0], p[1]))) {
			break;
		}
		ucl_chunk_skipc (chunk, p);
	}

	if (p >= chunk->end) {
		ucl_set_err (chunk, UCL_ESYNTAX, "unfinished value", &parser->err);
		return false;
	}

	return true;
}

/**
 * Parse multiline string ending with \n{term}\n
 * @param parser
 * @param chunk
 * @param term
 * @param term_len
 * @return size of multiline string or 0 in case of error
 */
static int
ucl_parse_multiline_string (struct ucl_parser *parser,
		struct ucl_chunk *chunk, const unsigned char *term,
		int term_len, unsigned char const **beg,
		bool *var_expand)
{
	const unsigned char *p, *c;
	bool newline = false;
	int len = 0;

	p = chunk->pos;

	c = p;

	while (p < chunk->end) {
		if (newline) {
			if (chunk->end - p < term_len) {
				return 0;
			}
			else if (memcmp (p, term, term_len) == 0 && (p[term_len] == '\n' || p[term_len] == '\r')) {
				len = p - c;
				chunk->remain -= term_len;
				chunk->pos = p + term_len;
				chunk->column = term_len;
				*beg = c;
				break;
			}
		}
		if (*p == '\n') {
			newline = true;
		}
		else {
			if (*p == '$') {
				*var_expand = true;
			}
			newline = false;
		}
		ucl_chunk_skipc (chunk, p);
	}

	return len;
}

/**
 * Handle value data
 * @param parser
 * @param chunk
 * @return
 */
static bool
ucl_parse_value (struct ucl_parser *parser, struct ucl_chunk *chunk)
{
	const unsigned char *p, *c;
	ucl_object_t *obj = NULL, *t;
	unsigned int stripped_spaces;
	int str_len;
	bool need_unescape = false, ucl_escape = false, var_expand = false;

	p = chunk->pos;

	while (p < chunk->end) {
		if (obj == NULL) {
			if (parser->stack->obj->type == UCL_ARRAY) {
				/* Object must be allocated */
				obj = ucl_object_new ();
				t = parser->stack->obj->value.av;
				DL_APPEND (t, obj);
				parser->cur_obj = obj;
				parser->stack->obj->value.av = t;
				parser->stack->obj->len ++;
			}
			else {
				/* Object has been already allocated */
				obj = parser->cur_obj;
			}
		}
		c = p;
		switch (*p) {
		case '"':
			ucl_chunk_skipc (chunk, p);
			if (!ucl_lex_json_string (parser, chunk, &need_unescape, &ucl_escape, &var_expand)) {
				return false;
			}
			str_len = chunk->pos - c - 2;
			obj->type = UCL_STRING;
			if ((str_len = ucl_copy_or_store_ptr (parser, c + 1, &obj->trash_stack[UCL_TRASH_VALUE],
					&obj->value.sv, str_len, need_unescape, false, var_expand)) == -1) {
				return false;
			}
			obj->len = str_len;
			parser->state = UCL_STATE_AFTER_VALUE;
			p = chunk->pos;
			return true;
			break;
		case '{':
			/* We have a new object */
			obj = ucl_add_parser_stack (obj, parser, false, parser->stack->level);

			ucl_chunk_skipc (chunk, p);
			return true;
			break;
		case '[':
			/* We have a new array */
			obj = ucl_add_parser_stack (obj, parser, true, parser->stack->level);

			ucl_chunk_skipc (chunk, p);
			return true;
			break;
		case '<':
			/* We have something like multiline value, which must be <<[A-Z]+\n */
			if (chunk->end - p > 3) {
				if (memcmp (p, "<<", 2) == 0) {
					p += 2;
					/* We allow only uppercase characters in multiline definitions */
					while (p < chunk->end && *p >= 'A' && *p <= 'Z') {
						p ++;
					}
					if (*p =='\n') {
						/* Set chunk positions and start multiline parsing */
						c += 2;
						chunk->remain -= p - c;
						chunk->pos = p + 1;
						chunk->column = 0;
						chunk->line ++;
						if ((str_len = ucl_parse_multiline_string (parser, chunk, c,
								p - c, &c, &var_expand)) == 0) {
							ucl_set_err (chunk, UCL_ESYNTAX, "unterminated multiline value", &parser->err);
							return false;
						}
						obj->type = UCL_STRING;
						if ((str_len = ucl_copy_or_store_ptr (parser, c, &obj->trash_stack[UCL_TRASH_VALUE],
							&obj->value.sv, str_len - 1, false, false, var_expand)) == -1) {
							return false;
						}
						obj->len = str_len;
						parser->state = UCL_STATE_AFTER_VALUE;
						return true;
					}
				}
			}
			/* Fallback to ordinary strings */
		default:
			/* Skip any spaces and comments */
			if (ucl_test_character (*p, UCL_CHARACTER_WHITESPACE_UNSAFE) ||
					(chunk->remain >= 2 && ucl_lex_is_comment (p[0], p[1]))) {
				while (p < chunk->end && ucl_test_character (*p, UCL_CHARACTER_WHITESPACE_UNSAFE)) {
					ucl_chunk_skipc (chunk, p);
				}
				if (!ucl_skip_comments (parser)) {
					return false;
				}
				p = chunk->pos;
				continue;
			}
			/* Parse atom */
			if (ucl_test_character (*p, UCL_CHARACTER_VALUE_DIGIT_START)) {
				if (!ucl_lex_number (parser, chunk, obj)) {
					if (parser->state == UCL_STATE_ERROR) {
						return false;
					}
				}
				else {
					parser->state = UCL_STATE_AFTER_VALUE;
					return true;
				}
				/* Fallback to normal string */
			}

			if (!ucl_parse_string_value (parser, chunk, &var_expand, &need_unescape)) {
				return false;
			}
			/* Cut trailing spaces */
			stripped_spaces = 0;
			while (ucl_test_character (*(chunk->pos - 1 - stripped_spaces),
					UCL_CHARACTER_WHITESPACE)) {
				stripped_spaces ++;
			}
			str_len = chunk->pos - c - stripped_spaces;
			if (str_len <= 0) {
				ucl_set_err (chunk, 0, "string value must not be empty", &parser->err);
				return false;
			}
			else if (str_len == 4 && memcmp (c, "null", 4) == 0) {
				obj->len = 0;
				obj->type = UCL_NULL;
			}
			else if (!ucl_maybe_parse_boolean (obj, c, str_len)) {
				obj->type = UCL_STRING;
				if ((str_len = ucl_copy_or_store_ptr (parser, c, &obj->trash_stack[UCL_TRASH_VALUE],
						&obj->value.sv, str_len, need_unescape,
						false, var_expand)) == -1) {
					return false;
				}
				obj->len = str_len;
			}
			parser->state = UCL_STATE_AFTER_VALUE;
			p = chunk->pos;

			return true;
			break;
		}
	}

	return true;
}

/**
 * Handle after value data
 * @param parser
 * @param chunk
 * @return
 */
static bool
ucl_parse_after_value (struct ucl_parser *parser, struct ucl_chunk *chunk)
{
	const unsigned char *p;
	bool got_sep = false;
	struct ucl_stack *st;

	p = chunk->pos;

	while (p < chunk->end) {
		if (ucl_test_character (*p, UCL_CHARACTER_WHITESPACE)) {
			/* Skip whitespaces */
			ucl_chunk_skipc (chunk, p);
		}
		else if (chunk->remain >= 2 && ucl_lex_is_comment (p[0], p[1])) {
			/* Skip comment */
			if (!ucl_skip_comments (parser)) {
				return false;
			}
			/* Treat comment as a separator */
			got_sep = true;
			p = chunk->pos;
		}
		else if (ucl_test_character (*p, UCL_CHARACTER_VALUE_END)) {
			if (*p == '}' || *p == ']') {
				if (parser->stack == NULL) {
					ucl_set_err (chunk, UCL_ESYNTAX, "end of array or object detected without corresponding start", &parser->err);
					return false;
				}
				if ((*p == '}' && parser->stack->obj->type == UCL_OBJECT) ||
						(*p == ']' && parser->stack->obj->type == UCL_ARRAY)) {

					/* Pop all nested objects from a stack */
					st = parser->stack;
					parser->stack = st->next;
					UCL_FREE (sizeof (struct ucl_stack), st);

					while (parser->stack != NULL) {
						st = parser->stack;
						if (st->next == NULL || st->next->level == st->level) {
							break;
						}
						parser->stack = st->next;
						UCL_FREE (sizeof (struct ucl_stack), st);
					}
				}
				else {
					ucl_set_err (chunk, UCL_ESYNTAX, "unexpected terminating symbol detected", &parser->err);
					return false;
				}

				if (parser->stack == NULL) {
					/* Ignore everything after a top object */
					return true;
				}
				else {
					ucl_chunk_skipc (chunk, p);
				}
				got_sep = true;
			}
			else {
				/* Got a separator */
				got_sep = true;
				ucl_chunk_skipc (chunk, p);
			}
		}
		else {
			/* Anything else */
			if (!got_sep) {
				ucl_set_err (chunk, UCL_ESYNTAX, "delimiter is missing", &parser->err);
				return false;
			}
			return true;
		}
	}

	return true;
}

/**
 * Handle macro data
 * @param parser
 * @param chunk
 * @return
 */
static bool
ucl_parse_macro_value (struct ucl_parser *parser,
		struct ucl_chunk *chunk, struct ucl_macro *macro,
		unsigned char const **macro_start, size_t *macro_len)
{
	const unsigned char *p, *c;
	bool need_unescape = false, ucl_escape = false, var_expand = false;

	p = chunk->pos;

	switch (*p) {
	case '"':
		/* We have macro value encoded in quotes */
		c = p;
		ucl_chunk_skipc (chunk, p);
		if (!ucl_lex_json_string (parser, chunk, &need_unescape, &ucl_escape, &var_expand)) {
			return false;
		}

		*macro_start = c + 1;
		*macro_len = chunk->pos - c - 2;
		p = chunk->pos;
		break;
	case '{':
		/* We got a multiline macro body */
		ucl_chunk_skipc (chunk, p);
		/* Skip spaces at the beginning */
		while (p < chunk->end) {
			if (ucl_test_character (*p, UCL_CHARACTER_WHITESPACE_UNSAFE)) {
				ucl_chunk_skipc (chunk, p);
			}
			else {
				break;
			}
		}
		c = p;
		while (p < chunk->end) {
			if (*p == '}') {
				break;
			}
			ucl_chunk_skipc (chunk, p);
		}
		*macro_start = c;
		*macro_len = p - c;
		ucl_chunk_skipc (chunk, p);
		break;
	default:
		/* Macro is not enclosed in quotes or braces */
		c = p;
		while (p < chunk->end) {
			if (ucl_lex_is_atom_end (*p)) {
				break;
			}
			ucl_chunk_skipc (chunk, p);
		}
		*macro_start = c;
		*macro_len = p - c;
		break;
	}

	/* We are at the end of a macro */
	/* Skip ';' and space characters and return to previous state */
	while (p < chunk->end) {
		if (!ucl_test_character (*p, UCL_CHARACTER_WHITESPACE_UNSAFE) && *p != ';') {
			break;
		}
		ucl_chunk_skipc (chunk, p);
	}
	return true;
}

/**
 * Handle the main states of rcl parser
 * @param parser parser structure
 * @param data the pointer to the beginning of a chunk
 * @param len the length of a chunk
 * @return true if chunk has been parsed and false in case of error
 */
static bool
ucl_state_machine (struct ucl_parser *parser)
{
	ucl_object_t *obj;
	struct ucl_chunk *chunk = parser->chunks;
	const unsigned char *p, *c = NULL, *macro_start = NULL;
	unsigned char *macro_escaped;
	size_t macro_len = 0;
	struct ucl_macro *macro = NULL;
	bool next_key = false, end_of_object = false;

	if (parser->top_obj == NULL) {
		if (*chunk->pos == '[') {
			obj = ucl_add_parser_stack (NULL, parser, true, 0);
		}
		else {
			obj = ucl_add_parser_stack (NULL, parser, false, 0);
		}
		parser->top_obj = obj;
		parser->cur_obj = obj;
		parser->state = UCL_STATE_INIT;
	}

	p = chunk->pos;
	while (chunk->pos < chunk->end) {
		switch (parser->state) {
		case UCL_STATE_INIT:
			/*
			 * At the init state we can either go to the parse array or object
			 * if we got [ or { correspondingly or can just treat new data as
			 * a key of newly created object
			 */
			obj = parser->cur_obj;
			if (!ucl_skip_comments (parser)) {
				parser->prev_state = parser->state;
				parser->state = UCL_STATE_ERROR;
				return false;
			}
			else {
				p = chunk->pos;
				if (*p == '[') {
					parser->state = UCL_STATE_VALUE;
					ucl_chunk_skipc (chunk, p);
				}
				else {
					parser->state = UCL_STATE_KEY;
					if (*p == '{') {
						ucl_chunk_skipc (chunk, p);
					}
				}
			}
			break;
		case UCL_STATE_KEY:
			/* Skip any spaces */
			while (p < chunk->end && ucl_test_character (*p, UCL_CHARACTER_WHITESPACE_UNSAFE)) {
				ucl_chunk_skipc (chunk, p);
			}
			if (*p == '}') {
				/* We have the end of an object */
				parser->state = UCL_STATE_AFTER_VALUE;
				continue;
			}
			if (parser->stack == NULL) {
				/* No objects are on stack, but we want to parse a key */
				ucl_set_err (chunk, UCL_ESYNTAX, "top object is finished but the parser "
						"expects a key", &parser->err);
				parser->prev_state = parser->state;
				parser->state = UCL_STATE_ERROR;
				return false;
			}
			if (!ucl_parse_key (parser, chunk, &next_key, &end_of_object)) {
				parser->prev_state = parser->state;
				parser->state = UCL_STATE_ERROR;
				return false;
			}
			if (end_of_object) {
				p = chunk->pos;
				parser->state = UCL_STATE_AFTER_VALUE;
				continue;
			}
			else if (parser->state != UCL_STATE_MACRO_NAME) {
				if (next_key && parser->stack->obj->type == UCL_OBJECT) {
					/* Parse more keys and nest objects accordingly */
					obj = ucl_add_parser_stack (parser->cur_obj, parser, false, parser->stack->level + 1);
				}
				else {
					parser->state = UCL_STATE_VALUE;
				}
			}
			else {
				c = chunk->pos;
			}
			p = chunk->pos;
			break;
		case UCL_STATE_VALUE:
			/* We need to check what we do have */
			if (!ucl_parse_value (parser, chunk)) {
				parser->prev_state = parser->state;
				parser->state = UCL_STATE_ERROR;
				return false;
			}
			/* State is set in ucl_parse_value call */
			p = chunk->pos;
			break;
		case UCL_STATE_AFTER_VALUE:
			if (!ucl_parse_after_value (parser, chunk)) {
				parser->prev_state = parser->state;
				parser->state = UCL_STATE_ERROR;
				return false;
			}
			if (parser->stack != NULL) {
				if (parser->stack->obj->type == UCL_OBJECT) {
					parser->state = UCL_STATE_KEY;
				}
				else {
					/* Array */
					parser->state = UCL_STATE_VALUE;
				}
			}
			else {
				/* Skip everything at the end */
				return true;
			}
			p = chunk->pos;
			break;
		case UCL_STATE_MACRO_NAME:
			if (!ucl_test_character (*p, UCL_CHARACTER_WHITESPACE_UNSAFE)) {
				ucl_chunk_skipc (chunk, p);
			}
			else if (p - c > 0) {
				/* We got macro name */
				macro_len = (size_t)(p - c);
				HASH_FIND (hh, parser->macroes, c, macro_len, macro);
				if (macro == NULL) {
					ucl_create_err (&parser->err, "error on line %d at column %d: "
							"unknown macro: '%.*s', character: '%c'",
								chunk->line, chunk->column, (int)(p - c), c, *chunk->pos);
					parser->state = UCL_STATE_ERROR;
					return false;
				}
				/* Now we need to skip all spaces */
				while (p < chunk->end) {
					if (!ucl_test_character (*p, UCL_CHARACTER_WHITESPACE_UNSAFE)) {
						if (chunk->remain >= 2 && ucl_lex_is_comment (p[0], p[1])) {
							/* Skip comment */
							if (!ucl_skip_comments (parser)) {
								return false;
							}
							p = chunk->pos;
						}
						break;
					}
					ucl_chunk_skipc (chunk, p);
				}
				parser->state = UCL_STATE_MACRO;
			}
			break;
		case UCL_STATE_MACRO:
			if (!ucl_parse_macro_value (parser, chunk, macro,
					&macro_start, &macro_len)) {
				parser->prev_state = parser->state;
				parser->state = UCL_STATE_ERROR;
				return false;
			}
			macro_len = ucl_expand_variable (parser, &macro_escaped, macro_start, macro_len);
			parser->state = parser->prev_state;
			if (macro_escaped == NULL) {
				if (!macro->handler (macro_start, macro_len, macro->ud)) {
					return false;
				}
			}
			else {
				if (!macro->handler (macro_escaped, macro_len, macro->ud)) {
					UCL_FREE (macro_len + 1, macro_escaped);
					return false;
				}
				UCL_FREE (macro_len + 1, macro_escaped);
			}
			p = chunk->pos;
			break;
		default:
			/* TODO: add all states */
			ucl_set_err (chunk, UCL_EINTERNAL, "internal error: parser is in an unknown state", &parser->err);
			parser->state = UCL_STATE_ERROR;
			return false;
		}
	}

	return true;
}

struct ucl_parser*
ucl_parser_new (int flags)
{
	struct ucl_parser *new;

	new = UCL_ALLOC (sizeof (struct ucl_parser));
	memset (new, 0, sizeof (struct ucl_parser));

	ucl_parser_register_macro (new, "include", ucl_include_handler, new);
	ucl_parser_register_macro (new, "try_include", ucl_try_include_handler, new);
	ucl_parser_register_macro (new, "includes", ucl_includes_handler, new);

	new->flags = flags;

	/* Initial assumption about filevars */
	ucl_parser_set_filevars (new, NULL, false);

	return new;
}


void
ucl_parser_register_macro (struct ucl_parser *parser, const char *macro,
		ucl_macro_handler handler, void* ud)
{
	struct ucl_macro *new;

	new = UCL_ALLOC (sizeof (struct ucl_macro));
	memset (new, 0, sizeof (struct ucl_macro));
	new->handler = handler;
	new->name = strdup (macro);
	new->ud = ud;
	HASH_ADD_KEYPTR (hh, parser->macroes, new->name, strlen (new->name), new);
}

void
ucl_parser_register_variable (struct ucl_parser *parser, const char *var,
		const char *value)
{
	struct ucl_variable *new = NULL, *cur;

	if (var == NULL) {
		return;
	}

	/* Find whether a variable already exists */
	LL_FOREACH (parser->variables, cur) {
		if (strcmp (cur->var, var) == 0) {
			new = cur;
			break;
		}
	}

	if (value == NULL) {

		if (new != NULL) {
			/* Remove variable */
			LL_DELETE (parser->variables, new);
			free (new->var);
			free (new->value);
			UCL_FREE (sizeof (struct ucl_variable), new);
		}
		else {
			/* Do nothing */
			return;
		}
	}
	else {
		if (new == NULL) {
			new = UCL_ALLOC (sizeof (struct ucl_variable));
			memset (new, 0, sizeof (struct ucl_variable));
			new->var = strdup (var);
			new->var_len = strlen (var);
			new->value = strdup (value);
			new->value_len = strlen (value);

			LL_PREPEND (parser->variables, new);
		}
		else {
			free (new->value);
			new->value = strdup (value);
			new->value_len = strlen (value);
		}
	}
}

bool
ucl_parser_add_chunk (struct ucl_parser *parser, const unsigned char *data,
		size_t len)
{
	struct ucl_chunk *chunk;

	if (parser->state != UCL_STATE_ERROR) {
		chunk = UCL_ALLOC (sizeof (struct ucl_chunk));
		chunk->begin = data;
		chunk->remain = len;
		chunk->pos = chunk->begin;
		chunk->end = chunk->begin + len;
		chunk->line = 1;
		chunk->column = 0;
		LL_PREPEND (parser->chunks, chunk);
		parser->recursion ++;
		if (parser->recursion > UCL_MAX_RECURSION) {
			ucl_create_err (&parser->err, "maximum include nesting limit is reached: %d",
					parser->recursion);
			return false;
		}
		return ucl_state_machine (parser);
	}

	ucl_create_err (&parser->err, "a parser is in an invalid state");

	return false;
}
