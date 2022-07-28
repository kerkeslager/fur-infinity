CC = gcc
CFLAGS = -Wall -Wextra -ggdb3

all: clean code.o compiler.o object.o parser.o runtime.o scanner.o symbol.o thread.o value.o main.o
	gcc -Wall -Wextra code.o compiler.o object.o parser.o runtime.o scanner.o symbol.o thread.o value.o main.o -o fur

debug: clean code.o compiler.o object.o parser.o runtime.o scanner.o symbol.o thread.o value.o main.o
	gcc -ggdb3 -Wall -Wextra code.o compiler.o object.o parser.o runtime.o scanner.o symbol.o thread.o value.o main.o -o fur

test: debug
	python3 integration_tests.py

clean: clean.o
	rm -f fur

clean.o:
	rm -f *.o
