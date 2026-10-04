#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>

#include "yajl.h"

#define UNUSED(x) (void)(x)


unsigned int string_hash(char *s) {
	unsigned int hash = 0;
	for (int i = 0; s[i] != '\0'; ++i) {
		hash = hash * 31 + s[i];
	}
	return hash;
}


// Reading helpers

int skip_whitespace(char **inputptr) {
	char *start = *inputptr;
	while (1) {
		switch (**inputptr) {
			case ' ':
			case '\t':
			case '\n':
				++*inputptr;
				break;

			default:
				return (int)(*inputptr - start);
		}
	}
}

int read_given_char(char **inputptr, char c) {
	char *start = *inputptr;

	skip_whitespace(inputptr);
	if (**inputptr == c) {
		++*inputptr;
		return 0;
	} else {
		*inputptr = start;
		return -1;
	}
}

int read_next_char(char **inputptr, char *c) {
	skip_whitespace(inputptr);
	*c = **inputptr;
	++*inputptr;
	return 0;
}

int read_given_string(char **inputptr, char *s, size_t n) {
	skip_whitespace(inputptr);
	if (strncmp(*inputptr, s, n) != 0)
		return -1;

	*inputptr += n;
	return 0;
}


// Writing helpers

int char_stack_new(CharStack **stackptr, size_t cap) {
	// TODO: is this necessarty
	if (cap < 1)
		return -2;

	*stackptr = malloc(sizeof(CharStack));
	CharStack *stack = *stackptr;

	stack->cap = cap;
	stack->len = 0;
	stack->s = malloc(sizeof(char[cap]));

	if (stack->s == NULL)
		return -1;

	return 0;
}

int char_stack_pushc(CharStack *stack, char x) {
	 if (stack->len == stack->cap) {
	 	stack->cap *= 2;
		stack->s = realloc(stack->s, sizeof(char[stack->cap]));
		if (stack->s == NULL)
			return -1;
	 }

	 stack->s[stack->len] = x;
	 ++stack->len;

	 return 0;
}

#define char_stack_pushs(stack, s) char_stack_pushsn(stack, s, strlen(s))
int char_stack_pushsn(CharStack *stack, const char *s, size_t n) {
	 if (stack->len + n >= stack->cap) {
		do
			stack->cap *= 2;
		while (stack->len + n >= stack->cap);

		stack->s = realloc(stack->s, sizeof(char[stack->cap]));
		if (stack->s == NULL)
			return -1;
	 }

	 memcpy(&stack->s[stack->len], s, n);
	 stack->len += n;

	 return 0;
}

int char_stack_snprintf(CharStack *stack, size_t maxn, char *format, ...) {
	va_list ap;
    va_start(ap, format);

	if (stack->len + maxn >= stack->cap) {
		do
			stack->cap *= 2;
		while (stack->len + maxn >= stack->cap);

		stack->s = realloc(stack->s, sizeof(char[stack->cap]));
		if (stack->s == NULL)
			return -1;
	 }

	int n = vsnprintf(&stack->s[stack->len], maxn, format, ap);
	if (n < 0)
		return -2;
	else if ((size_t)n > maxn)
		return -3;
	else {
		stack->len += n;
		return n;
	}
}

// TODO: maybe pop funcgtions? dont reallyneed them

// INIT


size_t max_len_int;
size_t max_len_float;

__attribute__((constructor)) void _yajl_init() {
	max_len_int   = snprintf(NULL, 0, "%d", INT_MIN);
	max_len_float = snprintf(NULL, 0, "%e", FLT_MIN);
}

// Primitives

int yajl_parse_null(char **inputptr, null *out) {
	UNUSED(out);
	if (read_given_string(inputptr, "null", 4) < 0)
		return -1;

    return 0;
}

int yajl_parse_int(char **inputptr, int *out) {
	int len;
	if (sscanf(*inputptr, " %d%n", out, &len) != 1)
		return -1;

	*inputptr += len;
	return 0;
}

int yajl_parse_float(char **inputptr, float *out) {
	int len;
	if (sscanf(*inputptr, " %f%n", out, &len) != 1)
		return -1;

	*inputptr += len;
	return 0;
}

int yajl_parse_bool(char **inputptr, bool *out) {
	if (read_given_string(inputptr, "false", 5) >= 0)
		*out = false;
	else if (read_given_string(inputptr, "true", 4) >= 0)
		*out = true;
	else
		return -1;

    return 0;
}

int yajl_parse_string(char **inputptr, string *out) {
	int len1 = -1;
	int len2 = -1;
	if (sscanf(*inputptr, " \"%n%*[^\"]\"%n", &len1, &len2) != 0 ||
		len1 == -1 ||
		len2 == -1
	)
		return -1;

	*out = *inputptr + len1;
	*inputptr += len2;
	(*inputptr)[-1] = '\0';
	return 0;
}


// Sorry im british

int yajl_serialise_null(CharStack *outputptr, null *in) {
	UNUSED(in);
	if (char_stack_pushsn(outputptr, "null", 4) < 0)
		return -1;

	return 0;
}

int yajl_serialise_int(CharStack *outputptr, int *in) {
	if (char_stack_snprintf(outputptr, max_len_int, "%d", *in) < 0)
		return -1;

	return 0;
}

int yajl_serialise_float(CharStack *outputptr, float *in) {
	if (char_stack_snprintf(outputptr, max_len_float, "%e", *in) < 0)
		return -1;

	return 0;
}

int yajl_serialise_bool(CharStack *outputptr, bool *in) {
	if (
		(*in
			? char_stack_pushsn(outputptr, "true", 4)
			: char_stack_pushsn(outputptr, "false", 5)
		) < 0
	)
		return -1;

	return 0;
}

int yajl_serialise_string(CharStack *outputptr, char **in) {
	if (char_stack_pushc(outputptr, '"') < 0)
		return -1;

	if (char_stack_pushs(outputptr, *in) < 0)
		return -2;

	if (char_stack_pushc(outputptr, '"') < 0)
		return -3;

	return 0;
}


inline void yajl_free_null(null *x) {
	UNUSED(x);
}

inline void yajl_free_int(int *x) {
	UNUSED(x);
}

inline void yajl_free_float(float *x) {
	UNUSED(x);
}

inline void yajl_free_bool(bool *x) {
	UNUSED(x);
}

inline void yajl_free_string(char **x) {
	UNUSED(x);
}


// Other

int _yajl_parse_array(
	char **inputptr,
	void *out,
	int (*parse)(char **, void *),
	int dims,
	size_t elem_size
) {
	char *start = *inputptr;

	size_t total = 0;
	size_t cap = 1; /* TODO: CHANGE THIS */
	char *data = malloc(cap * elem_size);

	int dim = -1;
	size_t dimi[dims];
	size_t *dimsize = (size_t *)out;
	for (int i = 0; i < dims; ++i)
		dimsize[i] = SIZE_MAX;

	array_start:
	if (read_given_char(inputptr, '[') < 0) {
		*inputptr = start;
		return -1;
	}

	++dim;
	dimi[dim] = 0;

	if (dim < dims - 1)
		goto array_start;
	else
		goto value;

	value:
	if ((*parse)(inputptr, data + elem_size * total) < 0)
		goto array_end;

	++dimi[dim];
	++total;
	if (total == cap) {
		cap *= 2;
		data = realloc(data, cap * elem_size);
	}

	if (read_given_char(inputptr, ',') < 0)
		goto array_end;

	goto value;

	array_end:
	if (read_given_char(inputptr, ']') < 0) {
		*inputptr = start;
		return -2;
	}

	if (dimsize[dim] == SIZE_MAX)
	 	dimsize[dim] = dimi[dim];
	else if (dimsize[dim] != dimi[dim]) {
		*inputptr = start;
		return -3;
	}

	--dim;
	if (dim == -1)
		goto finish;

	++dimi[dim];
	if (read_given_char(inputptr, ',') >= 0)
		goto array_start;
	else
		goto array_end;

	finish:
	*((void **)(out + dims * sizeof(size_t))) = realloc(data, total * elem_size);
	return 0;
}


int _yajl_parse_union(
	char **inputptr,
	void *type_out,
	void *data_out,
	int (**parsers)(char **, void *),
	size_t parser_count
) {
	for (uint32_t i = 0; i < parser_count; ++i) {
		if ((*parsers[i])(inputptr, data_out) >= 0) {
			*(uint32_t *)type_out = i;
			return 0;
		}
	}

	return -1;
}


int _yajl_parse_struct(
	char **inputptr,
	void *out,
	int (*field_parsers[])(char **, char *, int, void *),
	size_t size
) {
	char *start = *inputptr;

	if (read_given_char(inputptr, '{') < 0)
		return -1;

	if (read_given_char(inputptr, '}') >= 0)
		return 0;

	char *s;
	int n;
	int start_n;
	bool first;
	do {
		if (yajl_parse_string(inputptr, &s) < 0) {
			*inputptr = start;
			return -2;
		}

		if (read_given_char(inputptr, ':') < 0) {
			*inputptr = start;
			return -3;
		}

		start_n = string_hash(s) % size;
		n = start_n;
		first = true;

		field_parser_select:
		if (!first && n == start_n)
			return -4;

		first = false;
		while (field_parsers[n] == NULL) {
			n = (n + 1) % size;
			goto field_parser_select;
		}

		switch ((*field_parsers[n])(inputptr, s, n, out)) {
			case -1:
				return -5;

			case -2:
				n = (n + 1) % size;
				goto field_parser_select;

			default:
				break;
		}
	} while (read_given_char(inputptr, ',') >= 0);

	if (read_given_char(inputptr, '}') < 0) {
		*inputptr = start;
		return -6;
	}

	return 0;
}
