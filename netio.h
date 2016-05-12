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

#define NAME_LEN 256

typedef struct
{
	int c;					/* client number */
        int s;                                  /* communication socket */
        char *b;                                /* communication buffer */
        int nc;                                 /* index position in com buffer */
	char *cliname;				/* Temporary name for Client */

} RothagaClient;

typedef struct
{
        int sp;                                 /* server port */
        struct sockaddr_in sin;                 /* socket structure */

} RothagaServer;

RothagaClient *find_free_client(RothagaClient *);
int set_client_name(RothagaClient *, RothagaClient *);
int parse_client_message(RothagaClient *, RothagaClient *);
int parse_client_command(RothagaClient *, RothagaClient *);
int kill_client(RothagaClient *);

#endif /* NETIO_H_ */
