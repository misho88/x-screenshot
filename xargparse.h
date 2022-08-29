/* Copyright 2021 Mihail Georgiev
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef XARGPARSE_H
#define XARGPARSE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef char const * xap_error_t;
typedef xap_error_t (*xap_assign)(int, char **, void *, int *);

#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

static inline
xap_error_t xap_toggle(int argc, char ** argv, bool * target, int * consumed)
{
	*target = !*target;
	*consumed = 0;
	return NULL;
}

#if defined(__clang__)
	#pragma clang diagnostic pop
#elif defined(__GNUC__)
	#pragma GCC diagnostic pop
#endif

static inline
xap_error_t xap_string(int argc, char ** argv, char const ** target, int * consumed)
{
	*consumed = 0;
	if (argc < 1) return "need another argument";
	*target = argv[0];
	*consumed = 1;
	return NULL;
}

static inline
xap_error_t xap_double(int argc, char ** argv, double * target, int * consumed)
{
	*consumed = 0;
	if (argc < 1) return "need another argument";
	if (argv[0][0] == '\0') return "empty argument";

	char * endptr;
	double value = strtod(argv[0], &endptr);
	if (endptr[0] != '\0') return "not a real number";
	*target = value;
	*consumed = 1;
	return 0;
}

static inline
xap_error_t xap_long(int argc, char ** argv, long * target, int * consumed)
{
	*consumed = 0;
	if (argc < 1) return "need another argument";
	if (argv[0][0] == '\0') return "empty argument";

	char * endptr;
	double value = strtol(argv[0], &endptr, 10);
	if (endptr[0] != '\0') return "not a whole number";
	*target = value;
	*consumed = 1;
	return 0;
}

static inline
xap_error_t xap_int(int argc, char ** argv, int * target, int * consumed)
{
	long tmp;
	xap_error_t error = xap_long(argc, argv, &tmp, consumed);
	if (error) return error;
	*target = tmp;
	return NULL;
}

static inline
xap_error_t xap_float(int argc, char ** argv, float * target, int * consumed)
{
	double tmp;
	xap_error_t error = xap_double(argc, argv, &tmp, consumed);
	if (error) return error;
	*target = tmp;
	return NULL;
}


static inline
int xap_fprint_double(double d, FILE * stream) { return fprintf(stream, "%lg", d); }


static inline
int xap_fprint_long(long l, FILE * stream) { return fprintf(stream, "%ld", l); }


static inline
void xap_fprint_args(int argc, char ** argv, FILE * stream)
{
	for (int i = 0; i < argc; i++)
		fprintf(stream, "'%s'%s", argv[i], i < argc - 1 ? " " : "\n");
}

static inline
xap_error_t xap_shift_args(int origin, int count, int * p_argc, char ** argv)
{
	if (origin + count > *p_argc) return "not enough arguments for requested shift";
	*p_argc -= count;
	for (int i = origin; i < *p_argc; i++) argv[i] = argv[i + count];
	return NULL;
}

#define xap_define_repeat(name, type, func, count) \
	static inline \
	xap_error_t name(int argc, char ** argv, type (*target)[count], int * consumed) \
	{ \
		*consumed = 0; \
		for (int i = 0; i < count; i++) { \
			int loc_consumed; \
			xap_error_t error = func(argc, argv, *target + i, &loc_consumed); \
			if (error) return error; \
			argc -= loc_consumed; \
			argv += loc_consumed; \
			*consumed += loc_consumed; \
		} \
		return NULL; \
	}

typedef struct xap_error_context {
	xap_error_t error;
	char * argument;
	size_t n_parameters;
	char ** parameters;
} xap_error_context_t;

static inline
int xap_fprint_error_context(xap_error_context_t * ctx, FILE * stream)
{
	int cnt = 0;
	cnt += fprintf(stream, "error: %s: %s", ctx->error, ctx->argument);
	for (size_t i = 0; i < ctx->n_parameters; i++)
		cnt += fprintf(stream, " %s", ctx->parameters[i]);
	cnt += fputc('\n', stream) != EOF;
	return cnt;
}

static inline
ssize_t xap_find_int(size_t size, int * array, int item)
{
	for (size_t i = 0; i < size; i++) if (array[i] == item) return i;
	return -1;
}

#define XAP_NO_HELP ""
#define XAP_ALREADY_PARSED 0x7FFFFFFF  /* something else is bound to break before this is a problem */

/* simple transformations from arguments */
#define xap_derive_null_char(...) \
	'\0'
#define xap_derive_null_char_comma(...) \
	xap_derive_null_char(__VA_ARGS__),

#define xap_derive_id(sopt, lopt, ...) \
	((lopt) ? (int)(sopt) : -(int)(sopt))
#define xap_derive_id_comma(sopt, lopt, ...) \
	xap_derive_id(sopt, lopt, __VA_ARGS__), \

#define xap_derive_state_name(sopt, lopt, type, name, arry, conv) \
	SET_ ## name
#define xap_derive_state_name_comma(sopt, lopt, type, name, arry, conv) \
	xap_derive_state_name(sopt, lopt, type, name, arry, conv),

#define xap_derive_field(sopt, lopt, type, name, arry, conv) \
	type name arry;

#define xap_derive_disp(sopt, lopt, disp, desc) \
	disp
#define xap_derive_desc(sopt, lopt, disp, desc) \
	desc

/* generate helpful constants */
#define xap_count(arguments) \
	sizeof((char[]){ arguments(xap_derive_null_char_comma) })
#define xap_ids(arguments) \
	{ arguments(xap_derive_id_comma) }
#define xap_struct(arguments) \
	{ arguments(xap_derive_field) }


/* parser variables */
#define xap_parser_vars(arguments, stop_after, required) \
	bool dirty = false; \
	int i = -1, position = 0; \
	int consumed; \
	char * equal_sign = NULL; \
	size_t lopt_len; \
	int id, ids[] = xap_ids(arguments); \
	size_t n_ids = xap_count(arguments); \
	xap_error_context_t ctx = { 0 };


/* parsing states */
#define xap_derive_state_set_arg(sopt, lopt, type, name, arry, conv) \
	case xap_derive_state_name(sopt, lopt, type, name, arry, conv): \
		id = xap_derive_id(sopt, lopt); \
		if (xap_find_int(n_ids, ids, id) < 0) { \
			ctx.error = "already parsed"; \
			return ctx; \
		} \
		ctx.error = conv(*argc - i, argv + i, &args->name, &consumed); \
		ctx.n_parameters = consumed - (equal_sign != NULL); \
		ctx.parameters = argv + i + (equal_sign != NULL); \
		if (ctx.error) return ctx; \
		dirty &= consumed == 0; \
		ctx.error = xap_shift_args(i, consumed, argc, argv); \
		if (ctx.error) return ctx; \
		if (argv[i][0] == '\0' && i < *argc) { \
			ctx.error = xap_shift_args(i, 1, argc, argv); \
			if (ctx.error) return ctx; \
		} \
		ids[xap_find_int(n_ids, ids, id)] = XAP_ALREADY_PARSED; \
		if (get_stop_after(xap_derive_id(sopt, lopt))) return ctx; \
		state = NEXT_ARG; \
	break;
#define xap_states_set_arg_X(arguments) \
	arguments(xap_derive_state_set_arg); \

#define xap_state_next_arg() \
	case NEXT_ARG: \
		if (dirty && i >= 0 && argv[i][0] != '\0') { state = SOPT; break; } /* y or z in -xyz */ \
		if (*argc == i) { state = CHECK; break; } /* no more arguments */ \
		if (i < 0) i = 0; \
		equal_sign = NULL; \
		ctx.argument = argv[i]; \
		ctx.n_parameters = 0; \
		dirty = false; \
		if (argv[i][0] != '-' || argv[i][1] == '\0') { state = POSITIONAL; break; } /* not a keyword */ \
		argv[i]++; \
		if (argv[i][0] != '-') { state = SOPT; break; } /* x in -xyz */ \
		argv[i]++; \
		if (argv[i][0] != '\0') { state = LOPT; break; } /* arg in --arg */ \
		/* "--" means do not touch other arguments */ \
		argv[i] = ctx.argument; \
		state = CHECK; \
	break;

#define xap_derive_sopt_test(sopt, lopt, type, name, arry, conv) \
	else if (argv[i][0] == sopt) { \
		state = xap_derive_state_name(sopt, lopt, type, name, arry, conv); \
	}
#define xap_state_sopt(arguments) \
	case SOPT: \
		if (0) { } \
		arguments(xap_derive_sopt_test) \
		else { \
			argv[i] = ctx.argument; \
			i++; \
			state = NEXT_ARG; \
			break; \
		} \
		argv[i]++; \
		dirty = argv[i][0] != '\0'; \
		if (!dirty && i != *argc - 1) { \
			ctx.error = xap_shift_args(i, 1, argc, argv); \
			if (ctx.error) return ctx; \
		} \
	break;

#define xap_derive_lopt_test(sopt, lopt, type, name, arry, conv) \
	else if (memcmp(argv[i], lopt ? lopt : "", lopt_len) == 0) { \
		if (equal_sign != NULL) { \
			argv[i] = equal_sign + 1; \
		} \
		else { \
			ctx.error = xap_shift_args(i, 1, argc, argv); \
			if (ctx.error) return ctx; \
		} \
		state = xap_derive_state_name(sopt, lopt, type, name, arry, conv); \
	}
#define xap_state_lopt(arguments) \
	case LOPT: \
		equal_sign = strchr(argv[i], '='); \
		lopt_len = equal_sign != NULL ? (size_t)(equal_sign - argv[i]) : strlen(argv[i]); \
		if (0) { } \
		arguments(xap_derive_lopt_test) \
		else { \
			argv[i] = ctx.argument; \
			i++; \
			state = NEXT_ARG; \
			break; \
		} \
	break;

#define xap_derive_position_test(sopt, lopt, type, name, arry, conv) \
	else if (xap_derive_id(sopt, lopt) == xap_derive_id(position, NULL)) { \
		state = xap_derive_state_name(sopt, lopt, type, name, arry, conv); \
	}
#define xap_state_positional(arguments) \
	case POSITIONAL: \
		if (0) { } \
		arguments(xap_derive_position_test) \
		else { \
			i++; \
			state = NEXT_ARG; \
			break; \
		} \
		position++; \
	break;

#define xap_derive_argument_name(sopt, lopt, type, name, arry, conv) \
	if (ids[n] == xap_derive_id(sopt, lopt)) { \
		argument_name = lopt ? lopt : "at position " #sopt; \
	}

#define xap_derive_check_required_logic(sopt, lopt) \
	if (ids[n] == xap_derive_id(sopt, lopt)) { \
		ctx.error = "argument required"; \
		ctx.argument = argument_name; \
		ctx.n_parameters = 0; \
		return ctx; \
	}
#define xap_state_check(arguments, required) \
	case CHECK: \
		for (size_t n = 0; n < n_ids; n++) { \
			char * argument_name = "???"; \
			arguments(xap_derive_argument_name); \
			required(xap_derive_check_required_logic) \
		} \
		return ctx; \
	break;


/* parser state machine */
#define xap_parser_fsm(arguments, required) \
	enum state { \
		arguments(xap_derive_state_name_comma) \
		NEXT_ARG, \
		POSITIONAL, \
		SOPT, \
		LOPT, \
		CHECK \
	} state = NEXT_ARG; \
	\
	for (;;) switch (state) { \
		xap_states_set_arg_X(arguments) \
		xap_state_next_arg() \
		xap_state_sopt(arguments) \
		xap_state_lopt(arguments) \
		xap_state_positional(arguments) \
		xap_state_check(arguments, required) \
		default: \
			fprintf(stderr, "parser logic is wrong; submit a bug report\n"); \
			exit(-1); \
	}


/* the parser function */
#define xap_declare_parser(name, struct_type) \
	xap_error_context_t name(int * argc, char ** argv, struct_type * args)

#define xap_derive_return_stop_after(sopt, lopt) \
	if (id == xap_derive_id(sopt, lopt)) return true;

#define xap_define_parser(name, struct_type, arguments, stop_after, required) \
	bool xap_get_stop_after_ ## name(int id) \
	{ \
		stop_after(xap_derive_return_stop_after) \
		return false; \
	} \
	xap_declare_parser(name, struct_type) \
	{ \
		bool (*get_stop_after)(int) = xap_get_stop_after_ ## name; \
		xap_parser_vars(arguments, stop_after, required); \
		xap_parser_fsm(arguments, required); \
		return ctx; \
	}

/* usage function */
#define xap_derive_update_is_required(sopt, lopt) \
	if (id == xap_derive_id(sopt, lopt)) is_required = true;

#define xap_derive_update_display_name(sopt, lopt, disp, desc) \
	if (id == xap_derive_id(sopt, lopt) && disp != NULL) display_name = disp;

#define xap_derive_argument_usage_print_lopt(sopt, lopt, type, name, arry, conv) \
	cnt += lopt \
		? fprintf(stream, is_required ? " --%s%s%s" : " [--%s%s%s]", (char *)lopt, space, display_name) \
		: fprintf(stream, is_required ? " %s"       : " [%s]"                           , display_name);
#define xap_derive_argument_usage_print_sopt(sopt, lopt, type, name, arry, conv) \
	cnt += lopt \
		? fprintf(stream, is_required ? " -%c%s%s" : " [-%c%s%s]", sopt, space, display_name) \
		: fprintf(stream, is_required ? " %s"       : " [%s]"                 , display_name);

#define xap_derive_argument_usage_print xap_derive_argument_usage_print_lopt

#define xap_derive_argument_usage(sopt, lopt, type, name, arry, conv) \
	id = xap_derive_id(sopt, lopt); \
	is_required = false; \
	for (int i = 0; !is_required && i < n_required_ids; i++) \
		is_required = required_ids[i] == id; \
	display_name = get_name(id); \
	if (display_name == NULL) display_name = #name #arry; \
	space = display_name[0] == '\0' ? "" : " "; \
	xap_derive_argument_usage_print(sopt, lopt, type, name, arry, conv)

#define xap_declare_fprint_usage(name) \
	int name(FILE * stream)

#define xap_derive_return_name(sopt, lopt, disp, desc) \
	if (id == xap_derive_id(sopt, lopt)) return disp;

#define xap_define_fprint_usage(name, arguments, required, display_hints) \
	char * xap_get_name_ ## name(int id) \
	{ \
		display_hints(xap_derive_return_name) \
		return NULL; \
	} \
	\
	xap_declare_fprint_usage(name) \
	{ \
		char * (*get_name)(int) = xap_get_name_ ## name; \
		const int n_required_ids = xap_count(required); \
		const int required_ids[xap_count(required)] = xap_ids(required); \
		bool is_required; \
		int cnt = 0, id; \
		char * display_name, * space; \
		cnt += fprintf(stream, "usage:"); \
		arguments(xap_derive_argument_usage); \
		cnt += fputc('\n', stream) != EOF; \
		return cnt; \
	}

/* help function */
#define xap_derive_fprint_positional_help(sopt, lopt, type, name, arry, conv) \
	if (!lopt) { \
		int id = xap_derive_id(sopt, lopt); \
		char * desc = get_desc(id); \
		if (desc != NULL && desc[0] != '\0') { \
			char * disp = get_disp(id); \
			cnt += fprintf(stream, "  %-20s %s\n", disp, desc); \
		} \
	};

#define xap_derive_fprint_keyword_help(sopt, lopt, type, name, arry, conv) \
	if (lopt) { \
		int id = xap_derive_id(sopt, lopt); \
		char * desc = get_desc(id); \
		if (desc != NULL && desc[0] != '\0') { \
			char * disp = get_disp(id); \
			int n = 0; \
			n += fprintf(stream, "  -%c", sopt); \
			if (((char *)lopt)[0] != '\0') \
				n += fprintf(stream, ", --%s  ", (char *)lopt); \
			else \
				n += fputs("  ", stream); \
			if (disp) \
				n += fprintf(stream, "%s", disp); \
			cnt += n; \
			cnt += n >= 23 ? fprintf(stream, "\n%*.*s", 23, 23, "") : fprintf(stream, "%*.*s", 23 - n, 23 - n, ""); \
			cnt += fprintf(stream, "%s\n", desc); \
		} \
	};


#define xap_derive_return_desc(sopt, lopt, disp, desc) \
	if (id == xap_derive_id(sopt, lopt)) return desc;
#define xap_derive_return_disp(sopt, lopt, disp, desc) \
	if (id == xap_derive_id(sopt, lopt)) return disp;

#define xap_define_fprint_help(name, arguments, display_hints) \
	char * xap_get_desc_ ## name(int id) \
	{ \
		display_hints(xap_derive_return_desc) \
		return "---"; \
	} \
	\
	char * xap_get_disp_ ## name(int id) \
	{ \
		display_hints(xap_derive_return_disp) \
		return NULL; \
	} \
	\
	xap_declare_fprint_usage(name) \
	{ \
		int cnt = 0; \
		char * (*get_desc)(int) = xap_get_desc_ ## name; \
		char * (*get_disp)(int) = xap_get_disp_ ## name; \
		cnt += fputs("\npositional arguments:\n", stream); \
		arguments(xap_derive_fprint_positional_help) \
		cnt += fputs("\nkeyword arguments:\n", stream); \
		arguments(xap_derive_fprint_keyword_help) \
		return cnt; \
	}

#endif/*XARGPARSE_H*/
