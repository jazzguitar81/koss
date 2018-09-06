
BIN=app
SOURCE=app.c

all: $(BIN)

clean:
	$(RM) *.o $(BIN)

%.o : %.c
	$(CC) -c -o $@ -pg $<

$(BIN): $(patsubst %.c,%.o,$(SOURCE))
	$(CC) -o $@ $^

.PHONY : all clean
