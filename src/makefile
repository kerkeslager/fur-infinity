all: clean code.o compiler.o object.o parser.o runtime.o scanner.o thread.o value.o main.o
	gcc -Wall -Wimplicit-fallthrough {code,compiler,object,parser,runtime,scanner,thread,value,main}.o -o fur

code.o:
	gcc -c code.c -o code.o

compiler.o:
	gcc -c compiler.c -o compiler.o

main.o:
	gcc -c main.c -o main.o

object.o:
	gcc -c object.c -o object.o

parser.o:
	gcc -c parser.c -o parser.o

parser_test.o:
	gcc -c parser_test.c -o parser_test.o

scanner.o:
	gcc -c scanner.c -o scanner.o

thread.o:
	gcc -c thread.c -o thread.o

runtime.o:
	gcc -c runtime.c -o runtime.o

value.o:
	gcc -c value.c -o value.o

clean: clean.o
	rm -f fur

clean.o:
	rm -f *.o
