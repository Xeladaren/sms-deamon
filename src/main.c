#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


int main(int argc, char const *argv[])
{
	
	int ttyS0 = open("/dev/ttyS0", O_RDWR) ;

	if (ttyS0 > 0){
		



		close(ttyS0) ;
	}
	else{
		printf("ERROR : open /dev/ttyS0 %d\n", ttyS0);
	}

	printf("Saluton Mondo !\n");

	return 0;
}