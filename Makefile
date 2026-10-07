libyajl.a: out/yajl.o
	$(AR) rcs libyajl.a out/yajl.o

out/yajl.o: out src/yajl.c include/yajl.h
	$(CC) -Wall -Wextra -c src/yajl.c -o out/yajl.o

out:
	mkdir out

clean:
	rm -r out libyajl.a
