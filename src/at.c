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

#include "at.h"
#include "pdu-decode.h"

#define SUB 0x1a

#define BUFF_SIZE 200

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
int catLongSMS(SMS * sms, int longSMSId, int longSMSPos, int longSMSLen) ;
int decodeNewSMSPDU(char PDUstr[]) ;
int readSMSinMemory(int addr) ;
int delSMSinMemory(int addr) ;
int findChemAfterIndex(char * chem, char * chemError, int index, int timeout, char ** cmdReturn) ;

// fix impicite declaration

int usleep(int usec); // obsolete function

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

	pthread_mutex_lock(&bufferMutex) ;
	int lastBufferIndex = bufferIndex ;
	pthread_mutex_unlock(&bufferMutex) ;

	sendCmd("AT+CPIN?\n\r") ;

	PinStatus returnStat = SIN_ERROR ;

	if(findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) == -1){
		return SIN_ERROR ;
	}

	char * cmd = NULL ;

	if(findChemAfterIndex("+CPIN:", NULL, lastBufferIndex, 1000, &cmd) == -1){
		free(cmd) ;
		cmd = NULL ;
		return SIN_ERROR ;
	}

	if (cmd) {

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

		free(cmd) ;
		cmd = NULL ;

	}

	return returnStat ;
}

PinStatus setPinAT(char pin[], int timeout) {

	PinStatus returnStat = getPinStatusAT() ;

	if (returnStat == SIM_PIN || returnStat == SIM_PIN2) {

		// make write cmd

		pthread_mutex_lock(&bufferMutex) ;
		int lastBufferIndex = bufferIndex ;
		pthread_mutex_unlock(&bufferMutex) ;

		char writeMSG[11+strlen(pin)] ;

		sprintf(writeMSG, "AT+CPIN=%s\n\r", pin);

		sendCmd(writeMSG);

		// wait return


		if(findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) == -1){
			return SIN_ERROR ;
		}

		char * cmd = NULL ;

		if(findChemAfterIndex("+CPIN:", NULL, lastBufferIndex, 1000, &cmd) == -1){
			free(cmd);
			cmd = NULL ;
			return SIN_ERROR ;
		}


		if (cmd) {

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

			free(cmd) ;
			cmd = NULL ;

		}

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

	printf("[%s]", sms->sender);

	time_t date = sms->date ;

	struct tm * dateTM = gmtime(&date) ;

	printf("(%04d-%02d-%02d %02d:%02d:%02d UTC) : ",
		dateTM->tm_year + 1900,
		dateTM->tm_mon,
		dateTM->tm_mday,
		dateTM->tm_hour,
		dateTM->tm_min,
		dateTM->tm_sec
	);

	printf("%s\n", sms->msg);


}

void freeSMS(SMS * sms) {
	if (sms) {
		free(sms->sender);
		sms->sender = NULL ;
		free(sms->msg);
		sms->msg = NULL ;
		free(sms->PDU);
		sms->PDU = NULL ;
	}
	free(sms);
	sms = NULL ;
}

int loadSMSList() {

	//printf("Load SMS list\n");

	pthread_mutex_lock(&bufferMutex);
	int lastBufferIndex = bufferIndex ;
	pthread_mutex_unlock(&bufferMutex);

	sendCmd("AT+CMGL=4\n\r") ;

	int indexOK = findChemAfterIndex("OK", "ERROR", lastBufferIndex, 10000, NULL) ;

	//printf("index OK %d %d\n", lastBufferIndex, indexOK);

	if (indexOK != -1) {

		int searchIndex = lastBufferIndex ;
		int nextIsNewPDU = 0 ;

		while (searchIndex != indexOK) {

			char * cmd = NULL ;

			pthread_mutex_lock(&bufferMutex);

			cmd = (char*) malloc(strlen(buffer[searchIndex])+1) ;
			strcpy(cmd, buffer[searchIndex]) ;

			pthread_mutex_unlock(&bufferMutex);

			if (nextIsNewPDU) {

				decodeNewSMSPDU(cmd);

				nextIsNewPDU = 0 ;
			}

			if (strstr(cmd, "+CMGL:")) {

				int memAddr, status ;

				sscanf(cmd, "+CMGL: %d,%d", &memAddr, &status) ;

				if(status == 0 || status == 1) {
					nextIsNewPDU = 1 ;
				}

				delSMSinMemory(memAddr) ;

			}

			free(cmd);
			cmd = NULL ;

		searchIndex = ( searchIndex + 1 ) % BUFF_SIZE ;

		}

	}
	else {
		return -1 ;
	}

	return 0 ;

}

// *** private function ***

int sendCmd(char cmd[]) {

	printf("lock cmd > %s\n", cmd);
	pthread_mutex_lock(&ttyMutex) ;

	write(ttySMS, cmd, strlen(cmd)) ;

	//pthread_mutex_unlock(&ttyMutex) ;

	return 0 ;
}

void * readThreadFunc(void * param) {

	//pthread_mutex_lock(&ttyMutex) ;

	char out ;
	char outOld = '\n' ;
	int index2 = 0 ;

	printf("AT read thread start\n");

	pthread_mutex_lock(&threadRunMutex) ;
	threadRun = 1 ;
	pthread_mutex_unlock(&threadRunMutex) ;

	while(threadRun) {

		pthread_mutex_lock(&bufferMutex) ;

		if (buffer[bufferIndex] == NULL){

			buffer[bufferIndex] = malloc(1) ;
			buffer[0] = 0 ;

		}

		pthread_mutex_unlock(&bufferMutex) ;


		//pthread_mutex_unlock(&ttyMutex) ;

		read(ttySMS, &out, 1) ;

		//pthread_mutex_lock(&ttyMutex) ;

		if(out == 0)
			continue ;

		if (out == '\n' && outOld != '\n') {

			char * cmd = NULL ;

			pthread_mutex_lock(&bufferMutex) ;

			int cmdIndex = bufferIndex ;

			cmd = malloc(strlen(buffer[bufferIndex])+1) ;
			strcpy(cmd, buffer[bufferIndex]) ;

			bufferIndex = ( bufferIndex + 1 ) % BUFF_SIZE ;

			free(buffer[bufferIndex]) ;
			buffer[bufferIndex] = NULL ;


			pthread_mutex_unlock(&bufferMutex) ;


			if(cmd != NULL) {

				FILE * logFile = fopen("./log.txt", "a") ;

				if (logFile) {

					fprintf(logFile, "%s\n", cmd) ;
					printf("%d> %s\n", cmdIndex, cmd);
					fclose(logFile) ;

				}

				if (!strncmp(cmd, "OK", 2) || !strncmp(cmd, "ERROR", 5)) {

					printf("unlock cmd\n");
					pthread_mutex_unlock(&ttyMutex) ;

				}
				else if (!strncmp(cmd, "Call Ready", 10)) {

					//printf("Call Ready !!\n");
					CallReady = 1 ;

				}
				else if (!strncmp(cmd, "SMS Ready", 9)) {

					//printf("SMS Ready !!\n");
					SMSReady = 1 ;

				}
				else if (!strncmp(cmd, "+CMTI: \"SM\"", 11)) {

					printf("New sms received.\n");

					char * cmdSMS = malloc(strlen(cmd)+1) ;
					strcpy(cmdSMS, cmd) ;

					pthread_create(&newSMSThread, NULL, &newSMSThreadFunc, (void *) cmdSMS);

				}
				else if (!strncmp(cmd, "RING", 4)) {

				}

			}

			free(cmd) ;
			cmd = NULL ;

			index2 = 0 ;

		}
		else if(out != '\n') {

			pthread_mutex_lock(&bufferMutex) ;

			buffer[bufferIndex] = realloc(buffer[bufferIndex], index2+2) ;

			buffer[bufferIndex][index2+1] = 0 ;
			buffer[bufferIndex][index2] = out ;

			pthread_mutex_unlock(&bufferMutex) ;

			index2++ ;

		}

		outOld = out ;

	}

	return NULL ;

}

void * newSMSThreadFunc(void * param) {

	char * cmd = (char *) param ;

	int memAddr = 0 ;

	if(!strncmp(cmd, "+CMTI: \"SM\",", 12)) {
		sscanf(cmd, "+CMTI: \"SM\",%d", &memAddr) ;

		//sleep(1);

		if (memAddr) {
			readSMSinMemory(memAddr) ;
			delSMSinMemory(memAddr) ;
		}
	}

	free(cmd) ;
	cmd = NULL ;

	return NULL ;
}

int setPreferredMessageStorage() {

	pthread_mutex_lock(&bufferMutex) ;
	int lastBufferIndex = bufferIndex ;
	pthread_mutex_unlock(&bufferMutex) ;

	sendCmd("AT+CPMS=\"SM\",\"SM\",\"SM\"\n\r") ;


	if(findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) == -1){
		return -1 ;
	}


	return 0 ;

}

int setMessageFormat() {

	//printf("set Message Format !!\n");

	pthread_mutex_lock(&bufferMutex) ;
	int lastBufferIndex = bufferIndex ;
	pthread_mutex_unlock(&bufferMutex) ;

	sendCmd("AT+CMGF=0\n\r") ;

	if(findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) == -1){
		return -1 ;
	}

	return 0 ;

}

int catLongSMS(SMS * sms, int longSMSId, int longSMSPos, int longSMSLen) {

	static LongSMS * * longSMSList = NULL ;
	static int longSMSCount = 0 ;
	int newLongSMSCount = longSMSCount ;

	printf("[long sms %X][%d/%d]", longSMSId, longSMSPos, longSMSLen);
	printSMS(sms) ;

	int isNewLongSMS = 1 ;
	for (size_t i = 0; i < longSMSCount; i++) {

		LongSMS * longSMS = longSMSList[i];

		int isSameID 	= (longSMSId == longSMS->longSMSID) ;
		int isSameLen	= (longSMSLen == longSMS->longSMSLen) ;
		int isSameNum 	= strcmp(sms->sender, longSMS->sender) == 0 ;

		printf("test = %d %d %d\n", isSameID, isSameLen, isSameNum);

		if (isSameID && isSameLen && isSameNum) {
			isNewLongSMS = 0 ;

			printf("is same sms\n");

			longSMS->smsList[longSMSPos-1] = sms ;
			longSMS->SMSCount++ ;
			longSMS->localDate = time(NULL) ;

			if (longSMS->SMSCount == longSMS->longSMSLen) {

				printf("sms is compeled\n");

				SMS * smsFinal = (SMS *) malloc(sizeof(SMS)) ;

				time_t smsFinalTime = sms->date ;

				char * smsFinalSender = (char *) malloc(strlen(longSMS->sender)+1) ;
				strcpy(smsFinalSender, sms->sender) ;

				char * smsFinalMsg = (char *) malloc(1);
				size_t smsFinalMsgSize = 1 ;
				smsFinalMsg[0] = 0 ;

				char * smsFinalPDU = (char *) malloc(1);
				size_t smsFinalPDUSize = 1 ;
				smsFinalPDU[0] = 0 ;

				for (size_t j = 0; j < longSMS->SMSCount; j++) {

					SMS * smsi = longSMS->smsList[j] ;

					smsFinalMsgSize += strlen(smsi->msg) ;
					smsFinalMsg = realloc(smsFinalMsg, smsFinalMsgSize) ;
					strcat(smsFinalMsg, smsi->msg) ;

					smsFinalPDUSize += strlen(smsi->PDU) + 1 ;
					smsFinalPDU = realloc(smsFinalPDU, smsFinalPDUSize) ;
					strcat(smsFinalPDU, smsi->PDU);
					strcat(smsFinalPDU, " ");

				}

				longSMS->localDate = 0 ;

				smsFinal->date 	= smsFinalTime ;
				smsFinal->sender 	= smsFinalSender ;
				smsFinal->msg 		= smsFinalMsg ;
				smsFinal->PDU 		= smsFinalPDU ;

				printf("[final long sms]");
				printSMS(smsFinal);

				if (newSMSFunction) {
					(*newSMSFunction)(smsFinal);
				}
			}
		}

		time_t actualTime = time(NULL);

		// remove long sms if is outdated.
		if ((actualTime - longSMS->localDate) > 60*30) {
			printf("delet old sms\n");

			for (size_t j = 0; j < longSMS->longSMSLen; j++) {
				if (longSMS->smsList[j]) {
					freeSMS(longSMS->smsList[j]) ;
				}
			}

			free(longSMS->smsList);
			longSMS->smsList = NULL ;
			free(longSMS);

			longSMSList[i] = NULL ;

			newLongSMSCount-- ;
		}

	}

	if (isNewLongSMS) {
		newLongSMSCount++ ;
		printf("is new longSMS\n");
	}

	LongSMS * * newLongSMSList = (LongSMS * *) malloc(sizeof(LongSMS *)*newLongSMSCount) ;

	size_t i2 = 0 ;
	for (size_t i = 0; i < longSMSCount; i++) {

		// remove long sms if is outdated.
		if (longSMSList[i] != NULL) {
			newLongSMSList[i2] = longSMSList[i] ;
			i2++ ;
		}

	}
	//longSMSId, int longSMSPos, int longSMSLen
	if (isNewLongSMS) {

		LongSMS * newLongSMS = (LongSMS *) malloc(sizeof(LongSMS));

		newLongSMS->localDate = time(NULL) ;
		newLongSMS->longSMSID = longSMSId ;
		newLongSMS->longSMSLen = longSMSLen ;
		newLongSMS->SMSCount = 1 ;
		newLongSMS->sender = sms->sender ;
		newLongSMS->smsList = (SMS **) malloc(sizeof(SMS *)*longSMSLen) ;

		memset(newLongSMS->smsList, 0, sizeof(SMS *)*longSMSLen) ;

		newLongSMS->smsList[longSMSPos-1] = sms ;

		newLongSMSList[i2] = newLongSMS ;
	}

	free(longSMSList) ;
	longSMSList = newLongSMSList ;
	longSMSCount = newLongSMSCount ;

	return 0 ;

}

int decodeNewSMSPDU(char PDUstr[]) {


	int SMSCenterAddrLen = 0 ; // SMS Center Addrese len.
	int cursor = 0 ;

	sscanf(&PDUstr[cursor], "%02X", &SMSCenterAddrLen) ;

	cursor += 2 + (SMSCenterAddrLen * 2) ;

	int PDUType = 0 ;
	sscanf(&PDUstr[cursor], "%02X", &PDUType) ;

	int MTI  = ( PDUType >> 0 ) & 0x03 ;

	if (MTI == 0x00) {

		//int MMS  = ( PDUType >> 2 ) & 0x01 ;
		//int SRI  = ( PDUType >> 5 ) & 0x01 ;
		int UDHI = ( PDUType >> 6 ) & 0x01 ;
		//int RP   = ( PDUType >> 7 ) & 0x01 ;

		cursor += 2 ;

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

		PDUDecodeNumber(PDUSenderAddr, senderNum, SMSSenderAddrLen) ;

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

		int isLongSMS  = 0 ;
		int headerSize = 0 ;
		int headerType = -1 ;

		int longSMSLen = 0 ; // total SMS count
		int longSMSPos = 0 ; // pos of the actual sms

		int longSMSId  = 0 ;

		if (UDHI) {
			int cursor2 = cursor ;

			sscanf(&PDUstr[cursor2], "%02X", &headerSize) ;
			headerSize++;
			cursor2 += 2 ;

			sscanf(&PDUstr[cursor2], "%02X", &headerType) ;
			cursor2 += 2 + 2 ;

			if (headerType == 0x00 || headerType == 0x08) {
				isLongSMS = 1 ;

				if (headerType == 0x00) {
					sscanf(&PDUstr[cursor2], "%02X", &longSMSId) ;
					cursor2 += 2 ;
				}
				else {
					sscanf(&PDUstr[cursor2], "%04X", &longSMSId) ;
					cursor2 += 4 ;
				}

				sscanf(&PDUstr[cursor2], "%02X", &longSMSLen) ;
				cursor2 += 2 ;

				sscanf(&PDUstr[cursor2], "%02X", &longSMSPos) ;

				//printf("IS LONG SMS (%X) : %d/%d\n", longSMSId, longSMSPos, longSMSLen);

			}

		}

		char * smsText ;

		if (dataCodingID == 0x00) {

			smsText = (char *) malloc(dataLen*2) ; // allocate more for potential special character.
			memset(smsText, 0, dataLen*2);
			PDUDecodeData7b(&PDUstr[cursor], smsText, dataLen, headerSize) ;

		}
		else if (dataCodingID == 0x04) {
			smsText = (char *) malloc(19);
			strcpy(smsText, "MMS unsuported now") ;
		}
		else if (dataCodingID == 0x08) {

			//printf("RAW UNICODE = %s\n", &PDUstr[cursor]);

			smsText = (char *) malloc((dataLen*2)+1) ;
			PDUDecodeDataUnicode(&PDUstr[cursor], smsText, headerSize);

		}
		else {
			smsText = (char *) malloc(20);
			strcpy(smsText, "unsuported msg type") ;
		}

		SMS * sms = (SMS *) malloc(sizeof(SMS)) ;
		char * smsPDU = malloc(strlen(PDUstr)+1);
		strcpy(smsPDU, PDUstr);

		sms->date = date ;
		sms->sender = senderNum ;
		sms->msg = smsText ;
		sms->PDU = smsPDU ;

		//printSMS(sms) ;

		if (isLongSMS) {
			catLongSMS(sms, longSMSId, longSMSPos, longSMSLen) ;
		}
		else {
			if (newSMSFunction) {
				(*newSMSFunction)(sms);
			}
		}

	}
	else {
		return -1;
	}

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

			char * cmdReturn = NULL ;
			int indexCMGR = findChemAfterIndex("+CMGR:", NULL, lastBufferIndex, 1000, &cmdReturn) ;

			if (cmdReturn && indexCMGR != -1) {

				int PDUIndex = ( indexCMGR + 1 ) % BUFF_SIZE ;

				char * PDUstr = NULL ;

				pthread_mutex_lock(&bufferMutex);

				PDUstr = (char*) malloc(strlen(buffer[PDUIndex])+1) ;
				strcpy(PDUstr, buffer[PDUIndex]) ;

				pthread_mutex_unlock(&bufferMutex);

				decodeNewSMSPDU(PDUstr) ;

				free(PDUstr);

				free(cmdReturn) ;
				cmdReturn = NULL ;

				return 0 ;

			}

			free(cmdReturn) ;
			cmdReturn = NULL ;

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
		//printf("delet sms in GSM memory (%d)\n", addr);

		pthread_mutex_lock(&bufferMutex) ;
		int lastBufferIndex = bufferIndex ;
		pthread_mutex_unlock(&bufferMutex) ;

		sendCmd(writeMSG) ;

		if(findChemAfterIndex("OK", "ERROR", lastBufferIndex, 1000, NULL) == -1) {
			return -1 ;
		}

	}
	else {
		return -1 ;
	}

	return 0 ;

}

int findChemAfterIndex(char * chem, char * chemError, int index, int timeout, char ** cmdReturn) {

	while(timeout) {

		pthread_mutex_lock(&bufferMutex);
		int buffIndex = bufferIndex ;
		pthread_mutex_unlock(&bufferMutex) ;

		if(index != buffIndex) {

			int searchIndex = buffIndex - 1 ;

			if(searchIndex < 0) {
				searchIndex = BUFF_SIZE + (searchIndex % BUFF_SIZE) ;
			}

			while(searchIndex != index)
			{

				char * cmd ;

				pthread_mutex_lock(&bufferMutex);

				cmd = malloc(strlen(buffer[searchIndex])+1);
				strcpy(cmd, buffer[searchIndex]) ;

				pthread_mutex_unlock(&bufferMutex) ;



				if(strstr(cmd, chem))
				{

					if (cmdReturn) {
						*cmdReturn = cmd ;
					}
					else {
						free(cmd) ;
						cmd = NULL ;
					}

					return searchIndex ;
				}
				else if (chemError != NULL && strstr(cmd, chemError))
				{

					if (cmdReturn) {
						*cmdReturn = cmd ;
					}
					else {
						free(cmd);
						cmd = NULL ;
					}

					return -1 ;
				}

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
