all: clean code.o compiler.o runtime.o value.o main.o
	gcc {code,compiler,runtime,value,main}.o -o fur

code.o:
	gcc -c code.c -o code.o

compiler.o:
	gcc -c compiler.c -o compiler.o

main.o:
	gcc -c main.c -o main.o

runtime.o:
	gcc -c runtime.c -o runtime.o

value.o:
	gcc -c value.c -o value.o

clean: clean.o
	rm -f fur

clean.o:
	rm -f *.o
