all: clean code.o compiler.o object.o parser.o scanner.o thread.o value.o main.o
	gcc -Wall -Wextra code.o compiler.o object.o parser.o scanner.o thread.o value.o main.o -o fur

debug: clean code.o compiler.o object.o parser.o scanner.o thread.o value.o main.o
	gcc -ggdb3 -Wall -Wextra code.o compiler.o object.o parser.o scanner.o thread.o value.o main.o -o fur

test: debug
	python3 integration_tests.py

code.o:
	gcc -ggdb3 -c code.c -o code.o

compiler.o:
	gcc -ggdb3 -c compiler.c -o compiler.o

main.o:
	gcc -ggdb3 -c main.c -o main.o

object.o:
	gcc -ggdb3 -c object.c -o object.o

parser.o:
	gcc -ggdb3 -c parser.c -o parser.o

parser_test.o:
	gcc -ggdb3 -c parser_test.c -o parser_test.o

scanner.o:
	gcc -ggdb3 -c scanner.c -o scanner.o

thread.o:
	gcc -ggdb3 -c thread.c -o thread.o

value.o:
	gcc -ggdb3 -c value.c -o value.o

clean: clean.o
	rm -f fur

clean.o:
	rm -f *.o
