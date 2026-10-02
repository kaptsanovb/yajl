libyajl.a: bin/yajl.o
	$(AR) rcsT libyajl.a lib/hashmap/libhashmap.a bin/yajl.o

bin/yajl.o: bin lib/hashmap/libhashmap.a src/yajl.c src/yajl.h
	$(CC) -Wall -Wextra -c src/yajl.c -o bin/yajl.o

bin:
	mkdir bin

lib/hashmap/libhashmap.a:
	cmake -S lib/hashmap/ -B lib/hashmap/
	make -C lib/hashmap/

clean:
	rm -r bin
	rm libyajl.a
