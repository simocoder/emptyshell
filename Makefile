CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O2

all: mtsh
mtsh: mtsh.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f mtsh
.PHONY: all clean
