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
#include <sys/times.h>	 /*times()*/
#include <netinet/in.h> /* htons() */
#include <arpa/inet.h>  /* inet_addr() */
#include <netdb.h>      /* gethostbyaddr() */
#include <fcntl.h>	/* fcntl() */
#include <inttypes.h>
#include <math.h>


#define NAME_LEN 256				/* max length of a clients name */
#define SRV_PORT 7117				/* TCP port we listen on */
#define CLI_BUFR 1024				/* size of client buffer */
#define MAX_CLIS 256				/* maximum number of clients */
#define MAX_SRVS 1				/* maximum number of servers */

typedef void (*ftc)(void *, char *);			/* void function pointer */ /* Jon hates this one */

typedef struct
{
	int c;							/* client number */
        int s;                                  		/* communication socket */
        char *b;                                		/* communication buffer */
	char *k;						/* keyboard buffer */
        int nc;                                 		/* index position in com buffer */
	int nk;							/* index position in kbd buffer */
	char *cliname;						/* Temporary name for Client */
	char *argyon;						/* Lists the reported user's name */
	ftc f;							/* anon function pointer */
	clock_t ts;						/*Captures timestamp*/
	struct tms *tb;						/*Time holder*/
	long ms;						/*milliseconds*/
	time_t wts;						/*seconds*/
	struct timespec spec;					/*To grab time*/
	int tr;							/* number of times reported */
	int karma;						/* karma (duh) */

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
int parse_console_command(RothagaClient *);			/* parse a command from the console */
int parse_server_message(RothagaClient *);			/* parse a message from the server */
int set_name(RothagaClient *, char *);				/* set our name on the network */
int report_user(RothagaClient *,char *);			/* allows you to report a user */
int write_to_server(RothagaClient *, char *);			/* write a command out to the server */
int send_message(RothagaClient *, char *);			/* send a global message */
void sig_pipe_reset(int);					/* reset a client slot that was determined to be dead on write */
int ping_server(RothagaClient *);				/* find the ping between your computer and the server*/
int send_pong(RothagaClient *);					/* Sends ping back */
int send_report(RothagaClient *, RothagaClient *);		/* sends name of reported client to every user on the server */
int confirm_report(RothagaClient *,char *);			/* confirms the reporting with all users */
/* int karma_check(RothagaClient *); */				/* Checks karma levels for each use */





#endif /* NETIO_H_ */
