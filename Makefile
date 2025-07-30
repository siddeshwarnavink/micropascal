CFLAGS = -Wall -Wextra -ggdb
all: mpas

mpas: mpas.c lexer.c
	$(CC) -o mpas $(CFLAGS) mpas.c lexer.c

clean:
	rm -f mpas
