CC = /usr/local/bin/gcc-11
CFLAGS = -Wall -Wextra -ggdb3

all: clean code.o compiler.o object.o parser.o runtime.o scanner.o symbol.o symbol_table.o thread.o value.o main.o
	$(CC) $(CFLAGS) code.o compiler.o object.o parser.o runtime.o scanner.o symbol.o symbol_table.o thread.o value.o main.o -o fur

symbol_table_test: symbol.o symbol_table.o symbol_table_test.o
	$(CC) $(CFLAGS) symbol.o symbol_table.o symbol_table_test.o -o symbol_table_test

test: all
	python3 integration_tests.py

clean: clean.o
	rm -f fur
	rm -f symbol_table_test

clean.o:
	rm -f *.o
