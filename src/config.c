#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char * conf_db_host = NULL ;
char * conf_db_user = NULL ;
char * conf_db_pass = NULL ;
char * conf_db_name = NULL ;

int loadConfig(char filePath[]) {

   printf("conf file : %s\n", filePath);

   FILE * file = fopen(filePath, "r") ;

   if (file) {

      char line[200] ;

      while (fgets(line, 200, file)) {

         char * findSetting = NULL ;

         if((findSetting = strstr(line, "database_host="))) {

            char * settingValue = strchr(findSetting, '=') + 1 ;
            *strchr(findSetting, '\n') = '\0' ;

            conf_db_host = (char *) malloc(strlen(settingValue)+1);
            strcpy(conf_db_host, settingValue);

            printf("host = |%s|\n", conf_db_host);

         }
         else if((findSetting = strstr(line, "database_user="))) {

            char * settingValue = strchr(findSetting, '=') + 1 ;
            *strchr(findSetting, '\n') = '\0' ;

            conf_db_user = (char *) malloc(strlen(settingValue)+1);
            strcpy(conf_db_user, settingValue);

            printf("user = |%s|\n", conf_db_user);

         }
         else if((findSetting = strstr(line, "database_pass="))) {

            char * settingValue = strchr(findSetting, '=') + 1 ;
            *strchr(findSetting, '\n') = '\0' ;

            conf_db_pass = (char *) malloc(strlen(settingValue)+1);
            strcpy(conf_db_pass, settingValue);

            printf("pass = |%s|\n", conf_db_pass);

         }
         else if((findSetting = strstr(line, "database_name="))) {

            char * settingValue = strchr(findSetting, '=') + 1 ;
            *strchr(findSetting, '\n') = '\0' ;

            conf_db_name = (char *) malloc(strlen(settingValue)+1);
            strcpy(conf_db_name, settingValue);

            printf("name = |%s|\n", conf_db_name);

         }

      }

      fclose(file) ;
   }

   return 0 ;
}
