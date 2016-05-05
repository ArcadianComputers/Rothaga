/* internet related functions */

#include "netio.h"
#include "fileio.h"
#include "encio.h"

#define SRV_PORT 7117

int main(int argc, char **argv)
{
	int re=0;		/* return holder */
	int prt=0;		/* port return */
	struct sockaddr_in sin; /* socket structure */
	char c[1];
	int rc=0;

	memset(&sin,0,sizeof(sin)); /* clear it out */

	prt = socket(AF_INET,SOCK_STREAM,0);

	if (prt == -1)
	{
		perror("socket()");
		exit(-1);
	}

	fcntl(prt,F_SETFL,O_NONBLOCK); /* non-block i/o */

	sin.sin_family = AF_INET; /* address family */
	sin.sin_port = htons(SRV_PORT); /* tcp port */
	sin.sin_addr.s_addr = INADDR_ANY; /* listen on all interfaces */

	re = bind(prt, (struct sockaddr *) &sin,sizeof(sin));
		
	if (re == -1)
	{
		perror("bind()");
		printf("%i is in use\n",SRV_PORT);
		exit(-1);
	}

	re = listen(prt,5); /* hard-wired back log */

	if (re == -1)
	{
		perror("listen()");
		exit(-1);
	}

	while(1)
	{
		usleep(1);
		rc = read(prt,c,1);
		if (rc == 1)
		{
			printf("Got a byte: %s\n",c);
		}
	}
}
