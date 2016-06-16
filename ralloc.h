void *ralloc(int);

void *ralloc(int s)
{
	void *ret = NULL;
	
	ret = malloc(s);

        if (ret == NULL)
        {
                perror("malloc(): ");
		return NULL;
        }
	
	memset(ret,0,s);

	return ret;
}
