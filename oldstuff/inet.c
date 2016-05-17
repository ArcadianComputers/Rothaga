/* internet related functions */

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

#include <fcntl.h> /* fcntl() */

#include "config.h"
#include "struct.h"
#include "protos.h"
#include "typedefs.h" 

int init_ports(Servs *servptr)
{
	int i=0; 		/* counter */
	int re=0;		/* return holder */
	struct sockaddr_in sin; /* socket structure */

	memset(&sin,0,sizeof(sin)); /* clear it out */

	for(i = 0; i < servptr->pnum; i++) /* loop through our ports */
	{
		servptr->p_arr[i] = socket(AF_INET,SOCK_STREAM,0);

		if (servptr->p_arr[i] == -1)
		{
			perror("socket()");
			exit(-1);
		}

		fcntl(servptr->p_arr[i],F_SETFL,O_NONBLOCK); /* non-block i/o */

		sin.sin_family = AF_INET; /* address family */
		sin.sin_port = htons(servptr->ports[i]); /* tcp port */
		sin.sin_addr.s_addr = INADDR_ANY; /* listen on all interfaces */

		re = bind(servptr->p_arr[i],
			(struct sockaddr *) &sin,sizeof(sin));
		
		if (re == -1)
		{
			perror("bind()");
			printf("%i is in use\n",servptr->ports[i]);
			exit(-1);
		}

		re = listen(servptr->p_arr[i],5); /* hard-wired back log */

		if (re == -1)
		{
			perror("listen()");
			exit(-1);
		}

	} /* for loop */

	/* and now the routing port */

	servptr->rsd = socket(AF_INET,SOCK_STREAM,0);

	if (servptr->rsd == -1)
	{
		perror("socket()");
		exit(-1);
	}

	fcntl(servptr->rsd,F_SETFL,O_NONBLOCK); /* non-block i/o */

	sin.sin_family = AF_INET; /* address family */
	sin.sin_port = htons(servptr->route_port); /* tcp port */
	sin.sin_addr.s_addr = INADDR_ANY; /* listen on all interfaces */

	re = bind(servptr->rsd,(struct sockaddr *) &sin,sizeof(sin));
		
	if (re == -1)
	{
		perror("bind()");
		printf("%i is in use\n",servptr->route_port);
		exit(-1);
	}

	re = listen(servptr->rsd,5); /* hard-wired back log */

	if (re == -1)
	{
		perror("listen()");
		exit(-1);
	}

	/* and finally the speed test port */

	servptr->ssd = socket(AF_INET,SOCK_STREAM,0);

	if (servptr->ssd == -1)
	{
		perror("socket()");
		exit(-1);
	}

	fcntl(servptr->ssd,F_SETFL,O_NONBLOCK); /* non-block i/o */

	sin.sin_family = AF_INET; /* address family */
	sin.sin_port = htons(servptr->stp); /* tcp port */
	sin.sin_addr.s_addr = INADDR_ANY; /* listen on all interfaces */

	re = bind(servptr->ssd,(struct sockaddr *) &sin,sizeof(sin));
		
	if (re == -1)
	{
		perror("bind()");
		printf("%i is in use\n",servptr->stp);
		exit(-1);
	}

	re = listen(servptr->ssd,5); /* hard-wired back log */

	if (re == -1)
	{
		perror("listen()");
		exit(-1);
	}

	return 0;
}
 
int get_client_connects(Servs *servptr)
{
	int i=0; /* counter */
	int re=-1; /* return holder */
	int *len=NULL,addr_len=-1;
	Clis *cptr=NULL;
 
	struct sockaddr_in sin;
	struct hostent *hptr=NULL;

	memset(&sin,0,sizeof(sin));

	addr_len = sizeof(sin);
	len = &addr_len;

	for(i = 0; i < servptr->pnum; i++)
	{
		re = find_free_index(servptr,CLI);

		if (re == -1) 
		{
			printf("get_client_connects: client array is full\n");
			return -1;
		}

		servptr->c_arr[re] = accept(servptr->p_arr[i],
			(struct sockaddr *) &sin,len);

		if (servptr->c_arr[re] != -1)
		{
			fcntl(servptr->c_arr[re], F_SETFL, O_NONBLOCK);

			printf("client connect from: %s:%i on %i\n",
                               inet_ntoa((struct in_addr) sin.sin_addr),
		               ntohs(sin.sin_port),
                               servptr->ports[i]);

			/* ok, we need to allocate a structure for
			   this new connection, not exactly sure
			   how this is gonna work, but it'll sure
			   make things easier! -pope1 */
			
			/* ok lets have find_free_client() return an
			   integer which is the index of a free client
			   slot in the clients array */

			cptr = find_free_client();

			if (cptr == NULL) 
			{
				printf("get_client_connects: client array is full\n");
				return -1;
			}

			strcpy(cptr->ip,
			       inet_ntoa((struct in_addr) sin.sin_addr));

			hptr = gethostbyaddr((char *) &sin.sin_addr,4,AF_INET);

			if (hptr == NULL) 
				strcpy(cptr->host,cptr->ip); /*unresolved */
			else 
				strncpy(cptr->host,hptr->h_name,MAX_HOST_LEN);

			strcpy(cptr->nick,"*");

			cptr->serv_id = servptr->server_id;
			cptr->sd = servptr->c_arr[re];
			cptr->lastreply = time(NULL); /* kludge */
			cptr->replied = 1;
		}

	} /* for loop */

	return 0;

} /* get_client_connects() */


int get_server_connects(Servs *servptr)
{
	int re=-1; /* return holder */
        int *len=NULL,addr_len=-1;
	RServs *rptr=NULL;

        struct sockaddr_in sin;

        memset(&sin,0,sizeof(sin));

        addr_len = sizeof(sin);
        len = &addr_len;

	re = find_free_index(servptr,SRV);

        if (re == -1)
	{
		printf("server array is full! ahhh!\n");
		return -1;
	} 

	servptr->s_arr[re] = accept(servptr->rsd,
			     (struct sockaddr *) &sin,
			     len);

	if (servptr->s_arr[re] != -1)
	{
		fcntl(servptr->s_arr[re], F_SETFL, O_NONBLOCK);

		printf("get_server_connects: %s -> %i\n",
                        inet_ntoa((struct in_addr) sin.sin_addr),
                        servptr->route_port);

			rptr = find_free_server();

			if (rptr == NULL)
			{
				printf("get_server_connects: no free server slots\n");
				return -1;
			} 

			strcpy(rptr->ip,
			       inet_ntoa((struct in_addr) sin.sin_addr));

			rptr->sd = servptr->s_arr[re];
	}

	return 0;
}

int read_data(Servs *servptr)
{
	int i=0; /* counter(s) */
	int re=-1; /* return holder */
	Clis *cptr=NULL;
	RServs *rptr=NULL;
	char cmsg[MAX_MSG_LEN + 1];
	char smsg[MAX_SMSG_LEN + 1];

        cptr = &c_list[0];

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		memset(&cmsg,0,sizeof(cmsg));

		if (cptr->sd > 0)
		{
			re = read(cptr->sd,cmsg,MAX_MSG_LEN);
			if (re==-1)
			{
				if (errno==EAGAIN) /* nothing to read */
					goto next;
				else if (errno==EBADF ||
					 errno==ECONNRESET ||
					 errno==EPIPE)
				{
					remove_client(servptr,cptr->sd);
					goto next;
				}
				else {
					perror("read()");
					goto next;
				     }
			}
			else if (re==0)
			{
				remove_client(servptr,cptr->sd);
				goto next;
			}
			strcat(cptr->msg,cmsg);
		}
		next:
		cptr++;
	}

	rptr = &rs_list[0];

	for(i = 0; i < MAX_SERVERS; i++)
	{
		memset(&smsg,0,sizeof(smsg));

		if (rptr->sd > 0)
		{
			if (rptr->wl == 1) goto snext;
			re = read(rptr->sd,smsg,MAX_SMSG_LEN);
			if (re==-1)
			{
				if (errno==EAGAIN) /* nothing to read */
					goto snext;
				else if (errno==EBADF ||
					 errno==ECONNRESET ||
					 errno==EPIPE)
				{
					remove_server(servptr, rptr->sd);
					goto snext;
				}
				else {
					perror("read()");
					goto snext;
				     }
			}
			else if (re==0)
			{
				remove_server(servptr,rptr->sd);
				goto snext;
			}
			strcat(rptr->msg,smsg);
		}
		snext:
		rptr++;
	}

	return 0;

} /* read_data */

int find_free_index(Servs *servptr, int type)
{
	int i=0; /* counter */

	if (type==CLI)
	{
		for(i = 0; i < MAX_CONNECTIONS; i++)
			if (servptr->c_arr[i] == -1) return i;

		printf("connection list is full!\n");
		return -1; /* connection list is full */
	}

	else if (type==SRV)
	{
		for(i = 0; i < MAX_SERVERS; i++)
			if (servptr->s_arr[i] == -1) return i;
	
		printf("find_free_index: server sd array is full\n");
		psn_cleanup(500); /* for now */
		return -1; /* server list is full */
	}

	printf("find_free_index: unknown type -> %i\n",type);

	return -1; /* we better not get here.. */
} /* find_free_index() */

RServs *find_free_server(void)
{
        int i=0; /* our counter */
	RServs *rptr=NULL;

        rptr = &rs_list[0];

        for(i = 0; i < MAX_SERVERS; i++, rptr++) 
		if (strlen(rptr->ip) == 0) return rptr;

	printf("find_free_server: all server connections in use\n");
        return NULL; /* all server connections in use */
}

int remove_client(Servs *servptr, int sd)
{
	int i=0; /* counter */
	int tsd=-1; /* temp sd holder */
	char smsg[MAX_SMSG_LEN + 1];
	Clis *cptr=NULL;
	RServs *rptr=NULL;

	cptr = &c_list[0];

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if (cptr->sd == sd) goto next; 
		cptr++;
	}
	
	printf("remove_client: could not find a user where sd=%i\n",sd);
	return -1; /* we didnt find him, odd */

	next:			
	
	/* crap here about informing opers
	   and channels about a client leaving the
	   network and other stuff we need to do */

	printf("remove_client: user %s (%s@%s) exiting.\n",
		cptr->nick,
		cptr->user,
		cptr->host);

	close(sd);

	for(i = 0; i < MAX_CLIENTS; i++)
	{
			if (servptr->c_arr[i] == sd)
			{
				servptr->c_arr[i] = -1;
				goto finish;
			}
	}

	printf("remove_client: attempt to re-initialize an un-allocated client socket %i\n",sd);

	finish:

	if (cptr->reg == 1) /* uh-oh, registered client, propagate a QUIT */
	{
		memset(&smsg,0,sizeof(smsg));

		snprintf(smsg,MAX_SMSG_LEN,"%c%c%c%c%c\n",
			 servptr->server_id,BCAST_CHAR,CMD_QUIT,
			 cptr->uid[0],cptr->uid[1]);

		rptr = &rs_list[0];

		for(i = 0; i < MAX_SERVERS; i++, rptr++)
        	{
                	if (rptr->reg == 1)
                	{
				smsg[1] = rptr->server_id;
                        	
                        	if (rptr->sd > 0)
                                	tsd = rptr->sd;
                        	else if (rptr->sd == 0)
                                	tsd = find_route(0, rptr->server_id);

                        	serv_msg(tsd, smsg);
                	}
        	} /* main for loop */
	} /* cptr->reg == 1 */

	if (cptr->flags[FLAG_OPER] == 1) servptr->opers--;

	clear_user_struct(cptr);
	servptr->lusers--;
	servptr->users--;

	return 0;
}

int remove_server(Servs *servptr, int sd)
{
	int i=0; /* counter */
	int re=-1; /* return holder */
	RServs *rptr=NULL;

	/* we will never be asked to remove a link that
	   wasnt directly connected to us, other functions
	   will handle that, thank gawd! */

	close(sd);
	
	for(i = 0; i < MAX_SERVERS; i++) if (servptr->s_arr[i] == sd) goto k;

	printf("remove_server: sd=%i, not found in servptr->s_arr[]\n",sd);
	psn_cleanup(501);

	k:
	servptr->s_arr[i] = -1; /* reset! */

	rptr = &rs_list[0];

	for(i = 0; i < MAX_SERVERS; i++, rptr++) if (rptr->sd == sd) goto kk;

	printf("remove_server: sd=%i, not found in rs_list[]\n",sd);
	psn_cleanup(502);

	kk:
	if (rptr->reg == 0)
	{
		/* un-verified server bailed out, lets follow
		   suit */
		clear_serv_struct(sd);
		printf("remove_server: removing (un-reg'd) local link unknown\n");
		return 0;
	}

	re = find_route(sd, rptr->server_id);

	if (re != -1) 
	{
		/* someone else has a link to him! lets convert 
		   him into a remote struct and send
	           out a del_route() */

		rptr->sd = 0;
		memset(rptr->msg,0,(MAX_SQUE_LEN + 1));
		for(i = 0; i < MAX_ROUTES; i++) 
			if (rptr->routes[i] == rptr->server_id)
				rptr->routes[i] = BCAST_CHAR;
		rptr->links--;
		rptr->sd = 0;
		rptr->wl = 0;

		del_route(servptr, rptr->server_id);
		servptr->links--;
		servptr->servers--;

		return 0;
	}

	/* hes off the network, so now we need to see who he had a link
	   to that no one else did, and send out squit messages for them
	   as well */

	if (rptr->server_name[0] == 0)
		printf("remove_server: removing local link unknown\n");
	else
		printf("remove_server: removing local link %s\n",rptr->server_name);

	for(i = 0; i < MAX_ROUTES; i++)
	{
		if (rptr->routes[i] != BCAST_CHAR && 
		    rptr->routes[i] != servptr->server_id)
		{
			re = find_route(sd, rptr->routes[i]);
			if (re == -1) /* no one else knew him */
			{
				bcast_dead_server(servptr, sd, rptr->routes[i]);
				rm_remote_clis(servptr, rptr->routes[i]);
				rm_remote_serv(servptr, rptr->routes[i]);
				servptr->servers--;
			}
		}
	}

	bcast_dead_server(servptr, sd, rptr->server_id);
	rm_remote_clis(servptr, rptr->server_id);
	clear_serv_struct(sd);
	servptr->links--;
	servptr->servers--;

	return 0;
}

RServs *find_local_server(int sd)
{
        int i=0; /* our counter */
	RServs *rptr=NULL;

        rptr = &rs_list[0];

        for(i = 0; i < MAX_SERVERS; i++)
        {
                if (rptr->sd == sd) return rptr;
                rptr++;
        }

	printf("find_local_server: socket descriptor %i not found\n",sd);
        return NULL; /* couldnt find it */
}

RServs *find_remote_server(char id)
{
        int i=0; /* our counter */
        RServs *rptr=NULL;

        rptr = &rs_list[0];

        for(i = 0; i < MAX_SERVERS; i++, rptr++)
		if (rptr->server_id == id) return rptr;

        return NULL; /* couldnt find it */
} /* find_remote_server() */

RServs *find_remote_server2(char *ip, int port)
{
        int i=0; /* our counter */
        RServs *rptr=NULL;

        rptr = &rs_list[0];

        for(i = 0; i < MAX_SERVERS; i++)
        {
                if ((strcmp(rptr->ip,ip) == 0) && rptr->route_port == port)
			return rptr;
                rptr++;
        }

        return NULL; /* couldnt find it */
} /* find_remote_server2() */

int mk_icp_connects(Servs *servptr)
{
	int i=0; /* counter */
	int re=-1,re2=-1; /* return holders */
	Icps *iptr=NULL;	
	RServs *rptr=NULL;

	struct sockaddr_in sin;

	/* ok, heres where we attempt to complete the circuit --
	   we loop till we connect to the *first* one
	   the rest should connect to us on thier own once
	   they see us */

	memset(&sin,0,sizeof(sin));

	sin.sin_family = AF_INET; /* not changing */

	iptr = &i_list[0];

	for(i = 0; i < servptr->icps; i++)
	{
		sin.sin_port = htons(iptr->port);
		sin.sin_addr.s_addr = inet_addr(iptr->ip);

		re = find_free_index(servptr,SRV);

		if (re == -1)
		{
			printf("server array is full! ahhh!\n");
			return -1;
		}

		servptr->s_arr[re] = socket(AF_INET,SOCK_STREAM,0);

		re2 = connect(servptr->s_arr[re],
			     (struct sockaddr *) &sin,
			     sizeof(sin));

		if (re2 == -1)
		{
			/* perror("connect()"); */
			printf("mk_icp_connects: could not connect to %s:%i\n", 
				iptr->host,
				iptr->port);
		}

		else
		{
			/* here we go! --
			   we have a connection, now we need to allocate
			   a local structure for the sucker and dump
			   our structs on it, *after* we register
			   ourselves */

			fcntl(servptr->s_arr[re], F_SETFL, O_NONBLOCK);

                	printf("mk_icp_connects: connected to %s:%i\n",
                        	iptr->host,
                        	iptr->port);

                        rptr = find_free_server();

			if (rptr == NULL)
                        {
                                printf("mk_icp_connects: no free server slots\n");
                                return -1;
                        }

                        strcpy(rptr->ip,iptr->ip);

			strcpy(rptr->server_name,iptr->host);

                        rptr->sd = servptr->s_arr[re];
                        rptr->route_port = iptr->port;
                        rptr->mls = iptr->mls;

			register_serv(servptr,rptr->sd,iptr->pass,iptr->mls,1);

			return 0;
		}
		iptr++;
	}

	return 0;
}
