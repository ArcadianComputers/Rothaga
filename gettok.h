char *gettok(char *, char, int);

char retbuf[4096];

char *gettok(char *str, char c, int occur)
{
	int i;
	char *ptr;
	char tmp[4096];
	char sb[2];

	if (occur <= 0) return NULL;

	memset(&tmp,0,sizeof(tmp));
	strncpy(tmp,str,(sizeof(tmp)-1));

	sb[0] = c;	
	sb[1] = 0;

	ptr = strtok(tmp,sb);

	for(i = 1; i < occur; i++) ptr = strtok(NULL,sb);

	if (ptr == NULL) return NULL;

	memset(&retbuf,0,sizeof(retbuf));
	strncpy(retbuf,ptr,(sizeof(retbuf)-1));

	return &retbuf[0];
} /* gettok() */
