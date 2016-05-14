/* Rothaga server core <jon@arcadiancomputers.com> */

#include "netio.h"
#include "fileio.h"
#include "encio.h"

#define SRV_PORT 7117				/* TCP port we listen on */
#define CLI_BUFR 1024				/* size of client buffer */
#define MAX_CLIS 256				/* maximum number of clients */
#define MAX_SRVS 1				/* maximum number of servers */

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
	signal(SIGPIPE, SIG_IGN);		/* ignore SIGPIPE and handle it in errno */

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
		usleep(1);			/* busy waiting */

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
				if (re == -1 && errno == EPIPE) printf("Anonymous client disconnected.\n");
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
	int re = 0;
	int l = 0;
	char cmd[3];

	cmd[0] = 0;
	cmd[1] = 0;
	cmd[2] = 0;

	l = strlen(c->b);

	if (l < 4)
	{
		printf("Malformed command from client: %i on socket #%i\n",c->c,c->s);

		re = write(c->s,"Malformed command: ",19);
		if (re == -1 && errno == EPIPE)
		{
			printf("Client %i disconnected abruptly.\n",c->c);
			kill_client(c);
		}

		re = write(c->s,c->b,l);
		if (re == -1 && errno == EPIPE)
		{
			printf("Client %i disconnected abruptly.\n",c->c);
			kill_client(c);
		}

		re = write(c->s,"\r\n",2);
		if (re == -1 && errno == EPIPE)
		{
			printf("Client %i disconnected abruptly.\n",c->c);
			kill_client(c);
		}

		goto pcend;
	}

	cmd[0] = c->b[0];
	cmd[1] = c->b[1];

	if (strncmp(cmd,"SM",2) == 0) parse_client_message(rc,c);
	else if (strncmp(cmd,"SN",2) == 0) set_client_name(rc,c);
	
	printf("Client %i said: %s\n",c->c,c->b);

	pcend:

	memset(c->b,0,CLI_BUFR);

	c->nc = 0;

	return 0;
}

int set_client_name(RothagaClient *rc, RothagaClient *c)
{
	int re = 0;		/* return value */
	int i = 0;		/* incrementation counter */
	int l = 0;		/* length counter */
	char *cm = NULL;
	char *tmp = NULL;

	c->b += 2;		/* increment depth into c->b */
	l = strlen(c->b);	/* take length of client name */

	tmp = malloc(NAME_LEN);
	
	if (tmp == NULL)
	{
		perror("malloc(): ");

		if(c->cliname == NULL)
		{
			printf("Out of RAM! Killing client %i\n", c->c);
			re = write(c->s, "Out of RAM, closing connection.", 31);
			if (re == -1 && errno == EPIPE) printf("Client %i disconnected abrutply.\n",c->c);
			kill_client(c);

			return -1;
		}

		else 
		{
			printf("Out of RAM! Cannot change name for Client %i\n",c->c);

			re = write(c->s, "0M Cannot process request.",26);
			if (re == -1 && errno == EPIPE) 
			{
				printf("Client %i disconnected abruptly.\n",c->c);
				kill_client(c);
			}

			return -1;
		}
	}

	else 
	{
		strncpy(tmp, c->b, NAME_LEN-1);		
	}

	cm = malloc(l+256);
	memset(cm,0,l+256);

	if (c->cliname == NULL)
	{
		snprintf(cm,l+256,"Client %i changed name to: %s\n",c->c,tmp);
		c->cliname = malloc(NAME_LEN);
		memset(c->cliname,0,NAME_LEN);
		strncpy(c->cliname,tmp, l-2);
	}

	else
	{
		snprintf(cm,l+256,"Client %s changed name to: %s\n",c->cliname,tmp);
		memset(c->cliname,0,strlen(c->cliname));
		strncpy(c->cliname,tmp,l-2);		
	}

	l = strlen(cm);
	
	for (i = 0; i < MAX_CLIS; i++)
	{
		if (rc[i].s > 0)
		{
			re = write(rc[i].s,cm,l);
			if (re == -1 && errno == EPIPE)
			{
				printf("Client %i disconnected abruptly.\n",c->c);
				kill_client(c);
			}
		}  	
	}

	free(cm);
	free(tmp);

	return 0;	
}

int parse_client_message(RothagaClient *rc, RothagaClient *c)
{
	int re = 0;
	int i = 0;
	int l = 0;
	char *cm = NULL;

	if(c->cliname == NULL)
	{
		re = write(c->s,"NNYou must assign a name with the SN command!",45);
		if (re == -1 && errno == EPIPE)
		{
			printf("Client %i disconnected abruptly.\n",c->c);
			kill_client(c);
		}

		return -1;
	}

	l = strlen(c->b);
	l -= 2;
	c->b += 2;

	cm = malloc(l+256);

	memset(cm,0,l+256);

	snprintf(cm,l+256,"<%s> %s\n",c->cliname,c->b); 

	l = strlen(cm);
	
	for (i = 0; i < MAX_CLIS; i++)
	{
		if (rc[i].s > 0)
		{
			re = write(rc[i].s,cm,l);
			if (re == -1 && errno == EPIPE)
			{
				printf("Client %i disconnected abruptly.\n",c->c);
				kill_client(c);
			}
		}  	
	}

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

	c->s = -2;  			/* make sure we set it as free */

	return 0;
}

void kill_server(int sig)
{
	printf("\n\nCaught Ctrl-C (%i), shutting down!\n\n",sig); 
	exit(sig);
}
