all: clean code.o compiler.o parser.o runtime.o scanner.o value.o main.o
	gcc {code,compiler,parser,runtime,scanner,value,main}.o -o fur

code.o:
	gcc -c code.c -o code.o

compiler.o:
	gcc -c compiler.c -o compiler.o

main.o:
	gcc -c main.c -o main.o

parser.o:
	gcc -c parser.c -o parser.o

scanner.o:
	gcc -c scanner.c -o scanner.o

runtime.o:
	gcc -c runtime.c -o runtime.o

value.o:
	gcc -c value.c -o value.o

clean: clean.o
	rm -f fur

clean.o:
	rm -f *.o
