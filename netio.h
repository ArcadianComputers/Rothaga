/*
 * netio.h
 *
 *  Created on: Apr 16, 2016
 *      Author: Admin
 */

#ifndef NETIO_H_
#define NETIO_H_

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>   /* socket(), bind() */
#include <sys/socket.h>  /* listen(), accept() */
#include <netinet/in.h> /* htons() */
#include <arpa/inet.h>  /* inet_addr() */
#include <netdb.h>      /* gethostbyaddr() */
#include <fcntl.h>	/* fcntl() */

#define NAME_LEN 256	/* max length of a clients name */

typedef struct
{
	int c;							/* client number */
        int s;                                  		/* communication socket */
        char *b;                                		/* communication buffer */
        int nc;                                 		/* index position in com buffer */
	char *cliname;						/* Temporary name for Client */

} RothagaClient;

typedef struct
{
        int sp;                                 		/* server port */
        struct sockaddr_in sin;                 		/* socket structure */

} RothagaServer;

RothagaClient *find_free_client(RothagaClient *);		/* find and return an open client slot */
int set_client_name(RothagaClient *, RothagaClient *);		/* parse SN command to set the client's name */
int parse_client_message(RothagaClient *, RothagaClient *);	/* parse SM command to send a global message */
int parse_client_command(RothagaClient *, RothagaClient *);	/* parse a command from a client */
int kill_client(RothagaClient *);				/* close a client connection and free it's slot */
int write_client(RothagaClient *, char *);			/* write data to a client */
void sig_pipe_reset(int);					/* reset a client slot that was determined to be dead on write */

#endif /* NETIO_H_ */
