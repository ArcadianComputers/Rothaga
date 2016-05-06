/* Rothaga server core <jon@arcadiancomputers.com> */

#include "netio.h"
#include "fileio.h"
#include "encio.h"

#define SRV_PORT 7117				/* TCP port we listen on */

int main(int argc, char **argv)
{
	int i=0;				/* loop counter */
	int re=0;				/* return holder */
	int ra = 1;				/* reuse address */
	int ic[256];				/* array of incoming connections */
	int nc[256][1];				/* index into cc for current string */
	char cc[256][256];			/* array of incoming strings */
	int icp=0;				/* current fd of next incoming connection */
	int prt=0;				/* port return */
	struct sockaddr_in sin; 		/* socket structure */
	char c[1];
	int rc=0;

	signal(SIGINT, kill_server);		/* stop the server on Ctrl-C */

	memset(&sin,0,sizeof(sin));		/* clear it out */
	memset(&ic,0,sizeof(ic)); 		/* clear it out */

	for (i = 0; i < 256; i++) 		/* clear it out */
	{
		memset(cc[i],0,sizeof(cc));
		memset(nc[i],0,sizeof(nc));
	}

	prt = socket(AF_INET,SOCK_STREAM,0);

	if (prt == -1)
	{
		perror("socket()");
		exit(-1);
	}

	fcntl(prt,F_SETFL,O_NONBLOCK);		/* non-block i/o */

	if (setsockopt(prt,SOL_SOCKET,SO_REUSEADDR,&ra,sizeof(ra)) == -1)
	{
		perror("setsockopt(): ");	/* set the listening port reuseable to prevent TIME_WAIT blocking */
		return -1;
	}

	sin.sin_family = AF_INET;		/* address family */
	sin.sin_port = htons(SRV_PORT); 	/* tcp port to listen on */
	sin.sin_addr.s_addr = INADDR_ANY; 	/* listen on all interfaces */

	re = bind(prt, (struct sockaddr *) &sin,sizeof(sin));
		
	if (re == -1)
	{
		perror("bind()");
		printf("%i is in use\n",SRV_PORT);
		exit(-1);
	}

	re = listen(prt,5);			/* hard-wired back log */

	if (re == -1)
	{
		perror("listen()");
		exit(-1);
	}

	while(1)
	{
		usleep(1);

		ic[icp] = accept(prt,NULL,NULL);

		if (ic[icp] == -1)
		{
			if (errno != EAGAIN)
			{
				perror("accept(): ");
			}

		}

		else if (ic[icp] > 0)
		{
			fcntl(ic[icp],F_SETFL,O_NONBLOCK); /* non-block i/o */
			icp++;
		}

		for (i = 0; i < icp; i++)
		{
			rc = read(ic[i],c,1);
		
			if (rc == 1)
			{
				cc[i][nc[i][]] = c;

				if (c[0] >= 32) printf("Got a byte: \"%s\"\t[%i]\n",c,c[0]);
				else if (c[0] < 32 || c[0] > 126) printf("Got a byte: \"\"\t[%i]\n",c[0]);
			}
		}
	}
}

void kill_server(int sig)
{
	printf("\n\nCaught Ctrl-C (%i), shutting down!\n\n",sig); 
	exit(sig);
}
