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
#include "pdu-decode.h"

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

pthread_mutex_t ttyMutex = PTHREAD_MUTEX_INITIALIZER ;

int SMSReady = 0 ;
int CallReady = 0 ;

void (*newSMSFunction)(SMS *) = NULL ;

// *** private propotype *** :

int unicodeToUTF8(unsigned int unicodeChar, char outUTF8[] ) ;
int sendCmd(char cmd[]) ;
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

	sendCmd("AT+CPIN?\n\r") ;

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

	sendCmd("AT+CPAS\n\r") ;

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

		int lastBufferIndex = bufferIndex ;

		char writeMSG[11+strlen(pin)] ;

		sprintf(writeMSG, "AT+CPIN=%s\n\r", pin);

		sendCmd(writeMSG);

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

	return 0 ;
}

int writeCustomCmd(char cmd[], int size) {

	sendCmd(cmd) ;
	return 0 ;

}

int setNewSMSFunction(void (*function)(SMS *)) {
	newSMSFunction = function ;

	if(newSMSFunction)
		return 0 ;
	else
		return -1 ;
}

void printSMS(SMS * sms) {

	printf("NUM : %s\n", sms->sender);

	time_t date = sms->date ;

	struct tm * dateTM = gmtime(&date) ;

	printf("DATE : %02d/%02d/%04d %02d:%02d:%02d UTC\n",
		dateTM->tm_mday,
		dateTM->tm_mon,
		dateTM->tm_year + 1900,
		dateTM->tm_hour,
		dateTM->tm_min,
		dateTM->tm_sec
	);

	printf("MSG : \n%s\n____\n", sms->msg);


}

void freeSMS(SMS * sms) {
	free(sms->sender);
	free(sms->msg);
	free(sms->PDU);
	free(sms);
}

// *** private function ***

int sendCmd(char cmd[]) {

	pthread_mutex_lock(&ttyMutex) ;

	write(ttySMS, cmd, strlen(cmd)) ;

	pthread_mutex_unlock(&ttyMutex) ;

	return 0 ;
}

void * readThreadFunc(void * param) {

	pthread_mutex_lock(&ttyMutex) ;

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

		pthread_mutex_unlock(&ttyMutex) ;

		read(ttySMS, &out, 1) ;

		pthread_mutex_lock(&ttyMutex) ;

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
				else if (strstr(buffer[bufferIndex], "SMS Ready")) {

					printf("SMS Ready !!\n");
					SMSReady = 1 ;

				}
				else if (strstr(buffer[bufferIndex], "+CMTI:")) {

					printf("New SMS !!\n");

					char * cmd = malloc(STRING_SIZE) ;
					strncpy(cmd, buffer[bufferIndex], STRING_SIZE) ;

					pthread_create(&newSMSThread, NULL, &newSMSThreadFunc, (void *) cmd);

				}
				else if (strstr(buffer[bufferIndex], "+CMTI:")) {

					printf("New SMS !!\n");

					char * cmd = malloc(STRING_SIZE) ;
					strncpy(cmd, buffer[bufferIndex], STRING_SIZE) ;

					pthread_create(&newSMSThread, NULL, &newSMSThreadFunc, (void *) cmd);

				}
				else if (strstr(buffer[bufferIndex], "RING")) {

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

	sendCmd("AT+CPMS=\"SM\",\"SM\",\"SM\"\n\r") ;

	if(findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) == -1)
		return -1 ;

	return 0 ;

}

int setMessageFormat() {

	printf("set Message Format !!\n");

	int lastBufferIndex = bufferIndex ;

	sendCmd("AT+CMGF=0\n\r") ;

	if(findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) == -1)
		return -1 ;

	return 0 ;

}

int readSMSinMemory(int addr) {

	if(addr > 0 && addr <= 40){

		char writeMSG[13] ;
		memset(writeMSG, 0, 13) ;

		sprintf(writeMSG, "AT+CMGR=%d\n\r", addr) ;

		pthread_mutex_lock(&bufferMutex);
		int lastBufferIndex = bufferIndex ;
		pthread_mutex_unlock(&bufferMutex);

		sendCmd(writeMSG) ;

		int indexOK = findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) ;

		if(indexOK != -1) {

			char cmdReturn[STRING_SIZE] ;
			int indexCMGR = findChemAfterIndex("+CMGR:", NULL, lastBufferIndex, 1000, cmdReturn) ;

			if (indexCMGR != -1) {

				int PDUIndex = ( indexCMGR + 1 ) % BUFF_SIZE ;

				char * PDUstr = NULL ;

				pthread_mutex_lock(&bufferMutex);

				PDUstr = (char*) malloc(strlen(buffer[PDUIndex])+1) ;
				strcpy(PDUstr, buffer[PDUIndex]) ;

				pthread_mutex_unlock(&bufferMutex);


				int SMSCenterAddrLen = 0 ; // SMS Center Addrese len.
				int cursor = 0 ;

				sscanf(PDUstr, "%02X", &SMSCenterAddrLen) ;

				cursor += 2 + (SMSCenterAddrLen * 2) + 2 ;

				int SMSSenderAddrLen ;

				sscanf(&PDUstr[cursor], "%02X", &SMSSenderAddrLen) ;
				cursor += 2 ;

				int PDUSenderLen = 2+SMSSenderAddrLen ;

				if (PDUSenderLen % 2)
					PDUSenderLen++ ;

				char PDUSenderAddr[PDUSenderLen+1] ;
				PDUSenderAddr[PDUSenderLen] = '\0' ;
				memcpy(PDUSenderAddr, &PDUstr[cursor], PDUSenderLen) ;

				// '+' + num len + '\0' ending
				char * senderNum = (char *) malloc(1+SMSSenderAddrLen+1) ;

				PDUDecodeNumber(PDUSenderAddr, senderNum) ;

				// PDUSenderLen + '\0' ending
				cursor += PDUSenderLen ;

				unsigned char ProtocolID = 0 ;
				sscanf(&PDUstr[cursor], "%02hhX", &ProtocolID) ;
				cursor += 2 ;

				unsigned char dataCodingID = 0 ;
				sscanf(&PDUstr[cursor], "%02hhX", &dataCodingID) ;
				cursor += 2 ;

				char PDUdate[15] ;
				time_t date = 0 ;
				memcpy(PDUdate, &PDUstr[cursor], 14);
				PDUdate[14] = 0 ;
				PDUDecodeTime(PDUdate, &date);
				cursor += 14 ;

				int dataLen = 0 ;
				sscanf(&PDUstr[cursor], "%02X", &dataLen) ;
				cursor += 2 ;

				char * smsText ;

				if (dataCodingID == 0x00) {
					smsText = (char *) malloc(dataLen*2) ; // allocate more for potential special character.
					memset(smsText, 0, dataLen*2);
					PDUDecodeData7b(&PDUstr[cursor], smsText, dataLen) ;
				}
				else if (dataCodingID == 0x04) {
					smsText = (char *) malloc(19);
					strcpy(smsText, "MMS unsuported now") ;
				}
				else if (dataCodingID == 0x08) {
					smsText = (char *) malloc((dataLen*2)+1) ;
					PDUDecodeDataUnicode(&PDUstr[cursor], smsText);
				}
				else {
					smsText = (char *) malloc(20);
					strcpy(smsText, "unsuported msg type") ;
				}


				SMS * sms = (SMS *) malloc(sizeof(SMS)) ;

				sms->date = date ;
				sms->senderSize = 1+SMSSenderAddrLen+1 ;
				sms->msgSize = dataLen+1 ;
				sms->sender = senderNum ;
				sms->msg = smsText ;
				sms->PDU = PDUstr ;

				if (newSMSFunction) {
					(*newSMSFunction)(sms);
				}

				/*

				if(protocol == 0){

					// TODO : use string data

					SMS newSMS ;

					newSMS.date = timeEpoch ;
					strncpy(newSMS.sender, senderNum, 15);
					strncpy(newSMS.msg, data, 160);

					printf("SENDER : %s\n", newSMS.sender);
					printf("TIME : %ld\n", newSMS.date);
					printf("MSG : \n%s\n---\n", newSMS.msg);


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

				*/

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

		sendCmd(writeMSG) ;

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
