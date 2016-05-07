/* Rothaga server core <jon@arcadiancomputers.com> */

#include "netio.h"
#include "fileio.h"
#include "encio.h"

#define SRV_PORT 7117				/* TCP port we listen on */
#define CLI_BUFR 1024				/* size of client buffer */
#define MAX_CLIS 256				/* maximum number of clients */
#define MAX_SRVS 16				/* maximum number of servers */

int main(int argc, char **argv)
{
	int i = 0;				/* loop counter */
	int re = 0;				/* return holder */
	int ra = 1;				/* reuse address */
	int rx = 0;				/* how many characters did we read */
	char x[1];				/* read a character from any socket */

	RothagaServer *s;			/* pointer to current server */
	RothagaClient *c;			/* pointer to current client */
	RothagaServer rs[MAX_SRVS];		/* array of server structures */
	RothagaClient rc[MAX_CLIS];		/* array of client structures */

	signal(SIGINT, kill_server);		/* stop the server on Ctrl-C */

	for (i = 0; i < MAX_CLIS; i++) 		/* clear out client and server arrays */
	{
		memset(rc[i],0,sizeof(rc));
		memset(rs[i],0,sizeof(rs));
	}

	for (i = 0; i < MAX_CLIS; i++) 		/* clear out client and server arrays */
	{
		c = &rc[i];

		c.b = malloc(CLI_BUFR);

		if (c.b == NULL)
		{
			perror("malloc(): ");
			exit(-1);
		}
	}	

	s = &rs[0];				/* pointer to current server */

	s.sp = socket(AF_INET,SOCK_STREAM,0);

	if (s.sp == -1)
	{
		perror("socket()");
		exit(-1);
	}

	fcntl(s.sp,F_SETFL,O_NONBLOCK);		/* non-block i/o */

	if (setsockopt(s.sp,SOL_SOCKET,SO_REUSEADDR,&ra,sizeof(ra)) == -1)
	{
		perror("setsockopt(): ");	/* set the listening port reuseable to prevent TIME_WAIT blocking */
		return -1;
	}

	s.sin.sin_family = AF_INET;		/* address family */
	s.sin.sin_port = htons(SRV_PORT); 	/* tcp port to listen on */
	s.sin.sin_addr.s_addr = INADDR_ANY; 	/* listen on all interfaces */

	re = bind(s.sp, (struct sockaddr *) &s.sin,sizeof(s.sin));
		
	if (re == -1)
	{
		perror("bind()");
		printf("%i is in use\n",SRV_PORT);
		exit(-1);
	}

	re = listen(s.sp,5);			/* hard-wired back log */

	if (re == -1)
	{
		perror("listen()");
		exit(-1);
	}

	while(1)
	{
		usleep(1);			/* f off, I know it's bad */

		c = find_free_client(rc);	/* allocate a free client slot */

		if (c == NULL)
		{
			printf("No free client slots!\n");
		}

		else
		{
			c.s = accept(s.sp,NULL,NULL);
		}

		if (c.s == -1)
		{
			if (errno != EAGAIN)
			{
				perror("accept(): ");
			}
		}

		else if (c.s > 0)
		{
			fcntl(c.s,F_SETFL,O_NONBLOCK); /* non-block i/o */
		}

		for (i = 0; i < MAX_CLIS; i++)
		{
			c = &rc[i];

			if (c.s > 0) rx = read(c.s,x,1);
		
			if (rx == 1)
			{
				if (c.nc > CLI_BUFR)		/* buffer overflow attempt -Jon */
				{
					kill_client(c);
					continue;
				}

				c.b[c.nc] = x;
				c.nc++;

				if (x[0] >= 32) printf("Got a byte: \"%s\"\t[%i]\n",x,x[0]);
				else if (x[0] < 32 || x[0] > 126) printf("Got a byte: \"\"\t[%i]\n",x[0]);

				if ((x == 10) && (c.b[c.nc-1] == 13)) 
				{
					parse_client_command(c);
				}
			}
		}
	}
}

int parse_client_command(RothagaClient *c)
{
	printf("Client %i said: %s\n",c->s,c->b);

	memset(c->b,0,CLI_BUFR);

	c->nc = 0;

	return 0;
}

RothagaClient find_free_client(RothagaClient *rc)
{
	int i = 0;
	RothagaClient *c;

	for(i = 0; i < MAX_CLIS; i++)
	{
		c = rc[i];
		if (c->s > 0) return c;
	}

	return NULL;
}

int kill_client(RothagaClient *c)
{
	printf("\n\nKilling client %i!\n",c->s);

	close(c->s);

	free(c->b);

	memset(c,0,sizeof(RothagaClient));

	return 0;
}

void kill_server(int sig)
{
	printf("\n\nCaught Ctrl-C (%i), shutting down!\n\n",sig); 
	exit(sig);
}
