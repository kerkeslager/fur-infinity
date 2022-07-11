all: clean code.o compiler.o parser.o runtime.o scanner.o thread.o value.o main.o
	gcc {code,compiler,parser,runtime,scanner,thread,value,main}.o -o fur

parser_test: clean parser.o parser_test.o scanner.o
	gcc {parser,parser_test,scanner}.o -o parser_test

scanner_test: clean scanner.o scanner_test.o
	gcc {scanner,scanner_test}.o -o scanner_test

code.o:
	gcc -c code.c -o code.o

compiler.o:
	gcc -c compiler.c -o compiler.o

main.o:
	gcc -c main.c -o main.o

parser.o:
	gcc -c parser.c -o parser.o

parser_test.o:
	gcc -c parser_test.c -o parser_test.o

scanner.o:
	gcc -c scanner.c -o scanner.o

scanner_test.o:
	gcc -c scanner_test.c -o scanner_test.o

thread.o:
	gcc -c thread.c -o thread.o

runtime.o:
	gcc -c runtime.c -o runtime.o

value.o:
	gcc -c value.c -o value.o

clean: clean.o
	rm -f parser_test
	rm -f scanner_test
	rm -f fur

clean.o:
	rm -f *.o
