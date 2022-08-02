CC = /usr/local/bin/gcc-11
CFLAGS = -Wall -Wextra -ggdb3

objects: clean code.o compiler.o object.o parser.o read_file.o runtime.o scanner.o symbol.o symbol_table.o thread.o value.o main.o

all: fur fur_scan fur_parse fur_compile

symbol_table_test: objects symbol_table_test.o
	$(CC) $(CFLAGS) symbol.o symbol_table.o symbol_table_test.o -o symbol_table_test

fur: objects main.o
	$(CC) $(CFLAGS) code.o compiler.o object.o parser.o read_file.o runtime.o scanner.o symbol.o symbol_table.o thread.o value.o main.o -o fur

fur_scan: objects fur_scan.o
	$(CC) $(CFLAGS) fur_scan.o read_file.o scanner.o -o fur_scan

fur_parse: objects fur_parse.o
	$(CC) $(CFLAGS) fur_parse.o parser.o read_file.o scanner.o -o fur_parse

fur_compile: objects fur_compile.o
	$(CC) $(CFLAGS) code.o compiler.o fur_compile.o object.o parser.o read_file.o runtime.o scanner.o symbol.o symbol_table.o -o fur_compile

test: all
	python3 integration_tests.py

clean: clean.o
	rm -f fur
	rm -f fur_scan
	rm -f fur_parse
	rm -f fur_compile
	rm -f symbol_table_test

clean.o:
	rm -f *.o
