typedef struct {} null;
typedef char * string;


typedef struct CharStack {
	size_t cap;
	size_t len;
	char *s;
} CharStack;

