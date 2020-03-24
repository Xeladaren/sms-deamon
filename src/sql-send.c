#include "sql-send.h"

#include <mariadb/mysql.h>
#include <stdio.h>
#include <string.h>

// *** private Prototype ***


// *** public functions ***

int saveSMSinDB(SMS * sms) {

   MYSQL * mysql = NULL ;

	char db_host[] = "localhost" ;
	char db_user[] = "pi" ;
	char db_pass[] = "" ;
	char db_name[] = "webSMS" ;

	mysql = mysql_init(mysql) ;

	mysql = mysql_real_connect(mysql, db_host, db_user, db_pass, db_name, 0, NULL, 0);

	if (mysql) {
		printf("MySQL connection ok\n");

		struct tm * tm = gmtime(&sms->date);

		char formatedDate[20] ;

		sprintf(formatedDate, "%04d-%02d-%02d %02d:%02d:%02d",
			tm->tm_year + 1900,
			tm->tm_mon,
			tm->tm_mday,
			tm->tm_hour,
			tm->tm_min,
			tm->tm_sec
		);

		printf("formatedDate = %s\n", formatedDate);

      char formatedSender[(strlen(sms->sender)*2)+1] ;
      char formatedMsg[(strlen(sms->msg)*2)+1] ;

      mysql_hex_string(formatedSender, sms->sender, strlen(sms->sender)) ;
      mysql_hex_string(formatedMsg, sms->msg, strlen(sms->msg)) ;

		char cmdSQLformat[] = "INSERT INTO sms (sms_type, sms_date, sms_number_type, sms_number, sms_msg) VALUES (0, '%s', '91', X'%s', X'%s');" ;

		// fixe size of SQL cmd + size of date + size of number + size of msg + '\0'
		size_t cmdSQLSize = 104+19+strlen(formatedSender)+strlen(formatedMsg)+1 ;
		char cmdSQL[cmdSQLSize] ;

		sprintf(cmdSQL, cmdSQLformat, formatedDate, formatedSender, formatedMsg) ;

		printf("SQL = %s\n", cmdSQL);

		int ret = mysql_real_query(mysql, cmdSQL, strlen(cmdSQL));

		if (ret == 0) {
			printf("SQL Query ok\n");

			ret = mysql_commit(mysql) ;

			if (ret == 0) {
				printf("SQL Commit ok\n");
			}
			else {
				printf("SQL Commit error : %d\n", ret);
            return -1 ;
			}

		}
		else {
			printf("SQL Querry error : %d\n", ret);
         return -1 ;
		}

	}
	else {
		printf("MySQL connection fail\n");
      return -1 ;
	}

	mysql_close(mysql);

   return 0 ;

}

// *** private functions ***
