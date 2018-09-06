CROSS_COMPILE ?=

CC	:= $(CROSS_COMPILE)gcc
CFLAGS	:= -O2 -W -Wall -pg
LDFLAGS	:=
LIBS	:= -lm

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: test.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o
	rm -f test
