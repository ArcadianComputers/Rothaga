#include "rohasha.h"

int main(int argc, char **argv)
{
	gcry_init();									/* init the gcrypt library -Jon */

	printf("%s\n",rohasha(argv[1]));						/* hash the 1st input from the command line -Jon */

	free(phash);									/* free the global hash buffer after we use it -Jon */

	return 0;									/* exit without error -Jon */
}
