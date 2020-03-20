#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "pdu-decode.h"

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

   return 0;

}

// 02308181744040

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

int PDUDecodeData(char inputData[], char outputData[], size_t outputSize) {

   size_t dataInSize = strlen(inputData) / 2 ;
   unsigned char dataIN[dataInSize] ;
   memset(dataIN, 0, dataInSize) ;

   for (size_t i = 0; i < strlen(inputData); i += 2) {

      char octet[3] ;
      memset(octet, 0, 3);
      memcpy(octet, &inputData[i], 2);

      sscanf(octet, "%hhx", &dataIN[i/2]);

   }

   memset(outputData, 0, outputSize) ;

   int n = 1 ;
   for (size_t i = 0; i < outputSize-1; i++) {

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
      outputData[i] = valOut ;

   }

   return 0 ;

}
