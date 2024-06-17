all : tinftest rtgz demo

CFLAGS:=-lz -g -O2

demo : demo.c
	gcc -o $@ $^ $(CFLAGS) -s
	size $@

rtgz : rtgz.c
	gcc -o $@ $^ $(CFLAGS)

tinftest : tinftest.c
	gcc -o $@ $^ $(CFLAGS)

test : tinftest rtgz
	./demo
	./rtgz -c -i /usr/bin/gcc -o gcc_15.gz -w 15 -l 9 -v
	./rtgz -c -i /usr/bin/gcc -o gcc.gz -w 9 -l 9 -v
	./tinftest
	./rtgz -d -i gcc.gz -o gcc.check -w 9 -v
	diff gcc.check /usr/bin/gcc
	rm -rf gcc_15.gz gcc.gz gcc.check

clean :
	rm -rf tinftest rtgz demo
