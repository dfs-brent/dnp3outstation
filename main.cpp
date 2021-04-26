#include <ifaddrs.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <opendnp3/outstation/UpdateBuilder.h>

#include "dnp3.hpp"

extern void dnp3QueueEvent(int index, char *type, float val, time_t tstamp, bool offline);
///extern int UseAsOutstation(uint16_t port, std::string OutstationIP, uint16_t MasterAddress, uint16_t OutstationAddress, bool unsolicitedEnabled, uint16_t DatabaseSize, uint16_t EventBufferSize);
extern int UseAsOutstation(uint16_t port, std::string OutstationIP, uint16_t MasterAddress, uint16_t OutstationAddress, bool unsolicitedEnabled, uint16_t DatabaseSize, uint16_t EventBufferSize, void (*dnp3Control)());

extern int maxPoints(const char *pointType);

class opendnp3::UpdateBuilder builder; 


int driver_num = -1;

// Command line arguments
#define BUILD	 'b'
#define TEST	 't'
#define USAGE	 'h'

#define LOOP_TIME 100

static char event[1000];

bool dnp3clientInit(void);
void dnp3requestFullStatus(void);
void dnp3clientService(void);
void dnp3buildConfigs(void);
void dnp3shutDown(void); 

void dnp3test(void){;} ///

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
	if(argc){
		for(int i=1; i<argc; i++){
			//printd("argv: %s\n", argv[i]);

			if(argv[i][0] == '-'){	arg.opt = argv[i][1];		}
			else{					arg.val = atoi(argv[i]);	}
		}
	}
	//printd("val = %d opt = %c\n", arg.val, arg.opt);
	return(arg);
}
// *****************************************************************************

void bye(int sig){
	
	signal(sig, SIG_IGN);
	printd("\n\n dnp3 closing\n");
	
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

time_t epoch(char *timeString){
	struct tm tm;
	// 2020-12-15 13:40:38
	strptime(timeString, "%Y-%m-%d %H:%M:%S", &tm);
	time_t t = mktime(&tm);
	//printd("epoch: timeString: %s t: %ld\n", timeString, t);
	return(t);
}
// ****************************************************************************
				
void PauseMsec(int mSec){
	struct timespec ts;
	ts.tv_sec = mSec / 1000;
	ts.tv_nsec= (mSec % 1000) * 1000000;

	while(nanosleep(&ts,&ts) != 0);
}
// ****************************************************************************
// Add escapes to reserved characters in string

char *esc_str(const char *line){
	const char *pSrc  = NULL;
	char *pDest = NULL;
	static char escBuf[100] = {0};
	const char escChars[] = { "#*" };
		
	pSrc = line;
	pDest = escBuf;
	while((*pSrc) && (pDest < escBuf + sizeof(escBuf))){
		if(strchr(escChars, *pSrc)){
			*pDest++ = '\\';
		}
		*pDest++ = *pSrc++;
	}
	*pDest = 0;
	return(escBuf);
}
// *****************************************************************************
char *getSysResult(const char *cmd){
	static char buf[200];
	FILE *in;
	float tout = 1;		// timeout in seconds
	char *p=NULL;

	char *cmdBuf = esc_str(cmd);	// Escape any special characters

	memset(buf, 0, sizeof(buf));
	
	// Send the command
	if((in = popen(cmdBuf, "r"))){
		PauseMsec(100);
		// Get the the result or timeout
		while(((fgets(buf, sizeof(buf), in)) == NULL)
		 && ((tout -= .1) > 0)){
			PauseMsec(100);
		}
		pclose(in);
	}
	if((p=strchr(buf, '\n'))) *p = 0;	// remove cr
	return(buf);
}
// *****************************************************************************
#define ESC 0x1B

char *getIP(void){
	static char *ip = NULL;
#	ifdef USE_DFS_IP
		char *reply = getSysResult("dfs_ip | grep ' ip'");
		//printd("reply: %s ip: %s\n", reply, ip);
		
		ip = strchr(reply, ':');
		//printd("reply: %s ip: %s\n", reply, ip);
		if(ip){
			while(*ip != '.') ip++;				// advance to 1st .
			do{ ip--; } while(isdigit(*ip));	// backup to 1st octet
			ip++;
			if(strchr(ip,ESC)) *strchr(ip, ESC) = 0;	// remove end esc
			///for(int i=0; i<14; i++) printd("%X ", ip[i]);
			printd("ip[%d]: %s\n", (int)strlen(ip), ip);
		}
		else{
			printd("Error: could not get IP from dfs_ip!\n");
			ip = reply;
			*ip = 0;
		}
#	else
		char cmd[] = {"cat /etc/network/interfaces|grep -A 100 enp4s0|grep address"};
		char *reply = getSysResult(cmd);
		//printd("reply: %s ip: %s\n", reply, ip);
		
		ip = strstr(reply, "address ");
		//printd("reply: %s ip: %s\n", reply, ip);
		if(ip){
			ip = strchr(reply, ' ') + 1;
			//printd("ip[%d]: %s\n", (int)strlen(ip), ip);
		}
		else{
			printd("Error: could not get IP from interfaces!\n");
			ip = reply;
			*ip = 0;
		}
#	endif
	return(ip);
/*
	// Use system call to get ip
	struct ifaddrs *addrs;
	
	getifaddrs(&addrs);
	struct ifaddrs *tmp = addrs;

	while (tmp) 
	{
		if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
		{
			struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
			printf("%s: %s\n", tmp->ifa_name, inet_ntoa(pAddr->sin_addr));
		}
		tmp = tmp->ifa_next;
	}
	freeifaddrs(addrs);
*/	
	return(ip);
}

/// Future: Use to accept controls from the api
//	The opendnp3 library will need to be modified to que controls and
//	allow the api to poll for them.  The api will call this routine
//	with an array of these controls.
void dnp3Control(void){
	
}

// *****************************************************************************
void *apiThread(void *argin)
{
	char *ip = getIP();
	int masterAddress = 3; 		/// Make configurable
	int outstationAddress = 5;	/// Make configurable
	int dbSize = 10;
	int evSize = 1000;

	// Determine dnp3 database size by finding max number of DI,DO,DP,AI and AO points
	int diSize = maxPoints("DI");	if(diSize > dbSize) dbSize = diSize;
	int doSize = maxPoints("DO");	if(doSize > dbSize) dbSize = doSize;
	int dpSize = maxPoints("DP");	if(dpSize > dbSize) dbSize = dpSize;
	int aiSize = maxPoints("AI");	if(aiSize > dbSize) dbSize = aiSize;
	int aoSize = maxPoints("AO");	if(aoSize > dbSize) dbSize = aoSize;
	dbSize++;	// Add 1 for index starting at 1 instead of 0

	printd("Set up dnp3 api: port=20000 ip=%s masterAddress=%d outstationAddress=%d dbSize=%d\n", ip, masterAddress, outstationAddress, dbSize);
	//                 port,  OutstationIP, MasterAddress, OutstationAddress, unsolicitedEnabled, DatabaseSize, EventBufferSize);
///	if(UseAsOutstation(20000, ip, masterAddress, outstationAddress, true, dbSize, evSize)){
	if(UseAsOutstation(20000, ip, masterAddress, outstationAddress, true, dbSize, evSize, dnp3Control)){
		printd("api exited\n");
		exit(1);
	}
	return(0);
}


static bool toggle = false;

// *****************************************************************************

int main(int argc, char *argv[])
{	bool usage = false;
	bool activity_test = false;
	
	signal(SIGINT, bye);
	signal(SIGHUP, bye);
	signal(SIGTERM, bye);

	Arg arg = parseArgs(argc, argv);
	//printd("argc: %d driver: %d opt: %d\n", argc, arg.val, arg.opt);

	if(argc > 1){
		// Parse command line arguments
		switch(arg.opt){
			case BUILD: 	{	dnp3buildConfigs();		exit(-1);	}
///			case TEST:		{	dnp3test();				exit(-1);	}
			case TEST:		{	activity_test = true;	break;		}
			default:		{	usage = true;			break;		}
		}
	}
	if(usage){
		printf("usage: dnp3 [-b] | [-t] | [-h]\n");
		printf("       '-b' to build driver info from database\n");
		printf("       '-t' to run regression test\n");
		printf("       '-h' to print this message\n");
		exit(-1);			
	}
	
	driver_num = arg.val;

	printd("Client(Outstation) Mode\n");
	dnp3clientInit();
	
	pthread_t api_thread;
	pthread_create(&api_thread,NULL,apiThread,(void *)NULL);
	
	time_t nextTest = time(NULL) + 2;///
	int val = 0;
	int offset = 0;
	char di[] = {"DI"};
	char ai[] = {"AI"};
	while(1){
		do{
			dnp3clientService();

		} while(*event);				
		mSleep(LOOP_TIME);
		
		/// change digital and analog point 0 periodically for testing
		if(activity_test && (time(NULL) >= nextTest)){
///			val ^= 1;			
///			dnp3QueueEvent(0, di, (bool)val, time(NULL)-10, false);
			offset = (offset + 10) % 60;
			val ^= 1;			
			dnp3QueueEvent(0, di, (bool)val, time(NULL)-offset, false);
			dnp3QueueEvent(0, ai, (double)(time(NULL) % 100) + .5, time(NULL), false);
			nextTest = time(NULL) + 60;
		}
	}
}

