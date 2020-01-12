#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
//#include <asm/termios.h>

#define SUB 0x1a

int ttyS0 ;
int threadRun = 0 ;

int bufferIndex = 0 ;
char * buffer[30] ;


void * readThreadFunction(void * param){


	char out ; 
	char outOld = '\n' ;
	int index2 = 0 ;

	threadRun = 1 ;

	printf("Thread START\n");

	while(threadRun){

		if (buffer[bufferIndex] == NULL){

			buffer[bufferIndex] = malloc(100) ;
			memset(buffer[bufferIndex], 0, 100) ;

		}

		read(ttyS0, &out, 1) ;

		if (out == '\n' && outOld != '\n'){

			//printf("%s\n", buffer[bufferIndex]);

			bufferIndex = ( bufferIndex + 1 ) % 30 ;
			index2 = 0 ;

			if(buffer[bufferIndex] != NULL) {
				memset(buffer[bufferIndex], 0, 100) ;
			}

		}
		else if(out != '\n'){

			buffer[bufferIndex][index2] = out ;
			index2 = ( index2 + 1 ) % 99 ;

		}

		outOld = out ;
		//printf("%c", out);

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

	if (init()){

		pthread_t readThread ;

		pthread_create(&readThread, NULL, &readThreadFunction, NULL);

		sleep(1) ;

		while(1) {

			char command[100] ;
			char writeMSG[102] ;

			memset(command, 0, 100) ;
			memset(writeMSG, 0, 102) ;

			scanf("%s", command) ;

			if(!strcmp("quit", command)){
				printf("QUIT\n");

				break ;
			}
			else if(!strcmp("pin?", command)){

				write(ttyS0, "AT+CPIN?\n\r", 15) ;

			}
			else if(!strcmp("buffer", command)){

				for (int i = 0; i < 30; ++i) {

					printf("%02d > ", i);

					if (buffer[i] == NULL) {
						printf("NULL\n");
					}
					else {
						printf("%s\n", buffer[i]);
					}
				}

			}
			else if(!strcmp("pin", command)){

				char pin[4] ;

				printf("PIN : ");
				scanf("%s", pin);
				sprintf(writeMSG, "AT+CPIN=%s\n\r", pin);

				write(ttyS0, writeMSG, 102) ;

			}
			else if(!strcmp("custom", command)){

				char custom[100] ;

				printf("CUSTOM COMMEND : ");
				scanf("%s", custom);
				sprintf(writeMSG, "%s\n\r", custom);

				write(ttyS0, writeMSG, 102) ;

			}
			else if(!strcmp("sms", command)){

				/*char num[15] ;
				char msg[12] = "TEST SMS" ;
				msg[8] = 0x1a ;
				msg[9] = '\n' ;
				msg[10] = '\r' ;
				msg[11] = 0x00 ;*/

				//printf("NUMERO : ");
				//scanf("%s", num);
				
				//printf("MESSAGE : ");
				//scanf("%s", msg);

				//write(ttyS0, "AT+CMGF=1\n\r", 15) ;

				//sleep(5) ;

				//sprintf(writeMSG, "AT+CMGS=%s\n\r", num);

				char num[] = "0664491084" ;
				char msg[] = "TEST SMS" ;


				sprintf(writeMSG, "AT+CMGS=\"%s\"\r%s%c\r\n", num, msg, SUB);


				write(ttyS0, writeMSG, 30) ;

				//sleep(5) ;

				//printf("msg");

				//write(ttyS0, msg, 12) ;

			}

			//sprintf(writeMSG, "%s\n\r", command);

			

		}

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