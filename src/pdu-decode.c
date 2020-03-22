#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "pdu-decode.h"
#include "gsm-char-set.h"

// ** Private Prototype ***

int unicodeToUTF8(unsigned int unicodeChar, char outUTF8[] ) ;

// ** Public functions ***

int PDUDecodeNumber(char inputData[], char outputData[]) {

   if(strncmp(inputData, "91", 2) == 0 ){

      outputData[0] = '+' ;

      for (size_t i = 3; i < strlen(inputData); i+=2) {

         if (inputData[i-1] != 'F')
            outputData[i-1] = inputData[i-1] ;
         else
            outputData[i-1] = 0 ;

         if (inputData[i-2] != 'F')
            outputData[i-2] = inputData[i] ;
         else
            outputData[i-2] = 0 ;
      }

   }
   else if(strncmp(inputData, "A1", 2) == 0) {

      for (size_t i = 3; i < strlen(inputData); i+=2) {

         outputData[i-2] = inputData[i-1] ;
         outputData[i-3] = inputData[i] ;

      }

   }

   return 0 ;

}

int PDUDecodeTime(char inputData[], time_t * date) {

   if (strlen(inputData) == 14) {

      char dateString[strlen(inputData)+1] ;
      strcpy(dateString, inputData) ;

      for (size_t i = 0; i < strlen(dateString); i+=2) {
         char tmp = dateString[i] ;
         dateString[i] = dateString[i+1] ;
         dateString[i+1] = tmp ;
      }

      int year, month, day, hour, minute, second ;
      char timeZone ;

      sscanf(dateString, "%02d%02d%02d%02d%02d%02d%02hhX",
         &year, &month, &day,
         &hour, &minute, &second,
         &timeZone
      );

      year += 2000 ;

      struct tm utc ;

      utc.tm_year = year - 1900 ;
      utc.tm_mon = month ;
      utc.tm_mday = day ;
      utc.tm_hour = hour ;
      utc.tm_min = minute ;
      utc.tm_sec = second ;


      if(date != NULL) {
         *date = timegm(&utc) - ( 60 * 15 * timeZone )  ;
      }

   }
   else {
      return -1 ;
   }

   return 0 ;
}

int PDUDecodeData7b(char inputData[], char outputData[], size_t charCount, int headerSize) {

   size_t dataInSize = strlen(inputData) / 2 ;
   unsigned char dataIN[dataInSize] ;
   memset(dataIN, 0, dataInSize) ;

   int offsetSize = ( headerSize  * 8 ) / 7 ;

   if ( ( headerSize * 8) % 7 ) {
      offsetSize++;
   }

   for (size_t i = 0; i < strlen(inputData); i += 2) {

      char octet[3] ;
      memset(octet, 0, 3);
      memcpy(octet, &inputData[i], 2);

      sscanf(octet, "%hhx", &dataIN[i/2]);

   }

   outputData[0] = 0 ;

   int haveESC = 0 ;
   int n = 1 ;
   for (size_t i = 0; i < charCount; i++) {

      unsigned char valOut = 0 ;
      int x = i % 8 ;

      if (x == 0) {
         n--;
      }

      valOut = dataIN[n] << x ;

      if (n != 0) {
         valOut = valOut | dataIN[n-1] >> (8-x) ;
      }

      valOut = valOut & 0x7f ;

      n++ ;
      if (offsetSize > 0) {
         offsetSize-- ;
      }
      else{
         if (haveESC) {
            strcat(outputData, GSMCharSetBasicExt[valOut]) ;
            haveESC = 0 ;
         }
         else {
            strcat(outputData, GSMCharSetBasic[valOut]) ;
         }

         if (valOut == 0x1B) {
            haveESC = 1 ;
         }
      }

   }

   return 0 ;

}

int PDUDecodeDataUnicode(char inputData[], char outputData[], int headerSize) {

   outputData[0] = 0 ;

   for (size_t i = headerSize*2; i < strlen(inputData); i+=4) {

      unsigned int unicodeChar = 0 ;
      sscanf(&inputData[i], "%04X", &unicodeChar) ;

      char utf8char[5] ;
      unicodeToUTF8(unicodeChar, utf8char);
      strcat(outputData, utf8char);

   }
   return 0 ;
}

// ** Private functions ***

int unicodeToUTF8(unsigned int unicodeChar, char outUTF8[] ) {

	if (unicodeChar < 0x80) {
		outUTF8[0] = 0x00 | ( ( unicodeChar >> 0x00 ) & 0x7F ) ;
		outUTF8[1] = 0x00 ;
		return 0 ;
	}
	else if (unicodeChar < 0x800) {
		outUTF8[0] = 0xC0 | ( ( unicodeChar >> 0x06 ) & 0x1F ) ;
		outUTF8[1] = 0x80 | ( ( unicodeChar >> 0x00 ) & 0x3F ) ;
		outUTF8[2] = 0x00 ;
		return 0 ;
	}
	else if (unicodeChar < 0x10000) {
		outUTF8[0] = 0xE0 | ( ( unicodeChar >> 0x0C ) & 0x0F ) ;
		outUTF8[1] = 0x80 | ( ( unicodeChar >> 0x06 ) & 0x3F ) ;
		outUTF8[2] = 0x80 | ( ( unicodeChar >> 0x00 ) & 0x3F ) ;
		outUTF8[3] = 0x00 ;
		return 0 ;
	}
	else if (unicodeChar < 0x110000) {
		outUTF8[0] = 0xF0 | ( ( unicodeChar >> 0x12 ) & 0x07 ) ;
		outUTF8[1] = 0x80 | ( ( unicodeChar >> 0x0C ) & 0x3F ) ;
		outUTF8[2] = 0x80 | ( ( unicodeChar >> 0x06 ) & 0x3F ) ;
		outUTF8[3] = 0x80 | ( ( unicodeChar >> 0x00 ) & 0x3F ) ;
		outUTF8[4] = 0x00 ;
		return 0 ;
	}

   return -1 ;

}
