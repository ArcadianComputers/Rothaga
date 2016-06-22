win: rothaga.c server.c
	gcc -Wall -o server server.c
	gcc -Wall -o rothaga rothaga.c
	
mac: rothaga.c server.c
	gcc -Wall -o server server.c -DOSX
	gcc -Wall -o rothaga rothaga.c -DOSX

mac-debug: rothaga.c server.c
	gcc -Wall -ggdb -o server server.c -DOSX
	gcc -Wall -ggdb -o rothaga rothaga.c -DOSX

lnx: rothaga.c server.c
	gcc -Wall -o server server.c
	gcc -Wall -o rothaga rothaga.c

debug: rothaga.c server.c
	gcc -Wall -ggdb -o server server.c
	gcc -Wall -ggdb -o rothaga rothaga.c

clean:
	rm -f rothaga.exe server.exe rothaga server
