#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <mariadb/mysql.h>

#include "at.h"

void stop(int signal) {

	printf("stop sms-deamon (%d)\n", signal);
	stopAT();

	exit(0);

}

void newSMS(SMS * sms){

	printSMS(sms) ;

	int ret = mkdir("sms", 0755) ;

	if (ret == 0 || errno == EEXIST) {

		char fileName[100] ;
		sprintf(fileName, "sms/%ld_%s.txt", sms->date, sms->sender) ;

		FILE * file = fopen(fileName, "w") ;

		fprintf(file, "%s\n", sms->PDU);
		fprintf(file, "%ld\n", sms->date);
		fprintf(file, "%s\n", sms->sender);
		fprintf(file, "%s\n", sms->msg);

		fclose(file) ;

	}

	MYSQL * mysql = NULL ;

	mysql = mysql_init(mysql) ;

	//mysql = mysql_real_connect(mysql, )

	mysql_close(mysql);

	freeSMS(sms);
	sms = NULL ;

}

int main(int argc, char const *argv[]) {

	signal(SIGINT, stop);
	signal(SIGKILL, stop);

	if (initAT("/dev/ttyS0", 1000) == 0) {

		if (setPinAT("0000", 10000) == SIN_READY) {

			printf("init ok !!\n");

			waitCallReady(10000);

			waitSMSReady(10000);

			setSMSConfig();

			setNewSMSFunction(&newSMS);

			loadSMSList();

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
