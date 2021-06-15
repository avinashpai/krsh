CC=clang
CFLAGS=-g -Wall -Werror
BIN=krsh

all: $(BIN)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	${RM} -r $(BIN) *.o *.dSYM
