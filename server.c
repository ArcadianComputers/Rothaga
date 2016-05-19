/* Rothaga server core <jon@arcadiancomputers.com> */

#include "netio.h"
#include "fileio.h"
#include "encio.h"

RothagaClient *gpc;				/* global SIGPIPE Client pointer */

int main(int argc, char **argv)
{
	int i = 0;				/* loop counter */
	int re = 0;				/* return holder */
	int ra = 1;				/* reuse address */
	int rx = -2;				/* how many characters did we read */
	int ta = 0;				/* temporary accept */
	char x[1];				/* read a character from any socket */

	RothagaServer *s;			/* pointer to current server */
	RothagaClient *c;			/* pointer to current client */
	RothagaServer rs[MAX_SRVS];		/* array of server structures */
	RothagaClient rc[MAX_CLIS];		/* array of client structures */

	signal(SIGINT, kill_server);		/* stop the server on Ctrl-C */
	signal(SIGPIPE, sig_pipe_reset);	/* capture SIGPIPE and reset the correct socket */

	for (i = 0; i < MAX_CLIS; i++) 		/* clear out client and server arrays */
	{
		/* printf("MAX_CLIS = %i, i = %i\n",MAX_CLIS,i); */
		memset(&rc[i],0,sizeof(rc[i]));
	}

	memset(&rs[0],0,sizeof(rs));

	for (i = 0; i < MAX_CLIS; i++) 		/* clear out client and server arrays */
	{
		c = &rc[i];

		c->c = i;			/* set the client id */

		c->s = -2;			/* mark this client slot as free */

		c->b = malloc(CLI_BUFR);
		memset(c->b,0,CLI_BUFR);

		if (c->b == NULL)
		{
			perror("malloc(): ");
			exit(-1);
		}
	}	

	s = &rs[0];				/* pointer to our server */

	s->sp = socket(AF_INET,SOCK_STREAM,0);

	if (s->sp == -1)
	{
		perror("socket()");
		exit(-1);
	}

	printf ("Server socket fd is #%i\n",s->sp);

	re = fcntl(s->sp,F_SETFL,O_NONBLOCK);	/* non-block i/o */

	if (re == -1)
	{
		perror("fcntl(): ");
		return -1;
	}

	if (setsockopt(s->sp,SOL_SOCKET,SO_REUSEADDR,&ra,sizeof(ra)) == -1)
	{
		perror("setsockopt(): ");	/* set the listening port reuseable to prevent TIME_WAIT blocking */
		return -1;
	}

	s->sin.sin_family = AF_INET;		/* address family */
	s->sin.sin_port = htons(SRV_PORT); 	/* tcp port to listen on */
	s->sin.sin_addr.s_addr = INADDR_ANY; 	/* listen on all interfaces */

	re = bind(s->sp, (struct sockaddr *) &s->sin,sizeof(s->sin));
		
	if (re == -1)
	{
		perror("bind()");
		printf("%i is in use\n",SRV_PORT);
		exit(-1);
	}

	re = listen(s->sp,5);			/* hard-wired back log */

	if (re == -1)
	{
		perror("listen()");
		exit(-1);
	}

	while(1)
	{
		ta = accept(s->sp,NULL,NULL);
		
		if (ta == -1)
		{
			if (errno != EAGAIN)
			{
				perror("accept(): ");
			}
		}

		else if (ta != -1)
		{
			c = find_free_client(&rc[0]);	/* allocate a free client slot */

			if (c == NULL)
			{
				printf("No free client slots!\n");
				re = write(ta,"No free client slots!\r\n",23);
				if (re == -1 && errno == EPIPE) printf("Anonymous client broke pipe.\n");
				close(ta);
				goto readarrs;
			}

			c->s = ta;			/* assign socket to client */

			re = fcntl(c->s,F_SETFL,O_NONBLOCK); /* non-block i/o */

			if (re == -1)
			{
				perror("fcntl():");
				exit(-1);
			}

			printf("Client %i has socket #%i\n",c->c,c->s);
		}

		readarrs:

		for (i = 0; i < MAX_CLIS; i++)
		{
			c = &rc[i];

			if (c->s > 0) rx = read(c->s,x,1);

			if (rx == 0)
			{
				printf("Client: %i disconnected!\n",c->c);
				kill_client(c);
				rx = -2;
			}
		
			if ((c->s > 0) && (rx == 1))
			{
				if (c->nc > CLI_BUFR)		/* buffer overflow attempt -Jon */
				{
					printf("Possible Buffer Overflow attempt! Killing client %i on socket #%i\n",c->c,c->s);
					kill_client(c);
					continue;
				}

				c->b[c->nc] = x[0];
				c->nc++;

				/* if (x[0] >= 32) printf("Got a byte [%i]: \"%s\"\t[%i]\n",c->c,x,x[0]);
				else if (x[0] < 32 || x[0] > 126) printf("Got a byte [n:%i]: \"\"\t[%i]\n",c->c,x[0]); */

				if ((x[0] == 10) && (c->b[c->nc-2] == 13)) 
				{
					parse_client_command(rc,c);
				}
			}
		}
	}
}

int parse_client_command(RothagaClient *rc, RothagaClient *c)
{
	int l = 0;
	char cmd[3];

	cmd[0] = 0;
	cmd[1] = 0;
	cmd[2] = 0;

	l = strlen(c->b);

	if (l < 4)
	{
		printf("Malformed command from client: %i on socket #%i\n",c->c,c->s);

		write_client(c,"Malformed command: ");
		write_client(c,c->b);

		goto pcend;
	}

	cmd[0] = c->b[0];
	cmd[1] = c->b[1];

	printf("Client %i said: %s\n",c->c,c->b);

	if (strncmp(cmd,"SM",2) == 0) parse_client_message(rc,c);
	else if (strncmp(cmd,"SN",2) == 0) set_client_name(rc,c);
	else if (strncmp(cmd,"PN",2)==0) send_pong(c);
	else if (strncmp(cmd,"RP",2)==0) send_report(rc,c);

	pcend:

	memset(c->b,0,CLI_BUFR);

	c->nc = 0;

	return 0;
}

int set_client_name(RothagaClient *rc, RothagaClient *c)
{
	int i = 0;		/* incrementation counter */
	int l = 0;		/* length counter */
	char *cm = NULL;
	char *tmp = NULL;
	char *cptr = NULL;

	cptr = c->b;
	cptr += 2;		/* increment depth into c->b */
	l = strlen(cptr);	/* take length of client name */

	tmp = malloc(NAME_LEN);
	
	if (tmp == NULL)
	{
		perror("malloc(): ");

		if(c->cliname == NULL)
		{
			printf("Out of RAM! Killing client %i\n", c->c);
			write_client(c,"0MOut of RAM, closing connection.");
			kill_client(c);
			return -1;
		}

		else 
		{
			printf("Out of RAM! Cannot change name for Client %i\n",c->c);
			write_client(c,"0MCannot process request.");
			return -1;
		}
	}

	else 
	{
		if (l-2 < MIN_NAME_LEN) /*Checking if the name is too short*/
		{
			write_client(c,"0NName too short."); 
			return -1;
		}

		else if (l > (NAME_LEN-2))	/* name too long */
		{
			write_client(c,"9NName too long.");
			return -1;
		}
		for(i = 0, i<l-2, i++)
		{
			if(cisin(cliname[i]) == -1)
			{
				write_client(c,"1NName may only be alphanumeric.");
				return -1;	
			}	
		} 
		strncpy(tmp, cptr, NAME_LEN-1);		
	}

	cm = malloc(l+NAME_LEN);
	memset(cm,0,l+NAME_LEN);

	if (c->cliname == NULL)
	{
		snprintf(cm,l+NAME_LEN,"<%i> is now %s",c->c,tmp);
		c->cliname = malloc(NAME_LEN);
		memset(c->cliname,0,NAME_LEN);
		strncpy(c->cliname,tmp,l-2);
	}

	else
	{
		snprintf(cm,l+256,"<%s> is now %s",c->cliname,tmp);
		memset(c->cliname,0,strlen(c->cliname));
		strncpy(c->cliname,tmp,l-2);		
	}

	l = strlen(cm);
	
	for (i = 0; i < MAX_CLIS; i++)
	{
		if (rc[i].s > 0)
		{
			write_client(&rc[i],cm);
		}  	
	}

	free(cm);
	free(tmp);

	return 0;	
}

int send_report(RothagaClient *rc, RothagaClient *c)
{
	int i = 0;		/* incrementation counter */
	int l = 0;		/* length counter */
	char *cm = NULL;
	char *tmp = NULL;
	char *cptr = NULL;

	cptr = c->b;
	cptr += 2;		/* increment depth into c->b */
	l = strlen(cptr);	/* take length of reported client name */

	tmp = malloc(NAME_LEN);

	if (tmp == NULL)
	{
		perror("malloc(): ");

		printf("Out of RAM! Cannot report %s\n",cptr);
		write_client(c,"0MCannot process request.");
		return -1;
		
	}

	else 
	{
		if (l > (NAME_LEN-2))	/* name too long */
		{
			write_client(c,"9NName too long.");
			return -1;
		}

		memset(tmp,0,NAME_LEN);

		strncpy(tmp, cptr, l-2);		
	}
	
	cm = malloc(CLI_BUFR);
	
	if (cm == NULL)
	{
		perror("malloc(): ");

		printf("Out of RAM! Cannot send report to users\n");
		write_client(c,"0MCannot process request.");
		return -1;
	}

	memset(cm,0,CLI_BUFR);

	snprintf(cm,CLI_BUFR-1,"RP%s",tmp);

	for (i = 0; i < MAX_CLIS; i++)
	{
		if (rc[i].s > 0)
		{
			write_client(&rc[i],cm);
		}  	
	}
	
	return 0;

}



int parse_client_message(RothagaClient *rc, RothagaClient *c)
{
	int i = 0;
	int l = 0;
	char *cm = NULL;
	char *cptr = NULL;

	if(c->cliname == NULL)
	{
		write_client(c,"0NYou must assign a name with SN<name>.");

		return -1;
	}

	l = strlen(c->b);
	l -= 2;
	cptr = c->b;
	cptr += 2;

	cm = malloc(l+256);

	memset(cm,0,l+256);

	snprintf(cm,l+256,"<%s> %s",c->cliname,cptr); 

	l = strlen(cm);
	
	for (i = 0; i < MAX_CLIS; i++)
	{
		if (rc[i].s > 0)
		{
			printf("%s\n",cm);
			write_client(&rc[i],cm);
		}  	
	}

	return 0;
}

int send_pong(RothagaClient *c)
{
	write_client(c, "PG");

	return 0;
}
RothagaClient *find_free_client(RothagaClient *rc)
{
	int i = 0;
	RothagaClient *c;

	for(i = 0; i < MAX_CLIS; i++)
	{
		c = &rc[i];
		/* printf("Client: %i has socket #%i\n",c->c,c->s); */
		if (c->s == -2) return c;
	}

	return NULL;
}

int kill_client(RothagaClient *c)
{
	printf("Killing client %i, socket #%i!\n",c->c,c->s);

	close(c->s);			/* close the socket */

	memset(c->b,0,CLI_BUFR);	/* clear the com buffer */

	if (c->cliname != NULL)		/* remove client name */
	{
		memset(c->cliname,0,NAME_LEN);
		free(c->cliname);
	}

	c->s = -2;  			/* make sure we set it as free */

	return 0;
}

int write_client(RothagaClient *c, char *str)
{
	int re = 0;			/* return value */

	int l = strlen(str);		/* length of data to write */

	gpc = c;			/* associate c with gpc for SIGPIPE handling */
	re = write(c->s,str,l);

	if (re == -1)
	{
		if (errno == EPIPE)
		{
			printf("Caught EPIPE from failed write to client %i!\n",c->c);
		}
	}

	re = write(c->s,"\r\n",2);

	if (re == -1)
	{
		if (errno == EPIPE)
		{
			printf("Caught EPIPE from failed write to client %i!\n",c->c);
		}
	}

	return 0;
}

void kill_server(int sig)
{
	printf("\n\nCaught Ctrl-C (%i), shutting down!\n\n",sig); 
	exit(sig);
}

int cisin(char c)
{
	int i = 0;
	int l = strlen(ALLOWED);

	for(i = 0; i<l; i++)
	{
		if(c == ALLOWED[i]) return 0; 
	}

	return -1;
}

void sig_pipe_reset(int sig)
{
	printf("\n\nCaught SIGPIPE (%i) from Client %i!\n\n",sig,gpc->c);
	kill_client(gpc);
	return;
}