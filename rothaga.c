/*
 * rothaga.c
 *
 *  Created on: Apr 16, 2016
 *      Author: Jonathan Pollock <jon@arcadiancomputers.com>
 */

#include "netio.h"
#include "fileio.h"
#include "encio.h"
#include "agathor.h"
#include "OSXTIME.h"

#ifndef RALLOC
#define RALLOC
#include "ralloc.h"
#endif

#include "gettok.h"
#include "getmac.h"
#include "rohasha.h"

#define WIN_CFG "C:\\Users\\Admin\\workspace\\Rothaga\\Rothaga.ini"
#define LNX_CFG "./rothaga.ini"
#define ENT_SRC "/dev/random"

const char *defname = "Rothagan";				/* if the client dosen't pick a name -Jon */

WINDOW *chat;							/* the chat window -Jon */
WINDOW *cmds;							/* the command line -Jon */

int main (int argc, char **argv)
{
	int re = 0;						/* return code -Jon */
	char x[1];						/* temp char holder for network i/o -Jon */
	RothagaClient rc;					/* local client structure -Jon */
	RothagaServer rs;					/* remote server structure -Jon */

	x[0] = 0;						/* clear the net buffer -Jon */
	memset(&rc,0,sizeof(rc));				/* clear out the client structure -Jon */
	memset(&rs,0,sizeof(rs));				/* clear out the server structure -Jon */

        signal(SIGINT, kill_rothaga);				/* stop the server on Ctrl-C -Jon */
        signal(SIGPIPE, SIG_IGN);				/* capture SIGPIPE and IGNORE IT -Jon */

	rc.cliname = ralloc(NAME_LEN);				/* allocate space for client's name -Jon */
	rc.argyon = ralloc(NAME_LEN);				/* allocate space for yes/no reply -Jon */

	rs.sin.sin_family = AF_INET;				/* select ipv4 mode for now -Jon */

	build_ui(&rc);						/* build the user interface -Jon */

	if (argc < 4)
	{
		wprintw(chat,"\nUsage: %s <name> <server> <port>\n",argv[0]);
		wrefresh(chat);

		rndname(&rc);

		rs.sin.sin_addr.s_addr	= inet_addr(DEF_SERV);
		rs.sin.sin_port		= htons(SRV_PORT);
	}

	else
	{
		strncpy(rc.cliname, argv[1], NAME_LEN-1);
		rs.sin.sin_addr.s_addr	= inet_addr(argv[2]);
		rs.sin.sin_port		= htons(atoi(argv[3]));
	}


	/* wprintw(chat,"plaintext = %s, enctext = %s\n",argv[2],encrp(argv[2])); */
		
	load_config(LNX_CFG);			/* parse our config file -Jon */
	net_connect(&rc,&rs);			/* connect to the network -Jon */
	send_mac(&rc);				/* send our mac address FIRST! -Jon */
	set_name(&rc,rc.cliname);		/* set our name as soon as we connect -Jon */

	/* wprintw(chat,"%s\n",agathor); */

	wprintw(chat,"Welcome to Rothaga version 1.0 Alpha\nTo view a list of commands type '/comlist'\n");
	wprintw(chat,"Client chose name: %s\n",rc.cliname);

	wrefresh(chat);				/* refresh the window to show cursor */
	wrefresh(cmds);				/* refresh the window to show cursor */

	while(1)
	{
		re = read(rc.s,x,1);		/* read a byte from the server -Jon */
		
		if (re == -1)
		{
			if (errno == EAGAIN) goto rconsole;
			else if (errno == EPIPE || errno == EBADF || errno == ECONNRESET)
			{
				perror("read(): ");
				rprintw(chat,&rc,"Server closed connection.\n");
				break;
			}
		}

		else
		{
			rc.b[rc.nc] = x[0];
			rc.nc++;

			if ((x[0] == 10) && (rc.b[rc.nc-2] == 13))
			{
				parse_server_message(&rc);
				wrefresh(chat);
			}
			
		}

		rconsole:

		wrefresh(cmds);
		re = wgetch(cmds);

		if (re == ERR)
		{
			if ((errno == EAGAIN) || (errno == 0)) continue;
			else if (errno != EAGAIN)
			{
				perror("wgetch(): \n");
				printf("errno: %i\n",errno);
				return -1;
			}
		}

		else if (re != ERR)
		{
			rc.k[rc.nk] = re;
			rc.nk++;

			if (re == 10)
			{
				parse_console_command(&rc);
				wmove(cmds,1,2);
				wclrtoeol(cmds);
				box(cmds,'|','-');
				wrefresh(cmds);
				rc.nk = 0;
			}

			else if (re == KEY_BACKSPACE || re == 127)
			{
				backspace(&rc);
			}

			else
			{
				draw_ui(&rc);
				wprintw(cmds,"%c",(char)re);
			}
		}
	}

	close(rc.s);
	free(rc.b);
	free(rc.k);

	return 0;
}

void rprintw(WINDOW *w, RothagaClient *c, char *str)
{
	wprintw(w,str);
	wrefresh(w);

	return;
}

void build_ui(RothagaClient *rc)
{
	initscr();						/* NCURSES screen init -Jon */
	noecho();
	cbreak();

	getmaxyx(stdscr,rc->y,rc->x);				/* get screen limits -Jon */

	chat = newwin(rc->y-3,rc->x,0,0);
	cmds = newwin(3,rc->x,rc->y-3,0);

	nodelay(chat, TRUE);					/* please dont block my loop ncurses -Jon */
	nodelay(cmds, TRUE);

	scrollok(chat,TRUE);
    	scrollok(cmds,TRUE);

	keypad(cmds, TRUE);					/* support all input options -Jon */
	idcok(cmds, TRUE);					/* support hardware character delete -Jon */
	idlok(cmds, TRUE);					/* support hardware line delete -Jon */
	idlok(chat, TRUE);

    	box(cmds,'|','-');

	wmove(cmds,1,2);				/* set initial cursor position */
       	wrefresh(cmds);

	wmove(chat,rc->y-4,2);				/* set initial chat position */
	wrefresh(chat);

	return;
}

void draw_ui(RothagaClient *rc)
{
	if (is_term_resized(rc->y, rc->x))
	{
		resizeterm(rc->y, rc->x);
    		box(cmds,'|','-');
		wmove(cmds,1,2);				/* set initial cursor position */
       		wrefresh(cmds);
		wmove(chat,rc->y-4,2);				/* set initial chat position */
		wrefresh(chat);
	}

	return;
}

void backspace(RothagaClient *rc)
{
	int y,x = 0;

	noecho();
	nocbreak();

	getyx(cmds, y, x);			/* get cursor position */

	if (x >= 2 && y && (rc->nk > 1))
	{
		wprintw(chat,"backspace called()\n");
		wrefresh(chat);
		rc->k[rc->nk] = 0; 		/* get rid of last character -Jon */
		rc->nk--;			/* decrement character counter -Jon */
		rc->k[rc->nk] = 0; 		/* get rid of last character -Jon */
		rc->nk--;			/* decrement character counter -Jon */
		wmove(cmds,y,x-1);
		wdelch(cmds);
		box(cmds,'|','-');
		wrefresh(cmds);
	}

	cbreak();

	return;
}

int rndname(RothagaClient *rc)
{
	time_t tloc;

	srand(time(&tloc));	/* pseudorandom time seeding */

	snprintf(rc->cliname,NAME_LEN-1,"%s%i",defname,rand());

	return 0;
}

int send_mac(RothagaClient *rc)
{
	char *tmp = NULL;
	unsigned char *address = NULL;

	tmp = ralloc(CLI_BUFR);
	address = ralloc(7);

	address[0] = rc->mac_address[0];
	address[1] = rc->mac_address[1];
	address[2] = rc->mac_address[2];
	address[3] = rc->mac_address[3];
	address[4] = rc->mac_address[4];
	address[5] = rc->mac_address[5];
	address[6] = 0;

	snprintf(tmp,CLI_BUFR-1,"Sm%s",address);

	write_to_server(rc,tmp);

	free(tmp);

	return 0;
}

int net_connect(RothagaClient *rc, RothagaServer *rs)
{
	int re = 0;

	rc->s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	if (rc->s == -1)
	{
		perror("socket(): ");
		return -1;
	}

	getmac();

	memcpy(&rc->mac_address,&mac_address,6);

	wprintw(chat,"Client MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
	rc->mac_address[0],rc->mac_address[1],rc->mac_address[2],rc->mac_address[3],rc->mac_address[4],rc->mac_address[5]);

	re = connect(rc->s,(struct sockaddr *) &rs->sin,sizeof(rs->sin));

	if (re == -1)
	{
		perror("connect(): ");
		wprintw(chat,"Failed to connect to %s on port %i\n",inet_ntoa(rs->sin.sin_addr),ntohs(rs->sin.sin_port));
		return -1;
	}

	wprintw(chat,"Connected to %s:%i\n",inet_ntoa(rs->sin.sin_addr),ntohs(rs->sin.sin_port));

	rc->b = ralloc(CLI_BUFR);
	rc->k = ralloc(CLI_BUFR);

	re = fcntl(rc->s,F_SETFL,O_NONBLOCK);

	if (re == -1)
	{
		perror("fcntl(): ");
		return -1;
	}

	re = fcntl(1,F_SETFL,O_NONBLOCK);

	if (re == -1)
	{
		perror("fcntl(): ");
		return -1;
	}

	return 0;
}

int parse_console_command(RothagaClient *c)
{
	int l = 0;
	char *tmp = NULL;
	char *tmprp = NULL;

	tmp = ralloc(CLI_BUFR);

	l = strlen(c->k);

	strncpy(tmp,c->k,l-1);				/* skip the trailing new line -Jon */
	
	if (strncmp(tmp,"/quit",5) == 0)
	{
		kill_rothaga(0);
	}

	else if (strncmp(tmp,"/sn",3) == 0)
	{
		set_name(c,tmp+4);
	}

	else if (strncmp(tmp, "/ping",5) == 0)
	{
		ping_server(c,tmp+6);
	}

	else if (strncmp(tmp,"/rp",3) == 0)
	{	
		tmprp = tmp+4;

		wprintw(chat,"If you are sure you want to report <%s>, please type '/yes, if not please /no\n\n",tmprp);

		memset(c->argyon,0,NAME_LEN);
		snprintf(c->argyon,NAME_LEN-1,"%s",tmprp);
		
		c->f = (void *)report_user;
	}
	
	else if (strncmp(tmp,"/pd",3) == 0)
	{
		/* request_image(rc,c,tmp+4); */
	}

	else if (strncmp(tmp,"/comlist",8) == 0)
	{
		rprintw(chat,c,"The following is a list of commands: \n '/quit' will exit rothaga \n '/rp <user>' will send a report on <user> to everyone on the server, which they can confirm or deny \n '/kg <user>' will gift some of your karma to <user> \n '/ping' will ping the server \n '/sn <name>' will allow you to change your username \n '/yes' and '/no' are used for confirmation for various commands \n");
	}

	else if (strncmp(tmp,"/kg",3) == 0)
	{
		send_karma(c,tmp+4);
	}

	else if (strncmp(tmp,"/pm",3) == 0)
	{
		send_private_message(c,tmp+4);
	}

	else if (strncmp(tmp,"/bm",3) == 0)
	{
		send_big_message(c,tmp+4);
	}

	else if (strncmp(tmp,"/yes",4) == 0)
	{
		if (c->f == NULL) goto pcend;
		c->f(c,c->argyon);
	}
	
	else if (strncmp(tmp,"/no",3) == 0)
	{
		if (c->f == NULL) goto pcend;
		
		rprintw(chat,c,"Guess not then...\n\n");

		if (c->argyon != NULL) free(c->argyon);
		c->f = (void *)NULL;	/* don't leave the gun loaded */
	}

	else
	{
		send_message(c,tmp);
	}

	pcend:

	memset(c->k,0,CLI_BUFR);
	c->nk = 0;
	free(tmp);

	return 0;
}

int send_big_message(RothagaClient *c, char *msg)
{
	int l = 0;
	char *tmp = NULL;

	l = (strlen(msg) + 4);

	tmp = ralloc(l);

	snprintf(tmp,l-1,"BM%s",msg);

	write_to_server(c,tmp);

	free(tmp);

	return 0;
}

int image_request(RothagaClient *c, char *request)
{
	char *tmp = NULL;
	
	tmp = ralloc(CLI_BUFR);

	if (request == NULL)
	{
		rprintw(chat,c,"You must enter a valid command\n");

		return -1;
	}

	snprintf(tmp,CLI_BUFR-1,"PD%s",request);

	write_to_server(c,tmp);
	
	return 1;
}

int send_karma(RothagaClient *c, char *details)
{
	char *tmp = NULL;
	char *cliname = NULL;	
	int karma_amount = 0;

	tmp = ralloc(CLI_BUFR);
	cliname = ralloc(NAME_LEN);

	if ((gettok(details,' ',1) == NULL) || (gettok(details,' ',2) == NULL))
	{
		rprintw(chat,c,"To gift karma: /kg <name> <amount> Example: /kg Agathor 100\n");
	
		return -1;
	}

	strncpy(cliname,gettok(details,' ',1),NAME_LEN-1);

	karma_amount = atoi(gettok(details,' ',2));
	
	snprintf(tmp,CLI_BUFR-1,"KG%s %i",cliname,karma_amount);

	write_to_server(c,tmp);

	free(tmp);

	return 0;
}

int ping_server(RothagaClient *c, char *cliname)
{
	char *tmp = NULL;
	clock_gettime(CLOCK_MONOTONIC, &c->ping);

	if(strlen(cliname) == 0)
	{
		wprintw(chat,"Sending ping\n");
		write_to_server(c,"PN");
	}

	else 
	{
		wprintw(chat,"Sending ping to: %s\n", cliname);

		tmp = ralloc(NAME_LEN);

		snprintf(tmp,NAME_LEN-1,"PN%s",cliname);
		write_to_server(c,tmp);	
	}

	return 0;	
}

int print_pong(RothagaClient *c)
{
	clock_gettime(CLOCK_MONOTONIC, &c->pong);

	wprintw(chat,"Pong: %.03f milliseconds\n",((c->pong.tv_nsec - c->ping.tv_nsec) / 1.0e6));

	return 0;
}

int send_private_message(RothagaClient *c, char *details)
{
	char *tmp = NULL;
	char *cliname = NULL;	
	char *message = NULL;

	tmp = ralloc(CLI_BUFR);
	cliname = ralloc(NAME_LEN);
	message = ralloc(CLI_BUFR);
	
	if ((gettok(details,' ',1) == NULL) || (gettok(details,' ',2) == NULL))
	{
		rprintw(chat,c,"To send a private message: /pm <name> <message>\n");
		rprintw(chat,c,"For example: /pm Agathor Hello\n");

		return -1;
	}

	strncpy(cliname,gettok(details,' ',1),NAME_LEN-1);
	
	strncpy(message,details+strlen(gettok(details,' ',1))+1,CLI_BUFR-1);

	snprintf(tmp,CLI_BUFR-1,"PM%s %s",cliname,message);

	write_to_server(c,tmp);

	return 0;
}

int set_name(RothagaClient *c, char *cliname)
{
	char *tmp = NULL;				/* tmp buffer */
	
	tmp = ralloc(CLI_BUFR);				/* allocate and clear it */

	snprintf(tmp,CLI_BUFR-1,"SN%s",cliname);	/* format the command */

	write_to_server(c,tmp);				/* send the command */

	memset(tmp,0,CLI_BUFR);				/* clear tmp buffer for re-use */

	strncpy(tmp,cliname,NAME_LEN-1);		/* save cliname to tmp buffer */

	memset(c->cliname,0,NAME_LEN);			/* clear old cliname */

	strncpy(c->cliname,tmp,NAME_LEN-1);		/* copy saved name from tmp to cliname */

	free(tmp);					/* free tmp */

	return 0;					/* all done */
}

int confirmation_of_report(RothagaClient *c,char *reported)
{
	char *tmp = NULL;

	if (reported == NULL)
	{
		rprintw(chat,c,"confrimation_of_report() called with null 'reported' variable");
		return -1;
	}

	tmp = ralloc(CLI_BUFR);

	snprintf(tmp,CLI_BUFR-1,"CR%s",reported);

	write_to_server(c,tmp);
	
	free(tmp);

	return 0;
}

int report_user(RothagaClient *c, char *argyon)
{
	char *tmp = NULL;

	tmp = ralloc(CLI_BUFR);

	snprintf(tmp,CLI_BUFR-1,"RP%s",argyon);

	write_to_server(c,tmp);

	free(tmp);

	return 0;
}

int send_message(RothagaClient *c, char *msg)
{
	char *tmp = NULL;

	tmp = ralloc(CLI_BUFR);

	snprintf(tmp,CLI_BUFR-1,"SM%s",msg);

	write_to_server(c,tmp);

	free(tmp);

	return 0;
}

int write_to_server(RothagaClient *c, char *cmd)
{
	int re = -1;

	/* wprintw(chat,"SENDING: %s\n",cmd); */

	re = write(c->s,cmd,strlen(cmd));

	if (re == -1)
	{
		perror("write() ");
		return -1;
	}

	re = write(c->s,"\r\n",2);

	if (re == -1)
	{
		perror("write() ");
		return -1;
	}

	return 0;
}

int parse_server_message(RothagaClient *c)
{
	int l = 0;
	char *tmp = NULL;

	tmp = ralloc(CLI_BUFR);

	l = strlen(c->b);

	strncpy(tmp,c->b,l-2);	/* skip the \r\n combo */

	char_cleaner(tmp);

	wprintw(chat,"*** %s\n",tmp);
	/* mvwprintw(chat,c->y-(c->y/3),c->x-(c->x/3),"*** %s (y:%i)(x:%i)\n",tmp,c->y-(c->y/3),c->x-(c->x/3)); */
	wrefresh(chat);

	if (strncmp(tmp,"RP",2) == 0)
	{
		memset(c->argyon,0,NAME_LEN);
		strncpy(c->argyon,tmp+2,strlen(tmp+2));
		new_report(c,c->argyon);
	}

	else if(strncmp(tmp,"PG",2) == 0)
	{
		print_pong(c);
	}

	memset(c->b,0,CLI_BUFR);
	c->nc = 0;

	free(tmp);

	return 0;
}

void char_cleaner(char *str)
{
	int i = 0;
	int l = strlen(str);

	for(i = 0; i < l; i++)
	{
		if ((str[i] <= 31) && (str[i] != 10)) str[i] = 32;	/* no control sequences except new line */
		else if (str[i] >= 127) str[i] = 32;			/* no upper ascii */
	}

	return;
}

int new_report(RothagaClient *c,char *reported)
{
	wprintw(chat,"User %s has been reported, type /yes to agree or /no to disagree.\n",reported);
	c->f = (void *)confirmation_of_report;
	
	return 0;
}

char *encrp(char *plaintext)
{
	int i = 0;
	int fd = 0;
	int len = 0;
	char *encptd = NULL;
	char entropy[256];

	unsigned char c = 0;
		
	if (plaintext == NULL)
	{
		return NULL;
	}
	
	len = strlen(plaintext);

	if (len == 0)
	{
		return NULL;
	}

	encptd = ralloc(len*2);
	
	if (encptd == NULL)
	{
		return NULL;
	}

	memset(entropy,0,256);

	fd = open(ENT_SRC,O_RDONLY);

	if (fd == -1)
	{
		return NULL;
	}

	read(fd,entropy,256);
		
	for (i = 0; i < len; i++)
	{
		c = plaintext[i];
		c += entropy[i % 256];
		encptd[i] = c;
	}

	encptd[i] = 0;

	return encptd;
}


void load_config(char *cfg)
{
	int fd=-1;

	fd = open(cfg,O_RDONLY);

	if (fd == -1)
	{
		wprintw(chat,"Error opening %s!\n",cfg);
		return;
	}

	wprintw(chat,"Opened config file %s!\n",cfg);

	return;
}

void kill_rothaga(int sig)
{
	wprintw(chat,"\n\nAgathor, nooooooooo!\n\n");
        wprintw(chat,"\n\nCaught Ctrl-C (%i), shutting down!\n\n",sig);
	endwin();
        exit(sig);
}
