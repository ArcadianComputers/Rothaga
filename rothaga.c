/*
 * rothaga.c
 *
 *  Created on: Apr 16, 2016
 *      Author: Jonathan Pollock <jon@arcadiancomputers.com>
 */

#include "fileio.h"
#include "netio.h"
#include "encio.h"
/* #include "/usr/i586-pc-msdosdjgpp/sys-include/conio.h" */

#define WIN_CFG "C:\\Users\\Admin\\workspace\\Rothaga\\Rothaga.ini"
#define LNX_CFG "./rothaga.ini"

#define ENT_SRC "/dev/random"

int main (int argc, char **argv)
{
	int re = 0;						/* return code */
	char x[1],y[1];						/* temp char holders for net/con io */
	struct sockaddr_in sin;					/* network structure */
	RothagaClient rc;					/* local client structure */

	printf("Welcome to %s version 0.4 alpha\n",argv[0]);

	/* load_config(CONFIG_FILE) */
	/* find_clients() */
	/* show interface() */

	memset(&rc,0,sizeof(rc));				/* clear out the client structure */
	
	rc.cliname = malloc(NAME_LEN);
		
	if (rc.cliname == NULL)
	{
		perror("malloc(): ");
		exit(-1);
	}
	
	memset(rc.cliname, 0, NAME_LEN);
	snprintf(rc.cliname, NAME_LEN-1, argv[1]);

	printf("Client chose name: %s\n", rc.cliname);
	/* printf("plaintext = %s, enctext = %s\n",argv[2],encrp(argv[2])); */
		
	load_config(LNX_CFG);

	sin.sin_family	    = AF_INET;
	sin.sin_addr.s_addr = inet_addr(argv[2]);
	sin.sin_port	    = htons(atoi(argv[3]));

	rc.s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	re = connect(rc.s,(struct sockaddr *) &sin,sizeof(sin));

	if (re == -1)
	{
		perror("connect(): ");
		printf("Failed to connect to %s on port %s\n",argv[2],argv[3]);
		return -1;
	}

	printf("Connected to %s:%s\n",argv[2],argv[3]);

	rc.b = malloc(CLI_BUFR);
	rc.k = malloc(CLI_BUFR);

	if (rc.b == NULL || rc.k == NULL)
	{
		perror("malloc(): ");
		exit(-1);
	}

	memset(rc.b,0,CLI_BUFR);
	memset(rc.k,0,CLI_BUFR);

	x[1] = 0;
	y[1] = 0;

	re = fcntl(rc.s,F_SETFL,O_NONBLOCK);

	if (re == -1)
	{
		perror("fcntl(): ");
		return -1;
	}

	re = fcntl(1,F_SETFL,O_NONBLOCK);

	if (re == -1)
	{
		perror("fcntl(): ");
		return -1;
	}

	set_name(&rc,rc.cliname);	/* set our name as soon as we connect */

	while(1)
	{
		re = read(rc.s,x,1);		/* read a byte from the server */
		
		if (re == -1)
		{
			if (errno == EAGAIN) goto rconsole;
			else if (errno == EPIPE || errno == EBADF || errno == ECONNRESET)
			{
				printf("Server closed connection.\n");
				break;
			}
		}

		else
		{
			rc.b[rc.nc] = x[0];
			rc.nc++;

			/* printf(">>> \"%c\" [%i]\n",x[0],x[0]); */

			if ((x[0] == 10) && (rc.b[rc.nc-2] == 13))
			{
				parse_server_message(&rc);
			}
			
		}

		rconsole:

		/* clrscr(); */
		/* printf ("R> "); */

		re = read(1,y,1);		/* read a byte from the console */

		if (re == -1)
		{
			if (errno == EAGAIN) continue;
		}

		else
		{
			rc.k[rc.nk] = y[0];
			rc.nk++;

			if (y[0] == 10)
			{
				parse_console_command(&rc);
			}
		}
	}

	/* sleep(1); */

	close(rc.s);
	free(rc.b);
	free(rc.k);

	return 0;
}

int parse_console_command(RothagaClient *c)
{
	int l = 0;
	char *tmp = NULL;

	tmp = malloc(CLI_BUFR);

	if (tmp == NULL)
	{
		perror("malloc(): ");
		exit(-1);	
	}

	l = strlen(c->k);

	strncpy(tmp,c->k,l-1);		/* skip the trailing new line */

	if (strncmp(tmp,"/quit",5) == 0)
	{
		printf("\n\nAgathor, nooooooooo!\n\n");
		exit(0);
	}

	else if (strncmp(tmp,"/sn",3) == 0)
	{
		set_name(c,tmp+4);
	}

	else
	{
		send_message(c,tmp);
	}

	memset(c->k,0,CLI_BUFR);
	c->nk = 0;
	free(tmp);

	return 0;
}

int set_name(RothagaClient *c, char *cliname)
{
	char *tmp = NULL;

	tmp = malloc(CLI_BUFR);

	if (tmp == NULL)
	{
		perror("malloc(): ");
		exit(-1);
	}

	snprintf(tmp,CLI_BUFR-1,"SN%s\r\n",cliname);

	write_to_server(c,tmp);

	memset(c->cliname,0,NAME_LEN);

	strncpy(c->cliname,cliname,NAME_LEN-1);

	free(tmp);

	return 0;
}

int send_message(RothagaClient *c, char *msg)
{
	char *tmp = NULL;

	tmp = malloc(CLI_BUFR);

	if (tmp == NULL)
	{
		perror("malloc(): ");
		exit(-1);
	}

	snprintf(tmp,CLI_BUFR-1,"SM%s\r\n",msg);

	write_to_server(c,tmp);

	free(tmp);

	return 0;
}

int write_to_server(RothagaClient *c, char *cmd)
{
	int re = -1;

	printf("SENDING: %s",cmd);

	re = write(c->s,cmd,strlen(cmd));

	if (re == -1)
	{
		perror("write() ");
		return -1;
	}

	return 0;
}

int parse_server_message(RothagaClient *c)
{
	int l = 0;
	char *tmp = NULL;

	tmp = malloc(CLI_BUFR);

	if (tmp == NULL)
	{
		perror("malloc(): ");
		exit(-1);
	}

	l = strlen(c->b);

	strncpy(tmp,c->b,l-2);	/* skip the /r/n combo */

	printf("*** %s\n",tmp);

	memset(c->b,0,CLI_BUFR);
	c->nc = 0;

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
