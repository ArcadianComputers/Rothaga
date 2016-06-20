/*
 * netio.h
 *
 *  Created on: Apr 16, 2016
 *      Author: Jonathan Pollock <jon@arcadiancomputers.com>
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
#include <sys/times.h>	 /* times() */
#include <netinet/in.h> /* htons() */
#include <arpa/inet.h>  /* inet_addr() */
#include <netdb.h>      /* gethostbyaddr() */
#include <fcntl.h>	/* fcntl() */
#include <inttypes.h>
#include <math.h>

#define ALLOWED "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890" 	/*Allowed characters for names*/

#define NAME_LEN 16				/* max length of a clients name */
#define MIN_NAME_LEN 2				/* min length of a clients name */
#define SRV_PORT 7117				/* TCP port we listen on */
#define DEF_SERV "96.91.245.193"		/* starting server */
#define CLI_BUFR 1024				/* size of client buffer */
#define MAX_CLIS 256				/* maximum number of clients */
#define MAX_SRVS 1				/* maximum number of servers */
#define KARMA_BASE 25				/* starting level for karma */
#define KARMA_LOSS_NAME 10			/* value deducted from karma for name changes */
#define KARMA_LOSS_FLOOD 2			/* penalty for flooding the chat */
#define KARMA_LOSS_REPORT 5			/* penalty for being reported */
#define KARMA_LOSS_VECTOR 5			/* exponential rate that karma loss increases*/

typedef void (*ftc)(void *, char *);		/* void function pointer */ /* Jon hates this one */

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
	struct timespec ping;					/* Outgoing ping */
	struct timespec pong;					/* Incoming pong */
	struct timespec fstmes;					/* Timestamp the last message sent */
	struct timespec sndmes;					/* Timestamp the current message */
	int tr;							/* number of times reported */
	int karma;						/* karma (duh) */
	int kv;							/* karma loss vector */
	int sm;							/* is the mac address set? */
	unsigned char mac_address[6];				/* MAC address of this client */

} RothagaClient;

typedef struct
{
        int sp;                                 		/* server port */
        struct sockaddr_in sin;                 		/* socket structure */

} RothagaServer;

RothagaClient *find_free_client(RothagaClient *);		/* find and return an open client slot */
int set_client_name(RothagaClient *,RothagaClient *);		/* parse SN command to set the client's name */
int parse_client_message(RothagaClient *,RothagaClient *);	/* parse SM command to send a global message */
int parse_client_command(RothagaClient *,RothagaClient *);	/* parse a command from a client */
int kill_client(RothagaClient *,RothagaClient *,char *);	/* close a client connection and free it's slot */
int write_client(RothagaClient *,RothagaClient *,char *);	/* write data to a client */
int parse_console_command(RothagaClient *);			/* parse a command from the console */
int parse_server_message(RothagaClient *);			/* parse a message from the server */
int set_name(RothagaClient *,char *);				/* set our name on the network */
int report_user(RothagaClient *,char *);			/* allows you to report a user */
int write_to_server(RothagaClient *,char *);			/* write a command out to the server */
int send_message(RothagaClient *,char *);			/* send a global message */
void sig_pipe_reset(int);					/* reset a client slot that was determined to be dead on write */
int ping_server(RothagaClient *,char *);			/* find the ping between your computer and the server, or another client*/
int send_pong(RothagaClient *,RothagaClient *);			/* Sends ping back */
int send_report(RothagaClient *,RothagaClient *);		/* sends name of reported client to every user on the server */
int new_report(RothagaClient *,char *);				/* confirm the client wants to report a user */
int confirm_report(RothagaClient *,RothagaClient *);		/* confirms the reporting with all users */
/* int karma_check(RothagaClient *); */				/* Checks karma levels for each user */
RothagaClient *lookup_client_by_name(RothagaClient *rc,char *);	/* find a client structure by name */
int confirmation_of_report(RothagaClient *,char *);		/* send a yes answer in response to a report request */
int cisin(char);						/* check for allowed characters */
void char_cleaner(char *str);					/* get rid of control characters */
int karma_gift(RothagaClient *, RothagaClient *);		/* the gift of karma! */
int send_karma(RothagaClient *, char *);			/* client side karma! */
int net_connect(RothagaClient *, RothagaServer *);		/* connect to server */
int send_mac(RothagaClient *);					/* send client MAC to server */
int set_mac(RothagaClient *, RothagaClient *);			/* set client MAC on server */
<<<<<<< Updated upstream
int rndname(RothagaClient *);					/* create a random name if one if not entered */
=======
int check_mac(RothagaClient *, RothagaClient *);
>>>>>>> Stashed changes

#endif /* NETIO_H_ */
