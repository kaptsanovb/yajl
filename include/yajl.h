#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>


typedef struct {} null;
typedef char * string;


typedef struct CharStack {
	size_t cap;
	size_t len;
	char *s;
} CharStack;

int char_stack_new(CharStack **stack, size_t len);
int char_stack_pushc(CharStack *stack, char x);
int char_stack_pushs(CharStack *stack, char *x, size_t n);
int char_stack_vsnprintf(CharStack *stack, size_t n, char *format, va_list ap);

int yajl_parse_int     (char **inputptr, int        *out);
int yajl_parse_float   (char **inputptr, float      *out);
int yajl_parse_bool    (char **inputptr, bool       *out);
int yajl_parse_string  (char **inputptr, string     *out);
int yajl_parse_null    (char **inputptr, null       *out);

int yajl_serialise_int     (CharStack *outputptr, int        *in);
int yajl_serialise_float   (CharStack *outputptr, float      *in);
int yajl_serialise_bool    (CharStack *outputptr, bool       *in);
int yajl_serialise_string  (CharStack *outputptr, char **in);
int yajl_serialise_null    (CharStack *outputptr, null       *in);

void yajl_free_null  (null        *x);
void yajl_free_int   (int         *x);
void yajl_free_float (float       *x);
void yajl_free_bool  (bool        *x);
void yajl_free_string(char **x);


#define UNUSED(x) (void)(x)

// Thank you johnathan heathcote: http://jhnet.co.uk/articles/cpp_magic

#define EMPTY()
#define DEFER1(m) m EMPTY()
#define DEFER2(m) m EMPTY EMPTY()()
#define DEFER3(m) m EMPTY EMPTY EMPTY()()()
#define DEFER4(m) m EMPTY EMPTY EMPTY EMPTY()()()()


#define EVAL1(...) __VA_ARGS__
#define EVAL2(...) EVAL1(EVAL1(__VA_ARGS__))
#define EVAL4(...) EVAL2(EVAL2(__VA_ARGS__))
#define EVAL8(...) EVAL4(EVAL4(__VA_ARGS__))
#define EVAL16(...) EVAL8(EVAL8(__VA_ARGS__))
#define EVAL32(...) EVAL16(EVAL16(__VA_ARGS__))
#define EVAL64(...) EVAL32(EVAL32(__VA_ARGS__))
#define EVAL128(...) EVAL64(EVAL64(__VA_ARGS__))
#define EVAL256(...) EVAL128(EVAL128(__VA_ARGS__))
#define EVAL512(...) EVAL256(EVAL256(__VA_ARGS__))
#define EVAL1024(...) EVAL512(EVAL512(__VA_ARGS__))
#define EVAL(...) EVAL1024(__VA_ARGS__)

#define EVAL1_(...) __VA_ARGS__
#define EVAL2_(...) EVAL1_(EVAL1_(__VA_ARGS__))
#define EVAL4_(...) EVAL2_(EVAL2_(__VA_ARGS__))
#define EVAL8_(...) EVAL4_(EVAL4_(__VA_ARGS__))
#define EVAL16_(...) EVAL8_(EVAL8_(__VA_ARGS__))
#define EVAL32_(...) EVAL16_(EVAL16_(__VA_ARGS__))
#define EVAL64_(...) EVAL32_(EVAL32_(__VA_ARGS__))
#define EVAL128_(...) EVAL64_(EVAL64_(__VA_ARGS__))
#define EVAL256_(...) EVAL128_(EVAL128_(__VA_ARGS__))
#define EVAL512_(...) EVAL256_(EVAL256_(__VA_ARGS__))
#define EVAL1024_(...) EVAL512_(EVAL512_(__VA_ARGS__))
#define EVAL_(...) EVAL1024_(__VA_ARGS__)

#define FIRST(a, ...) a
#define SECOND(a, b, ...) b
#define TAIL(a, ...) __VA_ARGS__
#define TAIL3(a, b, c, ...) __VA_ARGS__

#define _FST(a, ...) a
#define __FST(a, ...) a
#define FOLDL(m, acc, ...)\
	EVAL(_FST(__VA_OPT__(_FOLDL(m, acc, __VA_ARGS__), )acc))
#define _FOLDL(m, acc, x, ...)\
	__FST(__VA_OPT__(DEFER2(__FOLDL)()(m, m(acc, x), __VA_ARGS__), )m(acc,  x))
#define __FOLDL() _FOLDL

#define _FST_(a, ...) a
#define __FST_(a, ...) a
#define FOLDL_(m, acc, ...)\
	EVAL_(_FST_(__VA_OPT__(_FOLDL_(m, acc, __VA_ARGS__), )acc))
#define _FOLDL_(m, acc, x, ...)\
	__FST_(__VA_OPT__(DEFER2(__FOLDL_)()(m, m(acc, x), __VA_ARGS__), )m(acc,  x))
#define __FOLDL_() _FOLDL_


int _yajl_parse_array(
	char **inputptr,
	void *out,
	int (*parse)(char **, void *),
	int dims,
	size_t elem_size
);

#define _COUNT(acc, _) acc + 1
#define _LENFIELD(acc, name) acc size_t name;

#define _SERIALISE_LOOP_OPEN(acc, name)\
	acc\
		if (char_stack_pushc(outputptr, '[') < 0)\
			return -1;\
		\
		for (size_t name##loopi = 0; name##loopi < in->name; ++name##loopi) {\
			if (name##loopi != 0 && char_stack_pushc(outputptr, ',') < 0)\
				return -2;
#define _SERIALISE_LOOP_CLOSE(acc, name)\
		}\
		if (char_stack_pushc(outputptr, ']') < 0)\
			return -4;\
	acc

#define _FREE_LOOP_OPEN(acc, name)\
	acc\
		for (size_t name##loopi = 0; name##loopi < x->name; ++name##loopi) {

#define _FREE_LOOP_CLOSE(acc, name)\
		}\
	acc

#define _SERIALISE_LOOP_ACCESS(acc, name) (acc) * in->name + name##loopi
#define _FREE_LOOP_ACCESS(acc, name) (acc) * x->name + name##loopi
#define YAJL_ARRAY_DEFS(name, datatype, ...)\
	typedef struct name {\
		FOLDL(_LENFIELD, , __VA_ARGS__)\
		datatype *data;\
	} name;\
	\
	/* TODO: Inline? */\
	static inline int yajl_parse_##name(char **inputptr, struct name *out) {\
		return _yajl_parse_array(\
			inputptr,\
			out,\
			(int (*)(char **, void *))yajl_parse_##datatype,\
			FOLDL(_COUNT, 0, __VA_ARGS__),\
			sizeof(datatype)\
		);\
	}\
	\
	int yajl_serialise_##name(CharStack *outputptr, struct name *in) {\
		FOLDL(_SERIALISE_LOOP_OPEN, , __VA_ARGS__)\
		if (\
			yajl_serialise_##datatype(\
				outputptr,\
				in->data __VA_OPT__(+ FOLDL(\
					_SERIALISE_LOOP_ACCESS,\
					CAT(FIRST(__VA_ARGS__), loopi),\
					TAIL(__VA_ARGS__)\
				))\
			) < 0\
		)\
			return -3;\
		FOLDL(_SERIALISE_LOOP_CLOSE, , __VA_ARGS__)\
		return 0;\
	}\
	\
	static inline void yajl_free_##name(struct name *x) {\
		FOLDL(_FREE_LOOP_OPEN, , __VA_ARGS__)\
		yajl_free_##datatype(\
			x->data + FOLDL(\
				_FREE_LOOP_ACCESS,\
				CAT(FIRST(__VA_ARGS__), loopi),\
				TAIL(__VA_ARGS__)\
			)\
		);\
		FOLDL(_FREE_LOOP_CLOSE, , __VA_ARGS__)\
		free(x->data);\
	}


int _yajl_parse_union(
	char **inputptr,
	void *type_out,
	void *data_out,
	int (**parsers)(char **, void *),
	size_t parser_count
);

#define _UEF_FST(a, ...) a
#define _UEF_SND(a, b, ...) b
#define _UEF_EVAL(...) __VA_ARGS__
#define CAT3(a, b, c) _CAT3(a, b, c)
#define _CAT3(a, b, c) a##b##c
#define CAT4(a, b, c, d) _CAT4(a, b, c, d)
#define _CAT4(a, b, c, d) a##b##c##d
#define _UNION_FIELD(acc, x) acc __UNION_FIELD x;
#define __UNION_FIELD(field_type, field_name) field_type field_name
#define _UNION_ENUM_FIELD(acc, x)\
	(\
		(_UEF_EVAL _UEF_FST acc CAT3(_UEF_SND acc, _, _UEF_SND x),),\
		_UEF_SND acc\
	)
#define _UEP_EVAL(...) __VA_ARGS__
#define _UNION_PARSER(acc, x) (_UEP_EVAL acc __UNION_PARSER x,)
#define __UNION_PARSER(field_type, field_name)\
	(int (*)(char **, void *))yajl_parse_##field_type
#define _UNION_SERIALISE(acc, x)\
	(\
		_UEF_FST acc\
		case CAT3(_UEF_SND acc, _, _UEF_SND x):\
			CAT(yajl_serialise_, _UEF_FST x)(outputptr, &in->_UEF_SND x);\
			break;\
	,\
		_UEF_SND acc\
	)
#define _UNION_FREE(acc, y)\
	(\
		_UEF_FST acc\
		case CAT3(_UEF_SND acc, _, _UEF_SND y):\
			CAT(yajl_free_, _UEF_FST y)(&x->_UEF_SND y);\
			break;\
	,\
		_UEF_SND acc\
	)
#define YAJL_UNION_DEFS(\
	name,\
	enum_name,\
	...\
)\
	enum enum_name: uint32_t { /* TODO: customisable? */\
		EVAL2(_UEP_EVAL FIRST FOLDL(_UNION_ENUM_FIELD, ((), name), __VA_ARGS__))\
	};\
	typedef enum enum_name enum_name;\
	typedef struct name {\
		enum enum_name type;\
		union { FOLDL(_UNION_FIELD, , __VA_ARGS__) };\
	} name;\
	\
	int yajl_parse_##name(char **inputptr, struct name *out) {\
		int (*parsers[])(char **, void *) = { EVAL1(_UEP_EVAL FOLDL(_UNION_PARSER, (), __VA_ARGS__)) };\
		return _yajl_parse_union(\
			inputptr,\
			out,\
			(uint8_t *)out + offsetof(struct name, SECOND FIRST(__VA_ARGS__)),\
			parsers,\
			FOLDL(_COUNT, 0, __VA_ARGS__)\
		);\
	}\
	\
	int yajl_serialise_##name(CharStack *outputptr, struct name *in) {\
		switch (in->type) {\
			EVAL1(FIRST FOLDL(_UNION_SERIALISE, (, name), __VA_ARGS__))\
			default:\
				return -1;\
		}\
		\
		return 0;\
	}\
	\
	void yajl_free_##name(struct name *x) {\
		switch (x->type) {\
			EVAL1(FIRST FOLDL(_UNION_FREE, (, name), __VA_ARGS__))\
			default:\
				break;\
		}\
	}


unsigned int string_hash(char *s);

int _yajl_parse_struct(
	char **inputptr,
	void *out,
	int (*fields[])(char **, char *, int, void*),
	size_t size
);

#define _OPTIONAL_FIELD_YES(type, name) struct { bool found; type data; } name
#define _OPTIONAL_FIELD_NO(type, name) type name
#define _FIELD(acc, x) acc __FIELD x;
#define __FIELD(type, name, is_optional, ...)\
	CAT(_OPTIONAL_FIELD_, is_optional)(type, name)

#define _SND(a, b, ...) b
#define _THD(a, b, c, ...) c
#define _FRT(a, b, c, d, ...) d
#define CAT(a, b) _CAT(a, b)
#define _CAT(a, b) a##b
#define STR(a) _STR(a)
#define _STR(a) #a

#define _CLEAR_FOUND(acc, x) acc __CLEAR_FOUND x
#define __CLEAR_FOUND_NO(field_name)
#define __CLEAR_FOUND_YES(field_name) out->field_name.found = false;
#define __CLEAR_FOUND(field_type, field_name, is_optional, ...)\
	CAT(__CLEAR_FOUND_, is_optional)(field_name)

#define _FIELD_PARSE_MAP(acc, x)\
	(\
		_UEF_FST acc\
		__FIELD_PARSE_MAP(_SND acc, _THD acc, _UEF_FST x, _SND x, _THD x)\
	,\
		_SND acc\
	,\
		_THD acc\
	)
#define _OPTIONAL_BIT_NO
#define _OPTIONAL_BIT_YES 0b010 |
#define __FIELD_PARSE_MAP(name, size, field_type, field_name, is_optional)\
	n = string_hash(STR(field_name)) % size;\
	while (CAT3(_yajl_, name, _fields[n]) != NULL)\
		n = (n + 1) % size;\
	CAT3(_yajl_, name, _fields[n]) = (int (*)(char **, char *, int, void *))CAT4(_yajl_parse_, name, _, field_name);\
	CAT3(_yajl_, name, _field_checks[n]) = CAT(_OPTIONAL_BIT_, is_optional) 0b100;

#define _FIELD_PARSE(acc, x)\
	(\
		_UEF_FST acc\
		__FIELD_PARSE(_SND acc, _UEF_FST x, _SND x, _THD x, TAIL3 x)\
	,\
		_SND acc\
	)
#define _OPTIONAL_NO(field) field
#define _OPTIONAL_YES(field) field.data
#define _OPTIONAL_FOUND_NO(field)
#define _OPTIONAL_FOUND_YES(field) field.found = true;

#define _CONSTRAINT(acc, constraint) acc __CONSTRAINT constraint
#define __CONSTRAINT(m, ...) m __VA_OPT__((__VA_ARGS__))
#define OR   ||
#define AND  &&
#define EQUAL(value) *data == value
#define STREQUAL(value) strcmp(*data, value) == 0

#define _OPEN (
#define _CLOSE )
#define _CONSTRAINT_DEFAULT(...) (true __VA_OPT__(&& _OPEN) __VA_ARGS__ __VA_OPT__(_CLOSE))

#define __FIELD_PARSE(name, field_type, field_name, is_optional, ...)\
	int CAT4(_yajl_parse_, name, _, field_name)(\
		char **inputptr,\
		char *field,\
		int n,\
		struct name *out\
	) {\
		field_type *data = &CAT(_OPTIONAL_, is_optional)(out->field_name);\
		UNUSED(data);\
		if (strcmp(field, STR(field_name)) == 0) {\
			if (\
				CAT(yajl_parse_, field_type)(\
					inputptr,\
					&CAT(_OPTIONAL_, is_optional)(out->field_name)\
				) < 0\
				|| !_CONSTRAINT_DEFAULT(FOLDL_(_CONSTRAINT, , __VA_ARGS__))\
			)\
				return -1;\
			\
			CAT3(_yajl_, name, _field_checks[n]) |= 0b001;\
			CAT(_OPTIONAL_FOUND_, is_optional)(out->field_name)\
			return 0;\
		}\
		\
		return -2;\
	}

#define _FIELD_FREE(acc, x) acc CAT(__FIELD_FREE_, _THD x) x
#define __FIELD_FREE_YES(field_type, field_name, is_optional, ...)\
	if (x->field_name.found)\
		yajl_free_##field_type(&x->field_name.data);
#define __FIELD_FREE_NO(field_type, field_name, is_optional, ...)\
	yajl_free_##field_type(&x->field_name);

#define _ADD_COMMA(...)\
	__VA_OPT__(,)
#define _FIELD_SERIALISE(acc, x)\
	__FIELD_SERIALISE(acc, _UEF_FST x, _UEF_SND x, _THD x)
#define __FIELD_SERIALISE_OPEN_NO(field_name)
#define __FIELD_SERIALISE_OPEN_YES(field_name)\
	if (in->field_name.found) {
#define __FIELD_SERIALISE_CLOSE_NO(field_name)
#define __FIELD_SERIALISE_CLOSE_YES(field_name)\
	}
#define __FIELD_SERIALISE(acc, field_type, field_name, is_optional)\
	acc\
	CAT(__FIELD_SERIALISE_OPEN_, is_optional)(field_name)\
	\
	if (!first && char_stack_pushc(outputptr, ',') < 0)\
		return -1;\
	first = false;\
	\
	x = STR(field_name);\
	if (yajl_serialise_string(outputptr, &x) < 0)\
		return -2;\
	\
	if (char_stack_pushc(outputptr, ':') < 0)\
		return -3;\
	\
	if (\
		CAT(yajl_serialise_, field_type)(\
			outputptr,\
			CAT(_OPTIONAL_, is_optional)(&in->field_name)\
		) < 0\
	)\
		return -4;\
	\
	CAT(__FIELD_SERIALISE_CLOSE_, is_optional)(field_name)

#define YAJL_FIELDS_EVIL_DO_NOT_USE_THIS ()
#define _ALL_FIELDS(acc, def)\
	(DEFER1(TAIL)(YAJL_FIELDS_##def) _ADD_COMMA acc _UEF_EVAL acc) DEFER1(__ALL_FIELDS)(YAJL_FIELDS_##def)
#define __ALL_FIELDS(extends, ...)\
	_ADD_COMMA extends _UEF_EVAL extends

#define YAJL_STRUCT_DEFS(name)\
	_YAJL_STRUCT_DEFS(\
		name,\
		EVAL1(TAIL FOLDL(_ALL_FIELDS, (), name, EVIL_DO_NOT_USE_THIS))\
	)

#define _YAJL_STRUCT_DEFS(name, ...)\
	typedef struct name { FOLDL(_FIELD, , __VA_ARGS__) } name;\
	\
	char _yajl_##name##_field_checks[FOLDL(_COUNT, 0, __VA_ARGS__)];\
	int (*_yajl_##name##_fields[sizeof(_yajl_##name##_field_checks)])(char **, char *, int, void *);\
	EVAL1(FIRST FOLDL(_FIELD_PARSE, (, name), __VA_ARGS__))\
	__attribute__((constructor)) void _yajl_##name##_init() {\
		int n;\
		EVAL1(FIRST FOLDL(\
			_FIELD_PARSE_MAP,\
			(, name, sizeof(_yajl_##name##_field_checks)),\
			__VA_ARGS__)\
		)\
	}\
	\
	int yajl_parse_##name(char **inputptr, struct name *out) {\
		FOLDL(_CLEAR_FOUND, , __VA_ARGS__)\
		for (size_t i = 0; i < sizeof(_yajl_##name##_field_checks); ++i) {\
			_yajl_##name##_field_checks[i] &= 0b110;\
		}\
		int r = _yajl_parse_struct(\
			inputptr,\
			out,\
			_yajl_##name##_fields,\
			sizeof(_yajl_##name##_field_checks)\
		);\
		if (r < 0)\
			return -1;\
		for (size_t i = 0; i < sizeof(_yajl_##name##_field_checks); ++i) {\
			if (_yajl_##name##_field_checks[i] == 0b100)\
				return -2;\
		}\
		\
		return r;\
	}\
	\
	int yajl_serialise_##name(CharStack *outputptr, struct name *in) {\
		if (char_stack_pushc(outputptr, '{') < 0)\
			return -1;\
		\
		char *x;\
		bool first = true;\
		FOLDL(_FIELD_SERIALISE, , __VA_ARGS__)\
		\
		if (char_stack_pushc(outputptr, '}') < 0)\
			return -5;\
		\
		return 0;\
	}\
	\
	void yajl_free_##name(struct name *x) {\
		FOLDL(_FIELD_FREE, , __VA_ARGS__)\
	}
