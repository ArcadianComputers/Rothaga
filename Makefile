all: rothaga.c server.c
	gcc -Wall -o server server.c -DOSX
	gcc -Wall -o rothaga rothaga.c -DOSX
