all: main

main:
	gcc -Wall -o processing processing.c -lm

clean:
	rm processing processing.exe