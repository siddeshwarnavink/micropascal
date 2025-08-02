CFLAGS = -Wall -Wextra -ggdb
all: mpas

mpas: mpas.c lexer.c ast.c codegen.c
	$(CC) -o mpas $(CFLAGS) $^

clean:
	rm -f mpas
