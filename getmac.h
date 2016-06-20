#include <sys/ioctl.h>
#include <net/if.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>

#ifdef OSX
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#endif

unsigned char mac_address[6];

unsigned char *getmac(void);

#ifdef OSX
unsigned char *getmac(void)
{
	const char* if_name = "en0";
	struct ifaddrs* iflist;
	struct ifaddrs* cur;
	struct sockaddr_dl* sdl;

	if (getifaddrs(&iflist) == 0) 
	{
        	for (cur = iflist; cur; cur = cur->ifa_next) 
		{
			if ((cur->ifa_addr->sa_family == AF_LINK) &&
			(strcmp(cur->ifa_name, if_name) == 0) &&
			cur->ifa_addr) 
			{
				sdl = (struct sockaddr_dl*)cur->ifa_addr;
                		memcpy(mac_address, LLADDR(sdl), sdl->sdl_alen);
                		break;
            		}
        	}

        	freeifaddrs(iflist);
    	}

    	return &mac_address[0];
}
#endif

#ifndef OSX
unsigned char *getmac(void)
{
	struct ifreq ifr;
	struct ifconf ifc;
	char buf[1024];
	int success = 0;
	struct ifreq* it;

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    
	if (sock == -1)
	{ 
		perror("socket()");
		return NULL;
	};
	
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	
	if (ioctl(sock, SIOCGIFCONF, &ifc) == -1)
	{ 
		perror("iotcl()");
		return NULL; 
	}

	it = ifc.ifc_req;

	const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

    	for (; it != end; ++it)
	{
        	strcpy(ifr.ifr_name, it->ifr_name);

        	if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0)
		{
            		if (! (ifr.ifr_flags & IFF_LOOPBACK))
			{ // don't count loopback
                		if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0)
				{
                    			success = 1;
                    			break;
                		}
            		}
        	}

        	else
		{ 
			perror("iotcl()");
			return NULL;
		}
    	}


	if (success) memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);

	return &mac_address[0];
}

#endif
