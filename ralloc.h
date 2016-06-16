#define CLI_BUFR 1024                           /* size of client buffer */

void *ralloc(int s)
{
	void *ret = NULL;
	
	ret = malloc(s);

        if (ret == NULL)
        {
                perror("malloc(): ");
		return NULL;
        }
	
	memset(ret,0,CLI_BUFR);

	return ret;
}
