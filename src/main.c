#include <string.h>
#include <stdio.h>

#include "at.h"

int main(int argc, char const *argv[]) {

	initAT("/dev/ttyS0");

	PinStat pinStat = pinStatusAT() ;

	printf("pinStat = %d\n", pinStat);

	if (pinStat != READY) {

		pinStat = setPinAT("0000", 10000) ;

		printf("pinStat = %d\n", pinStat);

		waitCallReady(10000);

		waitSMSReady(10000);

	}

	stopAT();

	return 0;
}
