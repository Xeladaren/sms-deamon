#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <termios.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <wchar.h>

#include "at.h"

#define SUB 0x1a

#define BUFF_SIZE 200
#define STRING_SIZE 300

#define RAW_BUFFER_SIZE 1000

// *** global variable ***

int ttySMS = 0 ;
int threadRun = 0 ; pthread_mutex_t threadRunMutex = PTHREAD_MUTEX_INITIALIZER ;

pthread_t readThread ;
pthread_t newSMSThread ;

int bufferIndex = 0 ;
char * buffer[BUFF_SIZE] ; pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER ;

int rawBufferIndex = 0 ;
char rawBuffer[RAW_BUFFER_SIZE] ; pthread_mutex_t rawBufferMutex = PTHREAD_MUTEX_INITIALIZER ;

int SMSReady = 0 ;
int CallReady = 0 ;

// *** private propotype *** :

void * readThreadFunc(void * param) ;
void * newSMSThreadFunc(void * param) ;
int setPreferredMessageStorage() ;
int setShowTextModeParameters() ;
int setMessageFormat() ;
int readSMSinMemory(int addr) ;
int delSMSinMemory(int addr) ;
time_t parseDate(char * date) ;
int findChemAfterIndex(char * chem, char * chemError, int index, int timeout, char * cmdReturn) ;
int usleep(int usec);

// *** public functions ***

int initAT(char ttyFILE[], int timeout) {

	ttySMS = open(ttyFILE, O_RDWR | O_NOCTTY | O_SYNC ) ;
	struct termios options;

	if (ttySMS > 0) {

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


		while(threadRun == 0 && timeout != 0) {
			timeout-- ;
			usleep(1000) ;
		}
		if (timeout == 0) {
			return -1 ;
		}
		else {

			if(getPinStatusAT() == SIN_READY) {

				SMSReady = 1 ;
				CallReady = 1 ;

			}

			return 0 ;
		}

	}

	else {
		return -1 ;
	}

}

int stopAT() {

	pthread_cancel(readThread) ;
	close(ttySMS) ;

	return 1 ;

}

int isCallReadyAT() {
	return CallReady ;
}

int isSMSReadyAT() {
	return SMSReady ;
}

int waitCallReady(int timeout) {

	while(!isCallReadyAT() && timeout) {
		timeout--;
		usleep(1000) ;
	}

	if (timeout == 0) {
		return -1 ;
	}
	else {
		return 0 ;
	}

}

int waitSMSReady(int timeout) {

	while(!isSMSReadyAT() && timeout) {
		timeout--;
		usleep(1000) ;
	}

	if (timeout == 0) {
		return -1 ;
	}
	else {
		return 0 ;
	}
}

PinStatus getPinStatusAT() {

	int lastBufferIndex = bufferIndex ;

	write(ttySMS, "AT+CPIN?\n\r", 15) ;

	PinStatus returnStat = SIN_ERROR ;

	if(findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) == -1)
		return SIN_ERROR ;

	char cmd[STRING_SIZE] ;

	if(findChemAfterIndex("+CPIN:", NULL, lastBufferIndex, 1000, cmd) == -1)
		return SIN_ERROR ;

	if (strstr(cmd, "READY"))
		returnStat = SIN_READY ;

	else if (strstr(cmd, "SIM PIN"))
		returnStat = SIM_PIN ;

	else if (strstr(cmd, "SIM PIN2"))
		returnStat = SIM_PIN2 ;

	else if (strstr(cmd, "SIM PUK"))
		returnStat = SIM_PUK ;

	else if (strstr(cmd, "SIM PUK2"))
		returnStat = SIM_PUK2 ;

	else
		returnStat = SIM_UNKNOWN ;


	return returnStat ;
}

PhoneStatus getPhoneStatusAT() {
	int lastBufferIndex = bufferIndex ;

	write(ttySMS, "AT+CPAS\n\r", 15) ;

	PhoneStatus returnStat = PHONE_UNKNOWN ;

	if(findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) == -1)
		return PHONE_UNKNOWN ;

	/*

	char cmd[STRING_SIZE] ;

	if(findChemAfterIndex("+CPIN:", NULL, lastBufferIndex, 1000, cmd) == -1)
		return ERROR ;

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

	*/

	return returnStat ;
}

PinStatus setPinAT(char pin[], int timeout) {

	PinStatus returnStat = getPinStatusAT() ;

	if (returnStat == SIM_PIN || returnStat == SIM_PIN2) {

		// make write cmd

		char * writeMSG ;

		int lastBufferIndex = bufferIndex ;

		writeMSG = malloc(11+strlen(pin)) ;

		sprintf(writeMSG, "AT+CPIN=%s\n\r", pin);

		write(ttySMS, writeMSG, 11+strlen(pin)) ;

		free(writeMSG) ;

		// wait return

		if(findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) == -1)
			return SIN_ERROR ;

		char cmd[STRING_SIZE] ;

		if(findChemAfterIndex("+CPIN:", NULL, lastBufferIndex, 1000, cmd) == -1)
			return SIN_ERROR ;

		if (strstr(cmd, "READY"))
			returnStat = SIN_READY ;

		else if (strstr(cmd, "SIM PIN"))
			returnStat = SIM_PIN ;

		else if (strstr(cmd, "SIM PIN2"))
			returnStat = SIM_PIN2 ;

		else if (strstr(cmd, "SIM PUK"))
			returnStat = SIM_PUK ;

		else if (strstr(cmd, "SIM PUK2"))
			returnStat = SIM_PUK2 ;

		else
			returnStat = SIM_UNKNOWN ;

	}


	return returnStat ;

}

int setSMSConfig() {

	if(setPreferredMessageStorage() == -1) {
		return -1 ;
	}

	if(setMessageFormat() == -1) {
		return -1 ;
	}

	if(setShowTextModeParameters() == -1) {
		return -1 ;
	}

	return 0 ;
}

int writeCustomCmd(char cmd[], int size) {

	write(ttySMS, cmd, size) ;

	return 0 ;

}

// *** private function ***

void * readThreadFunc(void * param) {

	char out ;
	char outOld = '\n' ;
	int index2 = 0 ;

	printf("Thread START\n");

	pthread_mutex_lock(&threadRunMutex) ;

	threadRun = 1 ;

	pthread_mutex_unlock(&threadRunMutex) ;

	while(threadRun) {

		if (buffer[bufferIndex] == NULL){

			pthread_mutex_lock(&bufferMutex) ;

			buffer[bufferIndex] = malloc(STRING_SIZE) ;
			memset(buffer[bufferIndex], 0, STRING_SIZE) ;

			pthread_mutex_unlock(&bufferMutex) ;

		}

		read(ttySMS, &out, 1) ;

		pthread_mutex_lock(&rawBufferMutex) ;

		rawBuffer[rawBufferIndex] = out ;
		rawBufferIndex = ( rawBufferIndex + 1 ) % RAW_BUFFER_SIZE ;

		pthread_mutex_unlock(&rawBufferMutex) ;

		if(out == 0)
			continue ;

		if (out == '\n' && outOld != '\n') {

			if(buffer[bufferIndex] != NULL) {

				FILE * logFile = fopen("./log.txt", "a") ;

				if (logFile != NULL) {

					pthread_mutex_lock(&bufferMutex) ;

					fprintf(logFile, "%s\n", buffer[bufferIndex]) ;

					printf("%d> %s\n", bufferIndex, buffer[bufferIndex]);

					pthread_mutex_unlock(&bufferMutex) ;

					fclose(logFile) ;
				}

				pthread_mutex_lock(&bufferMutex) ;

				if (strstr(buffer[bufferIndex], "Call Ready")) {

					printf("Call Ready !!\n");
					CallReady = 1 ;

				}

				if (strstr(buffer[bufferIndex], "SMS Ready")) {

					printf("SMS Ready !!\n");
					SMSReady = 1 ;

				}


				if (strstr(buffer[bufferIndex], "+CMTI:")) {

					printf("New SMS !!\n");

					char * cmd = malloc(STRING_SIZE) ;
					strncpy(cmd, buffer[bufferIndex], STRING_SIZE) ;

					pthread_create(&newSMSThread, NULL, &newSMSThreadFunc, (void *) cmd);

				}

				pthread_mutex_unlock(&bufferMutex) ;

			}

			pthread_mutex_lock(&bufferMutex) ;

			bufferIndex = ( bufferIndex + 1 ) % BUFF_SIZE ;

			free(buffer[bufferIndex]) ;
			buffer[bufferIndex] = NULL ;

			pthread_mutex_unlock(&bufferMutex) ;

			index2 = 0 ;


		}
		else if(out != '\n') {

			pthread_mutex_lock(&bufferMutex) ;

			buffer[bufferIndex][index2] = out ;

			pthread_mutex_unlock(&bufferMutex) ;

			index2 = ( index2 + 1 ) % (STRING_SIZE - 1) ;

		}

		outOld = out ;

	}

	return NULL ;

}

void * newSMSThreadFunc(void * param) {

	char * cmd = (char *) param ;

	int memAddr = 0 ;

	if(strstr(cmd, "+CMTI: \"SM\",")) {
		sscanf(cmd, "+CMTI: \"SM\",%d", &memAddr) ;

		sleep(1);

		readSMSinMemory(memAddr) ;

		delSMSinMemory(memAddr) ;

	}

	free(cmd) ;

	return NULL ;
}

int setPreferredMessageStorage() {

	int lastBufferIndex = bufferIndex ;

	write(ttySMS, "AT+CPMS=\"SM\",\"SM\",\"SM\"\n\r", 25) ;

	if(findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) == -1)
		return -1 ;

	return 0 ;

}

int setShowTextModeParameters() {

	printf("set Show Text Mode Parameters !!\n");

	int lastBufferIndex = bufferIndex ;

	write(ttySMS, "AT+CSDH=1\n\r", 12) ;

	if(findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) == -1)
		return -1 ;

	return 0 ;

}

int setMessageFormat() {

	printf("set Message Format !!\n");

	int lastBufferIndex = bufferIndex ;

	write(ttySMS, "AT+CMGF=1\n\r", 12) ;

	if(findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) == -1)
		return -1 ;

	return 0 ;

}

int readSMSinMemory(int addr) {

	if(addr > 0 && addr <= 40){

		char writeMSG[13] ;
		memset(writeMSG, 0, 13) ;

		sprintf(writeMSG, "AT+CMGR=%d\n\r", addr) ;
		printf("del msg in %d\n", addr);

		pthread_mutex_lock(&bufferMutex);
		int lastBufferIndex = bufferIndex ;
		pthread_mutex_unlock(&bufferMutex);

		pthread_mutex_lock(&rawBufferMutex) ;
		int lastRawBufferIndex = rawBufferIndex ;
		pthread_mutex_unlock(&rawBufferMutex) ;

		write(ttySMS, writeMSG, 13) ;

		int indexOK = findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) ;

		if(indexOK != -1) {

			char cmdReturn[STRING_SIZE] ;
			int indexCMGR = findChemAfterIndex("+CMGR:", NULL, lastBufferIndex, 1000, cmdReturn) ;

			if (indexCMGR != -1) {

				char * cmdCursor = cmdReturn ;

				cmdCursor = index(cmdCursor, ',') + 1 ;

				char num[20] ;

				if (cmdCursor != NULL) {
					strncpy(num, cmdCursor + 1, index(cmdCursor, ',') - cmdCursor - 2) ;
				}

				cmdCursor = index(cmdCursor, ',') + 1 ;
				cmdCursor = index(cmdCursor, ',') + 1 ;

				char date[25] ;
				memset(date, 0, 25);

				if (cmdCursor != NULL) {
					strncpy(date, cmdCursor + 1, 20) ;
				}

				time_t timeEpoch = parseDate(date) ;

				cmdCursor = index(cmdCursor, ',') + 1 ;
				cmdCursor = index(cmdCursor, ',') + 1 ;
				cmdCursor = index(cmdCursor, ',') + 1 ;
				cmdCursor = index(cmdCursor, ',') + 1 ;
				cmdCursor = index(cmdCursor, ',') + 1 ;

				printf("cmdCursor : %s\n", cmdCursor);

				int protocol = -1 ;

				// Portocol List :
				// 0 = SMS - Simple Text
				// 4 = MMS
				// 8 = SMS - UNICODE

				if (cmdCursor[0] == '0') {
					protocol = 0 ;
				}
				else if (cmdCursor[0] == '8') {
					protocol = 8 ;
				}

				cmdCursor = index(cmdCursor, ',') + 1 ;
				cmdCursor = index(cmdCursor, ',') + 1 ;
				cmdCursor = index(cmdCursor, ',') + 1 ;

				char charDataSize[5];
				memset(charDataSize, 0, 5);

				if (cmdCursor != NULL) {
					strcpy(charDataSize, cmdCursor) ;
				}

				int dataSize = 0 ;

				sscanf(charDataSize, "%d", &dataSize);

				char data[dataSize+1] ;
				memset(data, 0, dataSize+1) ;

				pthread_mutex_lock(&rawBufferMutex) ;

				lastRawBufferIndex += strlen(cmdReturn) + 16 ;

				for (size_t i = 0; i < dataSize; i++) {
					int pos = ( lastRawBufferIndex + i ) % RAW_BUFFER_SIZE ;
					data[i] = rawBuffer[pos];
				}

				pthread_mutex_unlock(&rawBufferMutex) ;

				printf("RAW DATA : %s\n", data);

				if(protocol == 0){

					// TODO : use string data

					

				}
				else if (protocol == 4) {
					// TODO : MMS
				}
				else if (protocol == 8) {

					size_t uniSMSlen = (dataSize/4)+1 ;
					wchar_t unicodeSMS[uniSMSlen];
					memset(unicodeSMS, 0, uniSMSlen * sizeof(wchar_t));

					for (size_t i = 0; i < dataSize; i += 4) {

						char uniChar[5];
						memset(uniChar, 0, 5);
						memcpy(uniChar, &data[i], 4);

						sscanf(uniChar, "%X", &unicodeSMS[i/4]) ;

					}

					// TODO : use unicode data.

				}

				return 0 ;

			}

		}
		else return -1 ;

	}
	else return -1 ;

	return 0 ;

}

int delSMSinMemory(int addr) {

	if(addr > 0 && addr <= 40){

		char writeMSG[13] ;
		memset(writeMSG, 0, 13) ;

		sprintf(writeMSG, "AT+CMGD=%d\n\r", addr) ;
		printf("del msg in %d\n", addr);

		int lastBufferIndex = bufferIndex ;

		write(ttySMS, writeMSG, 13) ;

		if(findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) == -1)
			return -1 ;

	}
	else {
		return -1 ;
	}

	return 0 ;

}

time_t parseDate(char * date) {

	int year, month, day ;
	int hours, minutes, seconds ;
	int timeZone ;
	char timeZoneSign ; // + or -

	sscanf(date, "%02d/%02d/%02d,%02d:%02d:%02d%c%02d",
		&year, &month, &day,
		&hours, &minutes, &seconds,
		&timeZoneSign, &timeZone
	);

	year = year + 2000 ;
	if (timeZoneSign == '-') timeZone = - timeZone ;

	struct tm timeDesc ;

	timeDesc.tm_year = year - 1900 ;
	timeDesc.tm_mon = month ;
	timeDesc.tm_mday = day ;
	timeDesc.tm_hour = hours ;
	timeDesc.tm_min = minutes ;
	timeDesc.tm_sec = seconds ;

	return ( mktime(&timeDesc) - ( 60 * 15 * timeZone ) ) ;

}

int findChemAfterIndex(char * chem, char * chemError, int index, int timeout, char * cmdReturn) {

	while(timeout) {

		if(index != bufferIndex) {

			int searchIndex = bufferIndex - 1 ;

			if(searchIndex < 0) {
				searchIndex = BUFF_SIZE + (searchIndex % BUFF_SIZE) ;
			}

			while(searchIndex != index)
			{

				pthread_mutex_lock(&bufferMutex);

				if(strstr(buffer[searchIndex], chem))
				{
					if(cmdReturn != NULL) {
						strncpy(cmdReturn, buffer[searchIndex], STRING_SIZE) ;
					}

					pthread_mutex_unlock(&bufferMutex) ;

					return searchIndex ;
				}
				else if (chemError != NULL && strstr(buffer[searchIndex], chemError))
				{
					if(cmdReturn != NULL){
						strncpy(cmdReturn, buffer[searchIndex], STRING_SIZE) ;
					}

					pthread_mutex_unlock(&bufferMutex) ;

					return -1 ;
				}

				pthread_mutex_unlock(&bufferMutex) ;

				searchIndex = searchIndex - 1 ;

				if(searchIndex < 0) {
					searchIndex = BUFF_SIZE + (searchIndex % BUFF_SIZE) ;
				}

			}

		}

		usleep(1000) ;
		timeout-- ;
	}

	return -1 ;

}
