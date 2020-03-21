#include <time.h>

typedef enum pinStatus {SIN_ERROR, SIN_READY, SIM_PIN, SIM_PIN2, SIM_PUK, SIM_PUK2, SIM_UNKNOWN} PinStatus ;

typedef enum phoneStatus {PHONE_READY, PHONE_UNAVAILABLE, PHONE_UNKNOWN, PHONE_RINGING, PHONE_CALL} PhoneStatus ;

typedef struct
{

   time_t date ;
   size_t senderSize ;
   size_t msgSize ;
   char * sender ;
   char * msg ;
   char * PDU ;

} SMS;


int initAT(char ttyFILE[], int timeout) ;

int stopAT() ;

int isCallReadyAT() ;

int isSMSReadyAT() ;

int waitCallReady(int timeout) ;

int waitSMSReady(int timeout) ;

PinStatus getPinStatusAT() ;

PhoneStatus getPhoneStatusAT() ;

PinStatus setPinAT(char pin[], int timeout) ;

int setSMSConfig() ;

int writeCustomCmd(char cmd[], int size) ;

int setNewSMSFunction(void (*function)(SMS *)) ;

void printSMS(SMS * sms) ;

void freeSMS(SMS * sms) ;
