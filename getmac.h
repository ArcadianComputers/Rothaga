#include <sys/ioctl.h>
#include <net/if.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>

unsigned char mac_address[6];

unsigned char *getmac(void);

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
