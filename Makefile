CC      := gcc
CFLAGS  := -Wall -Wextra -O2
LDLIBS  := -luring          # link against liburing

all: mycat

mycat: mycat.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

run: mycat
	./mycat mycat.c

clean:
	rm -f mycat

.PHONY: all run clean
