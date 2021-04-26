/*
	Copyright 2017 Data Flow Systems, Inc.

	Description:	Error Logging functions
	
	Change List:
			Original build June 6, 2017 R Brent Saunders
			
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "log.hpp"
#ifdef HYPERTAC
#	include "infolinker.hpp"
	class DFS_Logger *htLog = NULL;
#endif

char LogMsg[1000]= {0};
static char tmdt[100]={"00:00:00 00/00/0000  "};
//********************************************************************************

char *time_date(void){
	time_t t;
	tm *now;
	
	time(&t);
	now  = localtime(&t);
	
	sprintf(tmdt, "%02d:%02d:%02d %02d/%02d/%04d ",
		now->tm_hour,now->tm_min,now->tm_sec, now->tm_mon+1,now->tm_mday,now->tm_year+1900);
	return(tmdt);
}
// *****************************************************************************

char *stripCR(char *Line){
	char *crLoc;
	while((crLoc=strchr(Line,'\n'))) *crLoc = ' ';	// remove CR's
	return(Line);
}
// *****************************************************************************

void LogIt(void){
	int len;
	static bool first = true;
	
	if(first){
		first = false;
#		ifdef USE_INFOLINKER
			htLog =  new DFS_Logger(std::string("/dfs/admin/logs/dfs4.log"));
#		endif	
	}

	LogMsg[sizeof(LogMsg)-1] = 0;	// Make sure string is null terminated
	len = strlen(LogMsg);
	char LineHdr[30] = "";

	if((len < 3) || (len > (int)(sizeof(LogMsg)-sizeof(tmdt)))){
		fprintf( stderr, "%s*** Invalid Log Message: %s\n", LineHdr, LogMsg);	// Bad Message
		return;
	}
#	ifdef USE_INFOLINKER
		char buf[sizeof(LogMsg)];		
		sprintf(buf, "%s| %s", time_date(), LogMsg);				
		htLog->logMe(std::string(stripCR(buf)));

		sprintf(LineHdr, "%s| ", time_date());
#	endif

#	ifdef RAD_IFC
		sprintf(LineHdr, "%s| ", time_date());
#	endif
	
	// Handle partial log messages
	if(memcmp(&LogMsg[strlen(LogMsg)-3], "...", 3) == 0){				// start partial
		len -= 3;
		LogMsg[len]=0;
		fprintf( stderr, "%s%s", LineHdr, stripCR(LogMsg));			// Output time and start of message to error stream
	}
	else if(memcmp(LogMsg, "...", 3) == 0){								// Append partial
		len -= 3;
		strcpy(LogMsg, &LogMsg[3]);
		if(LogMsg[len-1] != '\n'){	
			fprintf( stderr, "%s", stripCR(LogMsg));						// Output middle of message to error stream			
		}
		else{																// End partial
			fprintf( stderr, "%s", LogMsg);								// Output end of message to error stream
		}
	}
	else{
		fprintf( stderr, "%s%s\n", LineHdr, stripCR(LogMsg));		// Non Partial: Output time and message to error stream
	}	
	LogMsg[0]=0;
}
