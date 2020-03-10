all: main

main:
	gcc -Wall -o parsing parsing.c -lm

clean:
	rm parsing.exe