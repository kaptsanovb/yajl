libyajl.a: bin/yajl.o
	$(AR) rcs libyajl.a bin/yajl.o

bin/yajl.o: bin src/yajl.c src/yajl.h
	$(CC) -Wall -Wextra -c src/yajl.c -o bin/yajl.o

bin:
	mkdir bin

clean:
	rm -r bin libyajl.a
