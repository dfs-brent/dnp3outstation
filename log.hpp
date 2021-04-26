/*
	Copyright 2017 Data Flow Systems, Inc.

	Description:	Error Logging functions

	Usage:
		printd(anything you can printf);	with up to 8 variables
		printd("buffer start ...")			to start a partial line output
		printd("...buffer append");		to append to a partial line output
		printd("...buffer end/n");			to end a partial line output

	Change List:
			Original build June 6, 2017 R Brent Saunders

*/
#include <stdio.h>

#define printd1(a)					sprintf(LogMsg, a); LogIt();
#define printd2(a,b)				sprintf(LogMsg, a,b); LogIt();
#define printd3(a,b,c);				sprintf(LogMsg, a,b,c); LogIt();
#define printd4(a,b,c,d)			sprintf(LogMsg, a,b,c,d); LogIt();
#define printd5(a,b,c,d,e)			sprintf(LogMsg, a,b,c,d,e); LogIt();
#define printd6(a,b,c,d,e,f)		sprintf(LogMsg, a,b,c,d,e,f); LogIt();
#define printd7(a,b,c,d,e,f,g)		sprintf(LogMsg, a,b,c,d,e,f,g); LogIt();
#define printd8(a,b,c,d,e,f,g,h)	sprintf(LogMsg, a,b,c,d,e,f,g,h); LogIt();
#define printd9(a,b,c,d,e,f,g,h,i)	sprintf(LogMsg, a,b,c,d,e,f,g,h,i); LogIt();
#define GET_MACRO(_1,_2,_3,_4,_5,_6,_7,_8,_9,NAME,...) NAME
#define printd(...) do{GET_MACRO(__VA_ARGS__, printd9, printd8, printd7, printd6, printd5, printd4, printd3, printd2, printd1)(__VA_ARGS__)}while(0)

extern char LogMsg[];

char *time_date(void);
void LogIt(void);
