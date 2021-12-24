CFLAGS=-g -ansi -pedantic -pthread -Wall -Werror -D_GNU_SOURCE=600
LDFLAGS=-pthread

all: inc mul dpp mem

mul: mul.c
	gcc mul.c -o mul $(LDFLAGS)

inc: inc.c
	gcc inc.c -o inc $(LDFLAGS)

dpp: dpp.c
	gcc dpp.c -o dpp $(LDFLAGS)

mem: mem1.c mem2.c
	gcc mem1.c -o mem1
	gcc mem2.c -o mem2

clean:
	rm -f inc mul dpp mem1 mem2
