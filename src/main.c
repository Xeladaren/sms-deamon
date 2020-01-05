#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
//#include <termios.h>
#include <asm/termios.h>


int main(int argc, char const *argv[])
{
	
	int ttyS0 = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_SYNC) ;
	struct termios options;

	if (ttyS0 > 0){

		ssize_t size = 0 ;
		
		printf("ttyS0 open on %d\n", ttyS0);

		/* raw mode, like Version 7 terminal driver */
		cfmakeraw(&options);
		options.c_cflag |= (CLOCAL | CREAD);

		/* set the options */
		tcsetattr(ttyS0, TCSANOW, &options);

		char writeMSG[10] = "ATS1?\r\n" ;

		size = write(ttyS0, writeMSG, 10) ;

		printf("%d byte writed : %s\n", size, writeMSG);

		char readMSG[100] ;
		memset(readMSG, 0, 100) ;

		size = read(ttyS0, readMSG, 100) ;

		printf("%d byte read : %s\n", size, readMSG);
		

		close(ttyS0) ;
	}
	else{
		printf("ERROR : open /dev/ttyS0 %d\n", ttyS0);
	}

	return 0;
}