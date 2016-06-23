#include <gcrypt.h>
#include "ralloc.h"

void gcry_init(void);									/* initialize the gcrypt() library -Jon */
char *rohasha(char *str);								/* return the sha512 hash of any text -Jon */

char *phash;										/* global friendly hash buffer -Jon */

char *rohasha(char *str)
{
	gcry_md_hd_t sft;								/* our gcrypt handle for hashing -Jon */
	unsigned char *hashed = NULL;							/* the raw bytes of the hash -Jon */
	char tmp[3];									/* tmp buffer -Jon */
	int i,dlen,hlen = 0;								/* increment, length of hash in bytes, length of friendly hash -Jon */
	int l = 0;									/* length of str */
	phash = NULL;									/* always start safe -Jon */

	if (str == NULL)								/* exit if we don't have a string to hash -Jon */
	{
		printf("rohasha(): No input given!");
		return NULL;
	}

	l = strlen(str);								/* take the length of the input str */

	gcry_md_open(&sft, GCRY_MD_SHA512, GCRY_MD_FLAG_SECURE);			/* open a hash handle securely for sha512 -Jon */

	if (gcry_md_is_secure(sft) != 1)						/* make sure we got a secure buffer -Jon */
	{
		printf("Buffer for hash computation was NOT secure! Exiting.\n");
		return NULL;
	}

	if (gcry_md_is_enabled(sft, GCRY_MD_SHA512) != 1)				/* make sure our handle is in fact sha512 related -Jon */
	{
		printf("SHA512 not available for hash computation! Exiting.\n");
		return NULL;
	}

	gcry_md_write(sft, str, l);							/* write input bytes into the hashing algo -Jon */

	gcry_md_final(sft);								/* finalize the hash -Jon */

	dlen = gcry_md_get_algo_dlen(GCRY_MD_SHA512);					/* get the number of bytes we need for sha512 -Jon */
	hlen = dlen*2;									/* friendly version is 2x that + 1 for \0 -Jon */

	hashed = ralloc(dlen);								/* get RAM for raw and friendly hashes -Jon */
	phash = ralloc(hlen+1);

	if (hashed == NULL || phash == NULL)						/* exit if we can't get RAM -Jon */
	{
		perror("malloc(): ");
		return NULL;
	}

	memcpy(hashed,gcry_md_read(sft, GCRY_MD_SHA512),dlen);				/* copy the resulting hash bytes -Jon */

	gcry_md_close(sft);								/* close our hash handle -Jon */

	for(i = 0; i < dlen; i++)							/* make the friendly version -Jon */
	{
		memset(&tmp,0,3);
		snprintf(tmp,2,"%02x",hashed[i]);
		strncat(phash,tmp,2);
	}

	free(hashed);									/* free RAM for this hash -Jon */

	return phash;									/* make sure to free this in the calling function! -Jon */
}

void gcry_init(void)
{
	if (!gcry_check_version (GCRYPT_VERSION))
    	{
      		printf("libgcrypt version mismatch\n");
      		exit(2);
    	}

	/* We don't want to see any warnings, e.g. because we have not yet
	parsed program options which might be used to suppress such
     	warnings. */

	gcry_control (GCRYCTL_SUSPEND_SECMEM_WARN);

	/* ... If required, other initialization goes here.  Note that the
	process might still be running with increased privileges and that
	the secure memory has not been initialized.  */

	/* Allocate a pool of 16k secure memory.  This make the secure memory
	available and also drops privileges where needed.  */
  
	gcry_control (GCRYCTL_INIT_SECMEM, 16384, 0);

	/* It is now okay to let Libgcrypt complain when there was/is
	a problem with the secure memory. */

	gcry_control (GCRYCTL_RESUME_SECMEM_WARN);

	/* ... If required, other initialization goes here.  */

	/* Tell Libgcrypt that initialization has completed. */

	gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);

	return;
}
