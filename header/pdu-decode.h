#ifndef PDU_DECODE
#define PDU_DECODE

int PDUDecodeNumber(char inputData[], char outputData[]) ;

int PDUDecodeTime(char inputData[], time_t * date) ;

int PDUDecodeData7b(char inputData[], char outputData[], size_t outputSize, int headerSize) ;

int PDUDecodeDataUnicode(char inputData[], char outputData[], int headerSize);

#endif
