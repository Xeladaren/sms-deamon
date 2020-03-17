#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "at.h"

void stop(int signal) {

	printf("stop sms-deamon (%d)\n", signal);
	stopAT();

	exit(0);

}

int main(int argc, char const *argv[]) {

	signal(SIGINT, stop);
	signal(SIGKILL, stop);

	if (initAT("/dev/ttyS0", 1000) == 0) {

		if (setPinAT("0000", 10000) == SIN_READY) {

			printf("init ok !!\n");

			waitCallReady(1000);

			waitSMSReady(1000);

			setSMSConfig();

			printf("Start main loop !!\n");
			while (1) {

				char command[100] ;
				char writeMSG[103] ;
				memset(writeMSG, 0, 103) ;


				scanf("%s", command) ;

				sprintf(writeMSG, "%s\n\r", command);

				writeCustomCmd(writeMSG, strlen(writeMSG));

			}

		}

	}

	stopAT();

	return 0;
}
