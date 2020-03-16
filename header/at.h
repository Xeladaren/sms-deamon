#include <time.h>

typedef enum pinStat {ERROR, READY, SIM_PIN, SIM_PIN2, SIM_PUK, SIM_PUK2, UNKNOWN} PinStat ;

typedef struct
{

   time_t date ;
   char sender[15] ;
   char msg[160] ;

} SMS;


int initAT(char ttyFILE[]) ;

int stopAT() ;

int isCallReadyAT() ;

int isSMSReadyAT() ;

int waitCallReady(int timeout) ;

int waitSMSReady(int timeout) ;

PinStat pinStatusAT() ;

PinStat setPinAT(char pin[], int timeout) ;