#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <stdio.h>
#include <pthread.h>
//#include <asm/termios.h>

int ttyS0 ;
int threadRun = 0 ;

void * readThreadFunction(void * param){


	char out ;

	threadRun = 1 ;

	while(threadRun){

		read(ttyS0, &out, 1) ;

		printf("%c", out);

	}

    pthread_exit(NULL) ; // fermeture du thread.

    return NULL ;
}

int init(){

	ttyS0 = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_SYNC ) ;
	struct termios options;

	if (ttyS0 > 0){
		
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

		return 1 ;
	}
	else{
		return 0 ;
	}
}


int main(int argc, char const *argv[])
{


	while(1) {

		char command[100] ;

		printf("> ");
		scanf("%s", command) ;

		if(strcmp("quit", command)){
			printf("QUIT\n");

			break ;
		}


	}

	exit(0) ;

	if (init()){

		pthread_t readThread ;

		pthread_create(&readThread, NULL, &readThreadFunction, NULL);

//		char writeMSG[100] = "AT+CPIN=\"0000\"\r\n" ;
//		char writeMSG[100] = "AT+CPIN?\n\r" ;

//		size = write(ttyS0, writeMSG, 100) ;

		pthread_cancel(readThread) ;

		close(ttyS0) ;
	}
	else{
		printf("ERROR : open /dev/ttyS0 %d\n", ttyS0);
	}

	return 0;
}