#ifndef AT
#define AT

#include <time.h>

typedef enum pinStatus {SIN_ERROR, SIN_READY, SIM_PIN, SIM_PIN2, SIM_PUK, SIM_PUK2, SIM_UNKNOWN} PinStatus ;

typedef enum phoneStatus {PHONE_READY, PHONE_UNAVAILABLE, PHONE_UNKNOWN, PHONE_RINGING, PHONE_CALL} PhoneStatus ;

typedef struct
{

   time_t date ;     // The timestamp of the SMS
   char * sender ;   // The phone number of the sender
   char * msg ;      // The SMS msg
   char * PDU ;      // the PDU string of the SMS

} SMS;


int initAT(char ttyFILE[], int timeout) ;

int stopAT() ;

int isCallReadyAT() ;

int isSMSReadyAT() ;

int waitCallReady(int timeout) ;

int waitSMSReady(int timeout) ;

PinStatus getPinStatusAT() ;

PinStatus setPinAT(char pin[], int timeout) ;

int setSMSConfig() ;

int writeCustomCmd(char cmd[], int size) ;

int setNewSMSFunction(void (*function)(SMS *)) ;

void printSMS(SMS * sms) ;

void freeSMS(SMS * sms) ;

int loadSMSList() ;


#endif
