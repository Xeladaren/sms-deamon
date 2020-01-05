#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <math.h>

int main(int argc, char const *argv[])
{
	
	int ttyS0 = open("/dev/ttyS0", O_RDWR) ;

	if (ttyS0 > 0){

		ssize_t size = 0 ;

		double test = pow((double) ttyS0, 2.0) ;
		
		printf("ttyS0 open on %d\n", ttyS0);

		struct termios theTermios;

		memset(&theTermios, 0, sizeof(struct termios));
		cfmakeraw(&theTermios);
		cfsetspeed(&theTermios, 115200);

		theTermios.c_cflag = CREAD | CLOCAL | CS8 ;     // turn on READ
		theTermios.c_cc[VMIN] = 0;
		theTermios.c_cc[VTIME] = 10;     // 1 sec timeout
		ioctl(ttyS0, TIOCSETA, &theTermios);

		char writeMSG[5] = "AT\r\n" ;

		size = write(ttyS0, writeMSG, 5) ;

		printf("%d byte writed\n", size);

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