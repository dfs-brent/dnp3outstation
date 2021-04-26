#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include "dnp3.hpp"

int driver_num = -1;

#define BUILD	'b'
#define TEST	't'
#define CLIENT	'c'
#define SERVER	's'

#define LOOP_TIME 1000

bool serverMode = false;
static char event[1000];

void dnp3requestFullStatus(void);
void dnp3serverInit(void);
void dnp3clientInit(void);
void dnp3serverService(char *event);
void dnp3clientService(char *event);

void dnp3buildConfigs(void){;} ///
void dnp3test(void){;} ///
void dnp3shutDown(void){;} ///
void dnp3receiveEvent(char *event){;}///

// *****************************************************************************
//	Argument parser.  Returns a structure with an option and value of the arguments
//		Two arguments are allowed in any order.  
//		Arguments with '-' prepended are the option. Otherwise they are considered the value
//		Invalid value or option are set to -1.

struct Arg{
	public:
	int opt;
	int val;
}arg;

Arg parseArgs(int argc, char **argv){
	arg.opt = -1;
	arg.val = -1;
	
	//printd("argc: %d\n", argc);
	if(argc > 1){
		for(int i=1; i<argc; i++){
			//printd("argv: %s\n", argv[i]);

			if(argv[i][0] == '-'){	arg.opt = argv[i][1];		}
			else{					arg.val = atoi(argv[i]);	}
		}
	}
	//printd("val = %d opt = %c\n", arg.val, arg.opt);
	return(arg);											// Default mode is server
}
// *****************************************************************************

void bye(int sig){
	
	signal(sig, SIG_IGN);
	printd("\n\ndfs4 closing\n");
	
	dnp3shutDown();

	exit(0);
}
// *****************************************************************************
void mSleep(int mSecs){

        struct timespec ts ;
        ts.tv_sec = mSecs / 1000;
        ts.tv_nsec= (mSecs % 1000) * 1000000;
        while(nanosleep(&ts,NULL));           // ignore interruptions
}
// *****************************************************************************
static bool toggle = false;

int main(int argc, char *argv[])
{
	signal(SIGINT, bye);
	signal(SIGHUP, bye);
	signal(SIGTERM, bye);


	// Parse command line arguments
	Arg arg = parseArgs(argc, argv);
	//printd("driver: %d opt: %d\n", arg.val, arg.opt);

	switch(arg.opt){
		case BUILD: 	{	dnp3buildConfigs();		exit(-1);	}
		case TEST:		{	dnp3test();				exit(-1);	}
		case CLIENT:	{	serverMode = false;		break;		}
		case SERVER:	{	serverMode = true;		break;		}
  		default:		{	arg.val = -1;						}
	}
	
	if(arg.val == -1){
		printf("usage: dfs4 [driver] [-b] | [-c] | [-s]\n");
		printf("       driver = hypertac driver number\n");
		printf("       -b to build driver info from infoserver\n");
		printf("       -c to run as client\n");
		printf("       -s to run as server\n");
		exit(-1);			
	}
	driver_num = arg.val;

	if(serverMode)	printd("Server Mode on Driver %d\n", driver_num);
	else			printd("Client Mode on Driver %d\n", driver_num);
	
	
	if(serverMode){
		dnp3serverInit();
		dnp3requestFullStatus();
	}
	else{
		dnp3clientInit();
	}
	
	
	while(1){
	do{
		dnp3receiveEvent(event);
		if(serverMode)	dnp3serverService(event);
		else			dnp3clientService(event);
		if(*event) mSleep(20);
	} while(*event);				
		mSleep(LOOP_TIME);
	}
}

