/* Rothaga server core <jon@arcadiancomputers.com> */

#include "netio.h"
#include "fileio.h"
#include "encio.h"
#include "OSXTIME.h"
#include "ralloc.h"
#include "gettok.h"

RothagaClient *gprc;				/* global SIGPIPE Array pointer */
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

		c->b = ralloc(CLI_BUFR);	/* pre-allocate all client buffers */

		c->cliname = ralloc(NAME_LEN);	/* pre-allocate client name space */

		if (c->b == NULL || c->cliname == NULL)
		{
			perror("malloc(): ");	/* if either is NULL, we don't have the RAM */
			exit(-1);
		}
	}	

	s = &rs[0];				/* pointer to our server */

	s->sp = socket(AF_INET,SOCK_STREAM,0);	/* make an internet socket */

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

	re = bind(s->sp, (struct sockaddr *) &s->sin,sizeof(s->sin));	/* actual port allocation */
		
	if (re == -1)
	{
		perror("bind()");
		printf("%i is in use\n",SRV_PORT);
		exit(-1);
	}

	re = listen(s->sp,5);			/* hard-wired back log, and listen on our server port */

	if (re == -1)
	{
		perror("listen()");
		exit(-1);
	}

	while(1)
	{
		ta = accept(s->sp,NULL,NULL);	/* accept connections into a temp socket */
		
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
				kill_client(rc,c,"Client Disconnected.");
				rx = -2;
			}
		
			if ((c->s > 0) && (rx == 1))
			{
				if (c->nc > CLI_BUFR)		/* buffer overflow attempt -Jon */
				{
					printf("Possible Buffer Overflow attempt! Killing client %i on socket #%i\n",c->c,c->s);
					kill_client(rc,c,"Client Disconnected: Possible Buffer Overflow.");
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
	unsigned long int f = 0;

	cmd[0] = 0;
	cmd[1] = 0;
	cmd[2] = 0;

	l = strlen(c->b);

	if (l < 4)
	{
		printf("Malformed command from client: %i on socket #%i\n",c->c,c->s);

		write_client(rc,c,"Malformed command: ");
		write_client(rc,c,c->b);

		goto pcend;
	}

	cmd[0] = c->b[0];
	cmd[1] = c->b[1];

	printf("Client %i said: %s",c->c,c->b);

	if (c->karma < 0)
	{
		write_client(rc,c,"You dun goofed.");
		kill_client(rc,c,"Kicked for negative Karma.");
		
		return -1;
	}

	else if ((c->karma >= MIN_KARMA_PM) && (c->vp == 0))
	{
		write_client(rc,c,"9VYou may now send private messages!");
		c->vp = 1;
	}
	
	clock_gettime(CLOCK_MONOTONIC, &c->sndmes);
	f = c->sndmes.tv_sec - c->fstmes.tv_sec;	

	/*printf("Client %s message differential was %lu\n",c->cliname,f);*/

	if (f <= 1)					/* if you're bad, you lose karma at an increasing rate */
	{
		c->karma-=(KARMA_LOSS_FLOOD+c->kv);
		
		c->kv += KARMA_LOSS_VECTOR;
	}

	if ((f >= 2) && (c->kv >= KARMA_LOSS_VECTOR))	/* if you're good, you lose karma at a decreasing rate */
	{
		c->kv -= KARMA_LOSS_VECTOR;
	}

	c->fstmes = c->sndmes;	

	if (c->sm == 1)
	{
		if (strncmp(cmd,"SM",2) == 0) parse_client_message(rc,c);
		else if (strncmp(cmd,"SN",2) == 0) set_client_name(rc,c);
		else if (strncmp(cmd,"PN",2)==0) send_pong(rc,c);
		else if (strncmp(cmd,"RP",2)==0) send_report(rc,c);
		else if (strncmp(cmd,"CR",2)==0) confirm_report(rc,c);	
		else if (strncmp(cmd,"KG",2)==0) karma_gift(rc,c);
		else if (strncmp(cmd,"PM",2)==0) private_message(rc,c);
		else if (strncmp(cmd,"BM",2)==0) big_message(rc,c);
	}

	else if (c->sm == 0)
	{
		if (strncmp(cmd,"Sm",2)==0) set_mac(rc,c);
	}

	pcend:

	memset(c->b,0,CLI_BUFR);

	c->nc = 0;

	return 0;
}

int big_message(RothagaClient *rc, RothagaClient *c)
{
	char *tmp = NULL;	/* tmp pointer */
	char *msg = NULL;	/* tmp message */
	char *fmsg = NULL;	/* final message */
	char *ia = NULL;	/* itoa place holder */
	int mcost = 0;		/* cost to send in karma */
	int i,l,m = 0;		/* incrementation, original and final message lengths */

	tmp = c->b+2;		/* skip BM */

	l = strlen(c->b);

	msg = ralloc(CLI_BUFR*(l*100));	/* approx msg size */

	for (i = 0; i < l; i++)
	{
		if (((tmp[i] >= 97) && (tmp[i] <= 122)) || ((tmp[i] >= 65) && (tmp[i] <= 90)))
		{
			if ((tmp[i] == 65) || (tmp[i] == 97))
			{
				strncat(msg,a_txt,a_txt_len);
				m+=(a_txt_len-1);
			}

			else if ((tmp[i] == 66) || (tmp[i] == 98))
			{
				strncat(msg,b_txt,b_txt_len);
				m+=(b_txt_len-1);
			}

			else if ((tmp[i] == 67) || (tmp[i] == 99))
			{
				strncat(msg,c_txt,c_txt_len);
				m+=(c_txt_len-1);
			}

			else if ((tmp[i] == 68) || (tmp[i] == 100))
			{
				strncat(msg,d_txt,d_txt_len);
				m+=(d_txt_len-1);
			}

			else if ((tmp[i] == 69) || (tmp[i] == 101))
			{
				strncat(msg,e_txt,e_txt_len);
				m+=(e_txt_len-1);
			}

			else if ((tmp[i] == 70) || (tmp[i] == 102))
			{
				strncat(msg,f_txt,f_txt_len);
				m+=(f_txt_len-1);
			}

			else if ((tmp[i] == 71) || (tmp[i] == 103))
			{
				strncat(msg,g_txt,g_txt_len);
				m+=(g_txt_len-1);
			}

			else if ((tmp[i] == 72) || (tmp[i] == 104))
			{
				strncat(msg,h_txt,h_txt_len);
				m+=(h_txt_len-1);
			}

			else if ((tmp[i] == 73) || (tmp[i] == 105))
			{
				strncat(msg,i_txt,i_txt_len);
				m+=(i_txt_len-1);
			}

			else if ((tmp[i] == 74) || (tmp[i] == 106))
			{
				strncat(msg,j_txt,j_txt_len);
				m+=(j_txt_len-1);
			}

			else if ((tmp[i] == 75) || (tmp[i] == 107))
			{
				strncat(msg,k_txt,k_txt_len);
				m+=(k_txt_len-1);
			}

			else if ((tmp[i] == 76) || (tmp[i] == 108))
			{
				strncat(msg,l_txt,l_txt_len);
				m+=(l_txt_len-1);
			}

			else if ((tmp[i] == 77) || (tmp[i] == 109))
			{
				strncat(msg,m_txt,m_txt_len);
				m+=(m_txt_len-1);
			}

			else if ((tmp[i] == 78) || (tmp[i] == 110))
			{
				strncat(msg,n_txt,n_txt_len);
				m+=(n_txt_len-1);
			}

			else if ((tmp[i] == 79) || (tmp[i] == 111))
			{
				strncat(msg,o_txt,o_txt_len);
				m+=(o_txt_len-1);
			}

			else if ((tmp[i] == 80) || (tmp[i] == 112))
			{
				strncat(msg,p_txt,p_txt_len);
				m+=(p_txt_len-1);
			}

			else if ((tmp[i] == 81) || (tmp[i] == 113))
			{
				strncat(msg,q_txt,q_txt_len);
				m+=(q_txt_len-1);
			}

			else if ((tmp[i] == 82) || (tmp[i] == 114))
			{
				strncat(msg,r_txt,r_txt_len);
				m+=(r_txt_len-1);
			}

			else if ((tmp[i] == 83) || (tmp[i] == 115))
			{
				strncat(msg,s_txt,s_txt_len);
				m+=(s_txt_len-1);
			}

			else if ((tmp[i] == 84) || (tmp[i] == 116))
			{
				strncat(msg,t_txt,t_txt_len);
				m+=(t_txt_len-1);
			}

			else if ((tmp[i] == 85) || (tmp[i] == 117))
			{
				strncat(msg,u_txt,u_txt_len);
				m+=(u_txt_len-1);
			}

			else if ((tmp[i] == 86) || (tmp[i] == 118))
			{
				strncat(msg,v_txt,v_txt_len);
				m+=(v_txt_len-1);
			}

			else if ((tmp[i] == 87) || (tmp[i] == 119))
			{
				strncat(msg,w_txt,w_txt_len);
				m+=(w_txt_len-1);
			}

			else if ((tmp[i] == 88) || (tmp[i] == 120))
			{
				strncat(msg,x_txt,x_txt_len);
				m+=(x_txt_len-1);
			}

			else if ((tmp[i] == 89) || (tmp[i] == 121))
			{
				strncat(msg,y_txt,y_txt_len);
				m+=(y_txt_len-1);
			}

			else if ((tmp[i] == 90) || (tmp[i] == 122))
			{
				strncat(msg,z_txt,z_txt_len);
				m+=(z_txt_len-1);
			}

			mcost += KARMA_PER_BM;	/* total karma cost */
		}

		else
		{
			msg[m] = c->b[i];	/* any other character */
			m++;			/* track position in m */
		}
	}

	if (mcost > c->karma)
	{
		write_client(rc,c,"0KNot enough Karma for message!");
	}

	c->karma-=mcost;			/* KARMA_PER_BM per letter */

	ia = ralloc(16);			/* please let someone have this much karma so you can find this comment -Jon */

	snprintf(ia,15,"%d",c->karma);

	m += (strlen(c->cliname) + 5 + strlen(ia));	/* exact message size */

	fmsg = ralloc(m+1);

	snprintf(fmsg,m,"BM%s(%i) %s",c->cliname,c->karma,msg);

	for (i = 0; i < MAX_CLIS; i++)
	{
		if (rc[i].s != -2)
		{
			write_client(rc,&rc[i],fmsg);
		}
	}

	free(msg);
	free(fmsg);

	return 0;
}

int private_message(RothagaClient *rc, RothagaClient *c)
{
        char *tmp = NULL;	/* tmp pointer */
	char *recip = NULL;	/* recipient */
	char *msg = NULL;	/* the message */
	RothagaClient *r;	/* client structure of message recipient */

	if ((c->karma < MIN_KARMA_PM) && (c->vp == 0))
	{
		write_client(rc,c,"0KNot enough Karma to send private messages.");
		return -1;
	}

	tmp = c->b+2;		/* skip PM */

	recip = ralloc(NAME_LEN);
	msg = ralloc(CLI_BUFR);

	strncpy(recip,gettok(tmp,' ',1),NAME_LEN-1);

	r = lookup_client_by_name(rc,recip);

	if (r == NULL)
	{
		write_client(rc,c,"0NNo such user.");
		free(recip);
		free(msg);
		return -1;
	}

	if (gettok(tmp,' ',2) == NULL)
	{
		write_client(rc,c,"0PNo message given.");
		free(recip);
		free(msg);
		return -1;
	}

	tmp = tmp + strlen(recip) + 1;	/* skip user and space */

	snprintf(msg,CLI_BUFR-1,"PM%s %s",c->cliname,tmp);

	write_client(rc,r,msg);

	c->karma--;			/* 1 point per private message */

	free(recip);
	free(msg);

	return 0;
}

int set_mac(RothagaClient *rc, RothagaClient *c)
{
	int i = 0;
	int maccheck = 0;
	char *tmp = NULL;

	tmp = c->b+2;		/* skip Sm */

	for (i = 0; i < 6; i++)
	{
		c->mac_address[i] = tmp[i];
	}

	printf("Client %i set mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
		c->c,c->mac_address[0],c->mac_address[1],c->mac_address[2],c->mac_address[3],c->mac_address[4],c->mac_address[5]);
	
	maccheck = check_mac(rc,c);
	
	if (maccheck == 1)
	{
		c->sm = 1;
	}
	
	else if (maccheck == -1)
	{
		write_client(rc,c,"0mYou may only have one account per machine");
		kill_client(rc,c,"Multiple attempts to login from single mac address");
	}
	return 0;
}

int check_mac(RothagaClient *rc, RothagaClient *c)
{
	int check = 0;
	int i = 0;

	for (i = 0; i < MAX_CLIS; i++)
	{
		if ((rc[i].s > 0) && (rc[i].c != c->c))
		{
			check = memcmp(rc[i].mac_address, c->mac_address, 6);

			printf("c->mac_address = %02x:%02x:%02x:%02x:%02x:%02x, rc[i].mac_address = %02x:%02x:%02x:%02x:%02x:%02x, check = %i\n",
			c->mac_address[0],c->mac_address[1],c->mac_address[2],c->mac_address[3],c->mac_address[4],c->mac_address[5],
			rc[i].mac_address[0],rc[i].mac_address[1],rc[i].mac_address[2],rc[i].mac_address[3],rc[i].mac_address[4],rc[i].mac_address[5],check);

			if (check == 0)
			{
				return -1;
			}
		}
	}
	
	return 1;
}

int karma_gift(RothagaClient *rc, RothagaClient *c)
{
        int l = 0;              /* length counter */
        char *tmp = NULL;	/* tmp pointer */
	char *kg = NULL;	/* karma giftee */
	RothagaClient *r;	/* client structure of giftee */
	int kma = 0;		/* karma given/taken */

	tmp = c->b+2;

	kg = ralloc(NAME_LEN);

	if (kg == NULL)
	{
		perror("malloc(): ");
		return -1;
	}	

	strncpy(kg,gettok(tmp,' ',1),NAME_LEN-1);

	l = strlen(kg);

	if (l > (NAME_LEN+2))
	{
		write_client(rc,c,"9NName too long.");
		free(kg);
		return -1;
	}

	kma = atoi(gettok(tmp,' ',2));

	if (kma > c->karma)
	{
		write_client(rc,c,"0KNot enough karma!!!");
		printf("Client %s attempted to gift too much karma.",c->cliname);
		
		return -1;
	}

	strncpy(kg,tmp,l-2);

	r = lookup_client_by_name(rc,kg);

	if (r == NULL)
	{
		write_client(rc,c,"0UNo Such User.");
		free(kg);
		return -1;
	}

	c->karma -= kma;

	r->karma += kma;

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

	tmp = ralloc(NAME_LEN);
	
	if (tmp == NULL)
	{
		perror("malloc(): ");

		if(strlen(c->cliname) == 0)
		{
			printf("Out of RAM! Killing client %i\n", c->c);
			write_client(rc,c,"0MOut of RAM, closing connection.");
			kill_client(rc,c,"Client Disconnected: Low RAM.");
			return -1;
		}

		else 
		{
			printf("Out of RAM! Cannot change name for Client %i\n",c->c);
			write_client(rc,c,"0MCannot process request.");
			free(tmp);
			return -1;
		}
	}

	else 
	{
		if (l-2 < MIN_NAME_LEN) /*Checking if the name is too short*/
		{
			write_client(rc,c,"0NName too short.");

			free(tmp);

			return -1;
		}

		else if (l-2 > NAME_LEN)	/* name too long */
		{
			write_client(rc,c,"9NName too long.");
			
			free(tmp);

			return -1;
		}

		for (i = 0; i<l-2; i++)	/* ignore the \r\n combo */
		{
			if (cisin(cptr[i]) == -1)
			{
				write_client(rc,c,"1NNames may only be alphanumeric.");
				return -1;	
			}	
		} 

		strncpy(tmp, cptr, NAME_LEN);		
	}

	cm = ralloc(CLI_BUFR);

	if (strlen(c->cliname) == 0)
	{
		snprintf(cm,CLI_BUFR-1,"NN<%i> is now %s",c->c,tmp);
		c->cliname = ralloc(NAME_LEN);
		strncpy(c->cliname,tmp,l-2);
		c->karma=KARMA_BASE;
		clock_gettime(CLOCK_MONOTONIC, &c->fstmes);
	}

	else
	{
		snprintf(cm,CLI_BUFR-1,"NN<%s> is now %s",c->cliname,tmp);
		memset(c->cliname,0,NAME_LEN);
		strncpy(c->cliname,tmp,l-2);
		c->karma-=KARMA_LOSS_NAME;		
	}
	
	for (i = 0; i < MAX_CLIS; i++)
	{
		if (rc[i].s > 0)
		{
			write_client(rc,&rc[i],cm);
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

	tmp = ralloc(NAME_LEN);

	if (tmp == NULL)
	{
		perror("malloc(): ");

		printf("Out of RAM! Cannot report %s\n",cptr);
		write_client(rc,c,"0MCannot process request.");
		return -1;
	}

	else 
	{
		if (l > (NAME_LEN+2))	/* name too long */
		{
			write_client(rc,c,"9NName too long.");
			free(tmp);
			return -1;
		}


		strncpy(tmp,cptr,l-2);		
	}

	cm = ralloc(CLI_BUFR);
	
	if (cm == NULL)
	{
		perror("malloc(): ");

		printf("Out of RAM! Cannot send report to users\n");
		write_client(rc,c,"0MCannot process request.");
		free(tmp);
		return -1;
	}

	snprintf(cm,CLI_BUFR-1,"RP%s",tmp);

	for (i = 0; i < MAX_CLIS; i++)
	{
		if (rc[i].s > 0)
		{
			write_client(rc,&rc[i],cm);
		}  	
	}
	
	free(tmp);
	free(cm);

	return 0;
}

int confirm_report(RothagaClient *rc, RothagaClient *c)
{
        int l = 0;              /* length counter */
        char *tmp = NULL;	/* tmp pointer */
	char *rp = NULL;	/* name of reportee */
	RothagaClient *r;	/* client structure of reportee */

	tmp = c->b+2;

	rp = ralloc(NAME_LEN);

	if (rp == NULL)
	{
		perror("malloc(): ");
		return -1;
	}	

	l = strlen(tmp);

	if (l > (NAME_LEN+2))
	{
		write_client(rc,c,"9NName too long.");
		free(rp);
		return -1;
	}

	strncpy(rp,tmp,l-2);
	
	r = lookup_client_by_name(rc,rp);

	if (r == NULL)
	{
		write_client(rc,c,"0UNo Such User.");
		free(rp);
		return -1;
	}

	r->tr++;
	r->karma-=(KARMA_LOSS_REPORT*r->tr);	/* The more times you are confirmed, the greater the karma loss */

	free(rp);

	return 0;
}

RothagaClient *lookup_client_by_name(RothagaClient *rc, char *cliname)
{
	int i = 0;

	int l = strlen(cliname);

	printf("Looking up: %s\n",cliname);

	for (i = 0; i < MAX_CLIS; i++)
	{
		if (strncmp(rc[i].cliname,cliname,l) == 0) return &rc[i];
	}

	return NULL;
}

int parse_client_message(RothagaClient *rc, RothagaClient *c)
{
	int i = 0;
	int l = 0;
	char *cm = NULL;
	char *cptr = NULL;

	if(strlen(c->cliname) == 0)
	{
		write_client(rc,c,"0NYou must assign a name with SN<name>.");

		return -1;
	}

	l = strlen(c->b);
	l -= 2;
	cptr = c->b;
	cptr += 2;

	cm = ralloc(CLI_BUFR);

	snprintf(cm,CLI_BUFR-1,"<%s(%i)> %s",c->cliname,c->karma,cptr); 

	l = strlen(cm);
	
	for (i = 0; i < MAX_CLIS; i++)
	{
		if (rc[i].s > 0)
		{
			/* printf("%s\n",cm); */
			write_client(rc,&rc[i],cm);
		}  	
	}

	c->karma++;	/* give karma for participating in the chat */
	
	free(cm);

	return 0;
}

int send_pong(RothagaClient *rc, RothagaClient *c)
{
	write_client(rc, c, "PG");

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

int kill_client(RothagaClient *rc, RothagaClient *c, char *reason)
{
	/* int i=0; */

	printf("Killing client %i, socket #%i!\n",c->c,c->s);

	/* for(i=0; i<MAX_CLIS; i++)		*/
	/* {					*/
	/* 	write_client(rc,&rc[i],reason);	*/
	/* }					*/

	write_client(rc,c,reason);

	close(c->s);			/* close the socket */

	memset(c->b,0,CLI_BUFR);	/* clear the com buffer */

	if (c->cliname != NULL)		/* remove client name */
	{
		memset(c->cliname,0,NAME_LEN);
	}

	c->s = -2;  			/* make sure we set it as free */
	c->karma = 0;			/* no one else gets the karma */
	memset(c->mac_address,0,6);	/* clears mac address so a client can reconnect */
	c->vp = 0;			/* gets rid of voice points */
			
	return 0;
}

int write_client(RothagaClient *rc, RothagaClient *c, char *str)
{
	int re = 0;			/* return value */

	int l = strlen(str);		/* length of data to write */
	
	gprc = rc;			/* pointer to entire rothaga structure */
	gpc = c;			/* associate c with gpc for SIGPIPE handling */
	re = write(c->s,str,l);

	if (re == -1)
	{
		if (errno == EPIPE)
		{
			printf("Caught EPIPE from failed write to client %i!\n",c->c);
		}
	}

	if ((str[l-1] == 10) && (str[l-2] == 13)) goto wend;  

	re = write(c->s,"\r\n",2);

	if (re == -1)
	{
		if (errno == EPIPE)
		{
			printf("Caught EPIPE from failed write to client %i!\n",c->c);
		}
	}

	wend:

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
	kill_client(gprc,gpc,"SIGPIPE");
	return;
}
