/*
 * rothaga.c
 *
 *  Created on: Apr 16, 2016
 *      Author: Admin
 */

#include "fileio.h"
#include "netio.h"
#include "encio.h"


#define WIN_CFG "C:\\Users\\Admin\\workspace\\Rothaga\\Rothaga.ini"
#define LNX_CFG "./rothaga.ini"

#define ENT_SRC "/dev/random"

int main (int argc, char **argv)
{
	RothagaClient rc;					/*Local Client Structure*/
	printf("Welcome to %s version 0.2 alpha\n",argv[0]);

	/* load_config(CONFIG_FILE) */
	/* find_clients() */
	/* show interface() */
	
	rc.cliname = malloc(NAME_LEN);
		
	if (rc.cliname == NULL)
	{
		perror("malloc(): ");
		printf("Out of RAM\n");
		exit(-1);
	}
	
	memset(rc.cliname, 0, NAME_LEN);
	snprintf(rc.cliname, NAME_LEN-1, argv[1]);

	printf("Client chose name: %s\n", rc.cliname);
		
	load_config(LNX_CFG);

	printf("plaintext = %s, enctext = %s\n",argv[2],encrp(argv[2]));

	return 0;
}

char *encrp(char *plaintext)
{
	int i = 0;
	int fd = 0;
	int len = 0;
	char *encptd = NULL;
	char entropy[256];

	unsigned char c = 0;
		
	if (plaintext == NULL)
	{
		return NULL;
	}
	
	len = strlen(plaintext);

	if (len == 0)
	{
		return NULL;
	}

	encptd = malloc(len*2);
	
	if (encptd == NULL)
	{
		return NULL;
	}

	memset(encptd,0,len*2);
	memset(entropy,0,256);

	fd = open(ENT_SRC,O_RDONLY);

	if (fd == -1)
	{
		return NULL;
	}

	read(fd,entropy,256);
	
	

	for (i = 0; i < len; i++)
	{
		c = plaintext[i];
		c += entropy[i % 256];
		encptd[i] = c;
	}
	encptd[i] = 0;

	return encptd;
}


void load_config(char *cfg)
{
	int fd=-1;

	fd = open(cfg,O_RDONLY);

	if (fd == -1)
	{
		printf("Error opening %s!\n",cfg);
		return;
	}

	printf("Opened config file %s!\n",cfg);

	return;
}
