/*
 * rothaga.c
 *
 *  Created on: Apr 16, 2016
 *      Author: Jonathan Pollock <jon@arcadiancomputers.com>
 */

#include "fileio.h"
#include "netio.h"
#include "encio.h"
#include "agathor.h"
#include "OSXTIME.h"
#include "ralloc.h"
#include "gettok.h"
#include "getmac.h"

/* #include "/usr/i586-pc-msdosdjgpp/sys-include/conio.h" */

#define WIN_CFG "C:\\Users\\Admin\\workspace\\Rothaga\\Rothaga.ini"
#define LNX_CFG "./rothaga.ini"

#define ENT_SRC "/dev/random"

int main (int argc, char **argv)
{
	int re = 0;						/* return code */
	char 
	x[1],y[1];						/* temp char holders for net/con io */
	RothagaClient rc;					/* local client structure */
	RothagaServer rs;					/* remote server structure */

	x[0] = 0;
	y[0] = 0;

	printf("%s\n",agathor);

	printf("Welcome to %s version 0.9 alpha\n To view a list of commands type '/comlist'\n",argv[0]);

	/* load_config(CONFIG_FILE) */
	/* find_clients() */
	/* show interface() */

	if (argc < 4)
	{
		printf("\nUsage: %s <name> <server> <port>\n",argv[0]);
		return -1;
	}

	memset(&rc,0,sizeof(rc));				/* clear out the client structure */
	memset(&rs,0,sizeof(rs));				/* clear out the server structure */
	
	rc.cliname = ralloc(NAME_LEN);
	
	strncpy(rc.cliname, argv[1], NAME_LEN-1);

	rc.argyon = ralloc(NAME_LEN);

	printf("Client chose name: %s\n", rc.cliname);
	/* printf("plaintext = %s, enctext = %s\n",argv[2],encrp(argv[2])); */
		
	load_config(LNX_CFG);

	rs.sin.sin_family	= AF_INET;
	rs.sin.sin_addr.s_addr	= inet_addr(argv[2]);
	rs.sin.sin_port		= htons(atoi(argv[3]));

	net_connect(&rc,&rs);

	set_name(&rc,argv[1]);	/* set our name as soon as we connect */
	send_mac(&rc);

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

int send_mac(RothagaClient *rc)
{
	char *tmp = NULL;
	unsigned char *address = NULL;

	tmp = ralloc(CLI_BUFR);
	address = ralloc(7);

	address[0] = rc->mac_address[0];
	address[1] = rc->mac_address[1];
	address[2] = rc->mac_address[2];
	address[3] = rc->mac_address[3];
	address[4] = rc->mac_address[4];
	address[5] = rc->mac_address[5];
	address[6] = 0;

	snprintf(tmp,CLI_BUFR-1,"Sm%s",address);

	write_to_server(rc,tmp);

	free(tmp);

	return 0;
}

int net_connect(RothagaClient *rc, RothagaServer *rs)
{
	int re = 0;

	rc->s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	if (rc->s == -1)
	{
		perror("socket(): ");
		return -1;
	}

	getmac();

	memcpy(&rc->mac_address,&mac_address,6);

	printf("Client MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
	rc->mac_address[0],rc->mac_address[1],rc->mac_address[2],rc->mac_address[3],rc->mac_address[4],rc->mac_address[5]);

	re = connect(rc->s,(struct sockaddr *) &rs->sin,sizeof(rs->sin));

	if (re == -1)
	{
		perror("connect(): ");
		printf("Failed to connect to %s on port %i\n",inet_ntoa(rs->sin.sin_addr),rs->sin.sin_port);
		return -1;
	}

	printf("Connected to %s:%i\n",inet_ntoa(rs->sin.sin_addr),rs->sin.sin_port);

	rc->b = ralloc(CLI_BUFR);
	rc->k = ralloc(CLI_BUFR);

	re = fcntl(rc->s,F_SETFL,O_NONBLOCK);

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

	return 0;
}

int parse_console_command(RothagaClient *c)
{
	int l = 0;
	char *tmp = NULL;
	char *tmprp = NULL;

	tmp = ralloc(CLI_BUFR);

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

	else if (strncmp(tmp, "/ping",5) == 0)
	{
		ping_server(c,tmp+6);
	}

	else if (strncmp(tmp,"/rp",3) == 0)
	{	
		tmprp = tmp+4;

		printf("If you are sure you want to report <%s>, please type '/yes, if not please /no\n\n",tmprp);

		memset(c->argyon,0,NAME_LEN);
		snprintf(c->argyon,NAME_LEN-1,"%s",tmprp);
		
		c->f = (void *)report_user;
	}
	
	else if (strncmp(tmp,"/comlist",8) == 0)
	{
		printf("The following is a list of commands: \n '/quit' will exit rothaga \n '/rp <user>' will send a report on <user> to everyone on the server, which they can confirm or deny \n '/kg <user>' will gift some of your karma to <user> \n '/ping' will ping the server \n '/sn <name>' will allow you to change your username \n '/yes' and '/no' are used for confirmation for various commands \n");
	}

	else if (strncmp(tmp,"/kg",3) == 0)
	{
		send_karma(c,tmp+4);
	}

	else if (strncmp(tmp,"/yes",4) == 0)
	{
		if (c->f == NULL) goto pcend;
		c->f(c,c->argyon);
	}
	
	else if (strncmp(tmp,"/no",3) == 0)
	{
		if (c->f == NULL) goto pcend;
		
		printf("Guess not then...\n\n");

		if (c->argyon != NULL) free(c->argyon);
		c->f = (void *)NULL;	/* don't leave the gun loaded */
	}

	else
	{
		send_message(c,tmp);
	}

	pcend:

	memset(c->k,0,CLI_BUFR);
	c->nk = 0;
	free(tmp);

	return 0;
}

int send_karma(RothagaClient *c, char *details)
{
	char *tmp = NULL;
	char *cliname = NULL;	
	int karma_amount = 0;

	tmp = ralloc(CLI_BUFR);
	cliname = ralloc(NAME_LEN);

	if ((gettok(details,' ',1) == NULL) || (gettok(details,' ',2) == NULL))
	{
		printf("To gift karma: /kg <name> <amount> Example: /kg Jon 100\n");
	
		return -1;
	}

	strncpy(cliname,gettok(details,' ',1),NAME_LEN-1);

	karma_amount = atoi(gettok(details,' ',2));
	
	snprintf(tmp,CLI_BUFR-1,"KG%s %i",cliname,karma_amount);

	write_to_server(c,tmp);

	free(tmp);

	return 0;
}

int ping_server(RothagaClient *c, char *cliname)
{
	char *tmp = NULL;
	clock_gettime(CLOCK_MONOTONIC, &c->ping);

	if(strlen(cliname) == 0)
	{
		printf("Sending ping\n");
		write_to_server(c,"PN");
	}

	else 
	{
		printf("Sending ping to: %s\n", cliname);

		tmp = ralloc(NAME_LEN);

		snprintf(tmp,NAME_LEN-1,"PN%s",cliname);
		write_to_server(c,tmp);	
	}

	return 0;	
}

int print_pong(RothagaClient *c)
{
	clock_gettime(CLOCK_MONOTONIC, &c->pong);

	printf("Pong: %.03f milliseconds\n",((c->pong.tv_nsec - c->ping.tv_nsec) / 1.0e6));

	return 0;
}

int set_name(RothagaClient *c, char *cliname)
{
	char *tmp = NULL;

	tmp = ralloc(CLI_BUFR);

	snprintf(tmp,CLI_BUFR-1,"SN%s",cliname);

	write_to_server(c,tmp);

	memset(c->cliname,0,NAME_LEN);

	strncpy(c->cliname,cliname,NAME_LEN-1);

	free(tmp);

	return 0;
}

int confirmation_of_report(RothagaClient *c,char *reported)
{
	char *tmp = NULL;

	if (reported == NULL)
	{
		printf("confrimation_of_report() called with null 'reported' variable");
		return -1;
	}

	tmp = ralloc(CLI_BUFR);

	snprintf(tmp,CLI_BUFR-1,"CR%s",reported);

	write_to_server(c,tmp);
	
	free(tmp);

	return 0;
}

int report_user(RothagaClient *c, char *argyon)
{
	char *tmp = NULL;

	tmp = ralloc(CLI_BUFR);

	snprintf(tmp,CLI_BUFR-1,"RP%s",argyon);

	write_to_server(c,tmp);

	free(tmp);

	return 0;
}

int send_message(RothagaClient *c, char *msg)
{
	char *tmp = NULL;

	tmp = ralloc(CLI_BUFR);

	snprintf(tmp,CLI_BUFR-1,"SM%s",msg);

	write_to_server(c,tmp);

	free(tmp);

	return 0;
}

int write_to_server(RothagaClient *c, char *cmd)
{
	int re = -1;

	/* printf("SENDING: %s\n",cmd); */

	re = write(c->s,cmd,strlen(cmd));

	if (re == -1)
	{
		perror("write() ");
		return -1;
	}

	re = write(c->s,"\r\n",2);

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

	tmp = ralloc(CLI_BUFR);

	l = strlen(c->b);

	strncpy(tmp,c->b,l-2);	/* skip the \r\n combo */

	char_cleaner(tmp);

	printf("*** %s\n",tmp);

	if (strncmp(tmp,"RP",2) == 0)
	{
		memset(c->argyon,0,NAME_LEN);
		strncpy(c->argyon,tmp+2,strlen(tmp+2));
		new_report(c,c->argyon);
	}

	else if(strncmp(tmp,"PG",2) == 0)
	{
		print_pong(c);
	}

	memset(c->b,0,CLI_BUFR);
	c->nc = 0;

	free(tmp);

	return 0;
}

void char_cleaner(char *str)
{
	int i = 0;
	int l = strlen(str);

	for(i = 0; i < l; i++)
	{
		if (str[i] <= 31) str[i] = 32;	/* no control sequences */
		else if (str[i] >= 127) str[i] = 32; /* no upper ascii */
	}

	return;
}

int new_report(RothagaClient *c,char *reported)
{
	printf("User %s has been reported, type /yes to agree or /no to disagree.\n",reported);
	c->f = (void *)confirmation_of_report;
	
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

	encptd = ralloc(len*2);
	
	if (encptd == NULL)
	{
		return NULL;
	}

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
