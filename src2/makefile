all: clean
	gcc -Wall -Wextra -c scanner.c -o scanner.o
	gcc -Wall -Wextra -c main.c -o main.o
	gcc -Wall -Wextra {main,scanner}.o -o fur

test: clean
	gcc -ggdb3 -D DEBUG -Wall -Wextra -c scanner.c -o scanner.o
	gcc -ggdb3 -D DEBUG -Wall -Wextra -c token.c -o token.o
	gcc -ggdb3 -D DEBUG -Wall -Wextra -c scanner_test.c -o scanner_test.o
	gcc -ggdb3 -D DEBUG -Wall -Wextra -c main.c -o main.o
	gcc -ggdb3 -D DEBUG -Wall -Wextra {scanner,scanner_test,token}.o -o scanner_test
	gcc -ggdb3 -D DEBUG -Wall -Wextra {main,scanner,token}.o -o fur
	python test.py

clean:
	rm -f *.o
	rm -f fur
