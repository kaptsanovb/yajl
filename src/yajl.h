#include <stdio.h>
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

void yajl_init();

int yajl_parse_int     (char **inputptr, int        *out);
int yajl_parse_float   (char **inputptr, float      *out);
int yajl_parse_bool    (char **inputptr, bool       *out);
int yajl_parse_string  (char **inputptr, string     *out);
int yajl_parse_null    (char **inputptr, null       *out);

int yajl_serialise_int     (CharStack *outputptr, int        *in);
int yajl_serialise_float   (CharStack *outputptr, float      *in);
int yajl_serialise_bool    (CharStack *outputptr, bool       *in);
int yajl_serialise_string  (CharStack *outputptr, const char **in);
int yajl_serialise_null    (CharStack *outputptr, null       *in);

void yajl_free_null  (null        *x);
void yajl_free_int   (int         *x);
void yajl_free_float (float       *x);
void yajl_free_bool  (bool        *x);
void yajl_free_string(const char **x);


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
#define EVAL(...) EVAL8(__VA_ARGS__)

#define FIRST(a, ...) a
#define SECOND(a, b, ...) b
#define TAIL(a, ...) __VA_ARGS__

#define _FST(a, ...) a
#define __FST(a, ...) a
#define FOLDL(m, acc, ...)\
	EVAL(_FST(__VA_OPT__(_FOLDL(m, acc, __VA_ARGS__), )acc))
#define _FOLDL(m, acc, x, ...)\
	__FST(__VA_OPT__(DEFER2(__FOLDL)()(m, m(acc, x), __VA_ARGS__), )m(acc,  x))
#define __FOLDL() _FOLDL


int _yajl_parse_array(
	char **inputptr,
	void *out,
	int (*parse)(char **, void *),
	int dims,
	size_t elem_size
);

int _yajl_serialise_array(
	CharStack *stack,
	void *in,
	int (*serialise)(CharStack *, void *),
	int dims,
	size_t elem_size
);

#define _COUNT(acc, _) acc + 1
#define _LENFIELD(acc, name) acc size_t name;
#define _DO_TYPEDEF_NO(name, x) x
#define _DO_TYPEDEF_YES(name, x) typedef x name
#define _FREE_LOOP_OPEN(acc, name)\
	acc\
		for (size_t name##loopi = 0; name##loopi < x->name; ++name##loopi) {
#define _FREE_LOOP_CLOSE(acc, name)\
		}\
	acc
#define _LOOP_ACCESS(acc, name) (acc) * x->name + name##loopi
#define YAJL_ARRAY_DEFS(name, do_typedef, datatype, ...)\
	_DO_TYPEDEF_##do_typedef(\
		name,\
		struct name {\
			FOLDL(_LENFIELD, , __VA_ARGS__)\
			datatype *data;\
		}\
	);\
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
		/* TODO: Just nested for loops? */\
		return _yajl_serialise_array(\
			outputptr,\
			in,\
			(int (*)(CharStack *, void *))yajl_serialise_##datatype,\
			FOLDL(_COUNT, 0, __VA_ARGS__),\
			sizeof(datatype)\
		);\
	}\
	\
	static inline void yajl_free_##name(struct name *x) {\
		FOLDL(_FREE_LOOP_OPEN, , __VA_ARGS__)\
		yajl_free_##datatype(\
			x->data + FOLDL(\
				_LOOP_ACCESS,\
				CAT(FIRST(__VA_ARGS__), loopi),\
				__VA_ARGS__\
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
	do_typedef,\
	enum_name,\
	enum_do_typedef,\
	...\
)\
	enum enum_name: uint32_t { /* TODO: customisable? */\
		EVAL2(_UEP_EVAL FIRST FOLDL(_UNION_ENUM_FIELD, ((), name), __VA_ARGS__))\
	};\
	_DO_TYPEDEF_##enum_do_typedef(\
		enum_name,\
		enum enum_name\
	);\
	_DO_TYPEDEF_##do_typedef(\
		name,\
		struct name {\
			enum enum_name type;\
			union { FOLDL(_UNION_FIELD, , __VA_ARGS__) };\
		}\
	);\
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


int _yajl_parse_struct(char **inputptr, void *out, int (*)(char **, char *, void *));

#define _OPTIONAL_FIELD_YES(type, name) struct { bool found; type data; } name
#define _OPTIONAL_FIELD_NO(type, name) type name
#define _FIELD(acc, x) acc __FIELD x;
#define __FIELD(type, name, optional)\
	CAT(_OPTIONAL_FIELD_, optional)(type, name)

#define _SND(a, b, ...) b
#define _THD(a, b, c, ...) c
#define CAT(a, b) _CAT(a, b)
#define _CAT(a, b) a##b
#define STR(a) _STR(a)
#define _STR(a) #a

#define _CLEAR_FOUND(acc, x) acc __CLEAR_FOUND x
#define __CLEAR_FOUND_NO(field_name)
#define __CLEAR_FOUND_YES(field_name) out->field_name.found = false;
#define __CLEAR_FOUND(field_type, field_name, is_optional)\
	CAT(__CLEAR_FOUND_, is_optional)(field_name)

#define _FIELD_PARSE(acc, x) acc __FIELD_PARSE x
#define _OPTIONAL_NO(field) field
#define _OPTIONAL_YES(field) field.data
#define _OPTIONAL_FOUND_NO(field)
#define _OPTIONAL_FOUND_YES(field) field.found = true;
#define __FIELD_PARSE(field_type, field_name, is_optional)\
	if (strcmp(field, STR(field_name)) == 0) {\
		if (\
			CAT(yajl_parse_, field_type)(\
				inputptr,\
				&CAT(_OPTIONAL_, is_optional)(out->field_name)\
			) < 0\
		)\
			return -1;\
		\
		CAT(_OPTIONAL_FOUND_, is_optional)(out->field_name)\
	} else

#define _FIELD_FREE(acc, x) acc CAT(__FIELD_FREE_, _THD x) x
#define __FIELD_FREE_YES(field_type, field_name, optional)\
	if (x->field_name.found)\
		yajl_free_##field_type(&x->field_name.data);
#define __FIELD_FREE_NO(field_type, field_name, optional)\
	yajl_free_##field_type(&x->field_name);

#define _ADD_COMMA(...)\
	__VA_ARGS__ __VA_OPT__(if (char_stack_pushc(outputptr, ',') < 0) return -1;)
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
	_ADD_COMMA(acc)\
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

#define YAJL_STRUCT_DEFS(name, do_typedef, ...)\
	_DO_TYPEDEF_##do_typedef(\
		name,\
		struct name { FOLDL(_FIELD, , __VA_ARGS__) }\
	);\
	\
	int _yajl_parse_##name##_field (\
		char **inputptr,\
		char *field,\
		struct name *out\
	) {\
		EVAL1(FIRST FOLDL(_FIELD_PARSE, (, p, m), __VA_ARGS__)) /*else*/\
			return -2;\
		\
		return 0;\
	}\
	\
	int yajl_parse_##name(char **inputptr, struct name *out) {\
		FOLDL(_CLEAR_FOUND, , __VA_ARGS__)\
		return _yajl_parse_struct(\
			inputptr,\
			out,\
			(int (*)(char **, char *, void *))_yajl_parse_##name##_field\
		);\
	}\
	\
	int yajl_serialise_##name(CharStack *outputptr, struct name *in) {\
		if (char_stack_pushc(outputptr, '{') < 0)\
			return -1;\
		\
		const char *x;\
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
