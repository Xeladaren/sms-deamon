#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


int main(int argc, char const *argv[])
{
	
	int ttyS0 = open("/dev/ttyS0", O_RDWR) ;

	if (ttyS0 > 0){

		ssize_t size = 0 ;
		
		printf("ttyS0 open on %d\n", ttyS0);

		char writeMSG[10] = "AT\r\n" ;

		size = write(ttyS0, writeMSG, 10) ;

		printf("%d byte writed\n", size);

		char readMSG[100] ;

		size = read(ttyS0, readMSG, 100) ;

		printf("%d byte read : %s\n", size, readMSG);

		close(ttyS0) ;
	}
	else{
		printf("ERROR : open /dev/ttyS0 %d\n", ttyS0);
	}

	return 0;
}