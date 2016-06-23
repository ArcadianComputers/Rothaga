win: rothaga.c server.c
	gcc -Wall -o server server.c
	gcc -Wall -o rothaga rothaga.c -lncurses `libgcrypt-config --cflags --libs`
	
mac: rothaga.c server.c
	gcc -Wall -o server server.c -DOSX
	gcc -Wall -o rothaga rothaga.c -DOSX -lncurses `libgcrypt-config --cflags --libs`

mac-debug: rothaga.c server.c
	gcc -Wall -ggdb -o server server.c -DOSX
	gcc -Wall -ggdb -o rothaga rothaga.c -DOSX -lncurses `libgcrypt-config --cflags --libs`

lnx: rothaga.c server.c
	gcc -Wall -o server server.c
	gcc -Wall -o rothaga rothaga.c -lncurses `libgcrypt-config --cflags --libs`

debug: rothaga.c server.c
	gcc -Wall -ggdb -o server server.c
	gcc -Wall -ggdb -o rothaga rothaga.c -lncurses `libgcrypt-config --cflags --libs`

rohasha: rohasha.c
	gcc -Wall -o rohasha rohasha.c `libgcrypt-config --cflags --libs`

clean:
	rm -f rothaga.exe server.exe rohasha.exe rothaga server rohasha
