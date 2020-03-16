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

#include "at.h"

#define SUB 0x1a

#define BUFF_SIZE 30
#define STRING_SIZE 300

// *** global variable ***

int ttySMS = 0 ;
int threadRun = 0 ; pthread_mutex_t threadRunMutex = PTHREAD_MUTEX_INITIALIZER ;

pthread_t readThread ;

int bufferIndex = 0 ;
char * buffer[BUFF_SIZE] ; pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER ;

int SMSReady = 0 ;
int CallReady = 0 ;

// *** private propotype *** : 

int findChemAfterIndex(char * chem, char * chemError, int index, int timeout, char * cmdReturn) ;
int usleep(int usec);

// *** public functions ***


void * readThreadFunc(void * param) 
{

	char out ; 
	char outOld = '\n' ;
	int index2 = 0 ;

	printf("Thread START\n");

	pthread_mutex_lock(&threadRunMutex) ;

	threadRun = 1 ;

	pthread_mutex_unlock(&threadRunMutex) ;

	while(threadRun)
	{

		if (buffer[bufferIndex] == NULL){

			pthread_mutex_lock(&bufferMutex) ;

			buffer[bufferIndex] = malloc(STRING_SIZE) ;
			memset(buffer[bufferIndex], 0, STRING_SIZE) ;

			pthread_mutex_unlock(&bufferMutex) ;

		}

		read(ttySMS, &out, 1) ;

		if(out == 0) 
			continue ;

		if (out == '\n' && outOld != '\n')
		{

			if(buffer[bufferIndex] != NULL) 
			{

				FILE * logFile = fopen("./log.txt", "a") ;

				if (logFile != NULL) {
					
					pthread_mutex_lock(&bufferMutex) ;

					fprintf(logFile, "%s\n", buffer[bufferIndex]) ;

					//printf("> %s\n", buffer[bufferIndex]);

					pthread_mutex_unlock(&bufferMutex) ;

					fclose(logFile) ;
				}

				pthread_mutex_lock(&bufferMutex) ;

				if (strstr(buffer[bufferIndex], "Call Ready"))
				{
					printf("Call Ready !!\n");
					CallReady = 1 ;
				}

				if (strstr(buffer[bufferIndex], "SMS Ready"))
				{
					printf("SMS Ready !!\n");
					SMSReady = 1 ;
				}

				pthread_mutex_unlock(&bufferMutex) ;

			}

			pthread_mutex_lock(&bufferMutex) ;

			bufferIndex = ( bufferIndex + 1 ) % BUFF_SIZE ;

			pthread_mutex_unlock(&bufferMutex) ;

			index2 = 0 ;

			if(buffer[bufferIndex] != NULL) {
				pthread_mutex_lock(&bufferMutex) ;

				memset(buffer[bufferIndex], 0, STRING_SIZE) ;

				pthread_mutex_unlock(&bufferMutex) ;
			}

		}
		else if(out != '\n')
		{

			pthread_mutex_lock(&bufferMutex) ;

			buffer[bufferIndex][index2] = out ;

			pthread_mutex_unlock(&bufferMutex) ;

			index2 = ( index2 + 1 ) % (STRING_SIZE - 1) ;

		}

		outOld = out ;

	}

	return NULL ;

}

int initAT(char ttyFILE[]) 
{

	ttySMS = open(ttyFILE, O_RDWR | O_NOCTTY | O_SYNC ) ;
	struct termios options;

	if (ttySMS > 0)
	{
		
		printf("%s open on %d\n", ttyFILE, ttySMS);

		fcntl(ttySMS, F_SETFL, 0);

		// get the current options 
		tcgetattr(ttySMS, &options);

		// set raw input, 1 character trigger 
		options.c_cflag     |= (CLOCAL | CREAD);
		options.c_lflag     &= ~(ICANON | ECHO | ECHOE | ISIG);
		options.c_oflag     &= ~OPOST;
		options.c_cc[VMIN]  = 1;
		options.c_cc[VTIME] = 0;

		// set the options 
		tcsetattr(ttySMS, TCSANOW, &options);

		pthread_create(&readThread, NULL, &readThreadFunc, NULL);

		while(threadRun == 0)
		{
			usleep(100) ;
		}

		return 1 ;
	}

	else
	{
		return 0 ;
	}

}

int stopAT() 
{

	pthread_cancel(readThread) ;

	close(ttySMS) ;

	return 1 ;

}

int isCallReadyAT()
{
	return CallReady ;
}

int isSMSReadyAT()
{
	return SMSReady ;
}

int waitCallReady(int timeout)
{

	while(!isCallReadyAT() && timeout) // wait Call Ready
	{
		timeout--;
		usleep(1000) ;
	}

	if (timeout == 0)
	{
		return -1 ;
	}
	else
	{
		return 0 ;
	}

}

int waitSMSReady(int timeout)
{
	while(!isSMSReadyAT() && timeout) // wait SMS Ready
	{
		timeout--;
		usleep(1000) ;
	}

	if (timeout == 0)
	{
		return -1 ;
	}
	else
	{
		return 0 ;
	}
}

PinStat pinStatusAT() 
{

	int lastBufferIndex = bufferIndex ;

	write(ttySMS, "AT+CPIN?\n\r", 15) ;

	PinStat returnStat = ERROR ;

	if(findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) == -1)
		return ERROR ;

	char cmd[STRING_SIZE] ;

	if(findChemAfterIndex("+CPIN:", NULL, lastBufferIndex, 1000, cmd) == -1)
		return ERROR ;

	printf("CMD = %s\n", cmd);

	if (strstr(cmd, "READY"))
		returnStat = READY ;

	else if (strstr(cmd, "SIM PIN"))
		returnStat = SIM_PIN ;

	else if (strstr(cmd, "SIM PIN2"))
		returnStat = SIM_PIN2 ;

	else if (strstr(cmd, "SIM PUK"))
		returnStat = SIM_PUK ;

	else if (strstr(cmd, "SIM PUK2"))
		returnStat = SIM_PUK2 ;

	else
		returnStat = UNKNOWN ;
	

	return returnStat ;
}

PinStat setPinAT(char pin[], int timeout)
{	

	PinStat returnStat = pinStatusAT() ;

	if (returnStat == SIM_PIN || returnStat == SIM_PIN2)
	{

		// make write cmd

		char * writeMSG ;

		int lastBufferIndex = bufferIndex ;

		writeMSG = malloc(11+strlen(pin)) ;

		sprintf(writeMSG, "AT+CPIN=%s\n\r", pin);

		write(ttySMS, writeMSG, 11+strlen(pin)) ;

		free(writeMSG) ;

		// wait return

		if(findChemAfterIndex("AT+CPIN=", NULL, lastBufferIndex, 1000, NULL) == -1)
			return ERROR ;

		if(findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) == -1)
			return ERROR ;

		char cmd[STRING_SIZE] ;

		if(findChemAfterIndex("+CPIN:", NULL, lastBufferIndex, 1000, cmd) == -1)
			return ERROR ;

		printf("CMD = %s\n", cmd);

		if (strstr(cmd, "READY"))
			returnStat = READY ;

		else if (strstr(cmd, "SIM PIN"))
			returnStat = SIM_PIN ;

		else if (strstr(cmd, "SIM PIN2"))
			returnStat = SIM_PIN2 ;

		else if (strstr(cmd, "SIM PUK"))
			returnStat = SIM_PUK ;

		else if (strstr(cmd, "SIM PUK2"))
			returnStat = SIM_PUK2 ;

		else
			returnStat = UNKNOWN ;

	}

	
	return returnStat ;

}

// *** private function ***

int findChemAfterIndex(char * chem, char * chemError, int index, int timeout, char * cmdReturn)
{

	while(timeout)
	{	

		if(index != bufferIndex)
		{

			int searchIndex = ( bufferIndex - 1 ) % BUFF_SIZE ;

			while(searchIndex != index)
			{

				pthread_mutex_lock(&bufferMutex);

				if(strstr(buffer[searchIndex], chem))
				{
					if(cmdReturn != NULL)
						strncpy(cmdReturn, buffer[searchIndex], STRING_SIZE) ;

					pthread_mutex_unlock(&bufferMutex) ;

					return 0 ;
				}
				else if (chemError != NULL && strstr(buffer[searchIndex], chemError))
				{
					if(cmdReturn != NULL)
						strncpy(cmdReturn, buffer[searchIndex], STRING_SIZE) ;

					pthread_mutex_unlock(&bufferMutex) ;

					return -1 ;
				}

				pthread_mutex_unlock(&bufferMutex) ;

				searchIndex = ( searchIndex - 1 ) % BUFF_SIZE ;

			}

		}
		
		usleep(1000) ;	
		timeout-- ;
	}

	return -1 ;

}
