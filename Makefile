CFLAGS=-std=c11 -g -static

hcc: src/main.c
				cc -o hcc ./src/main.c
test: hcc
				./test.sh

clean:
				rm -f hcc *.o *~ tmp*

.PHONY: test clean