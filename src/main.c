#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
//#include <asm/termios.h>


int main(int argc, char const *argv[])
{
	
	int ttyS0 = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_SYNC) ;
	struct termios options;

	if (ttyS0 > 0){

		ssize_t size = 0 ;
		
		printf("ttyS0 open on %d\n", ttyS0);

		fcntl(ttyS0, F_SETFL, 0);

		/* get the current options */
		tcgetattr(ttyS0, &options);

		/* set raw input, 1 character trigger */
		options.c_cflag     |= (CLOCAL | CREAD);
		options.c_lflag     &= ~(ICANON | ECHO | ECHOE | ISIG);
		options.c_oflag     &= ~OPOST;
		options.c_cc[VMIN]  = 1;
		options.c_cc[VTIME] = 0;

		/* set the options */
		tcsetattr(ttyS0, TCSANOW, &options);

		char writeMSG[100] = "AT+CPIN=\"0000\"\r\n" ;

		size = write(ttyS0, writeMSG, 100) ;

		printf("%d byte writed : %s\n", size, writeMSG);

		char readMSG[100] ;

		while(1){
			memset(readMSG, 0, 100) ;

			size = read(ttyS0, readMSG, 100) ;

			printf("%s", readMSG);
		}

		close(ttyS0) ;
	}
	else{
		printf("ERROR : open /dev/ttyS0 %d\n", ttyS0);
	}

	return 0;
}