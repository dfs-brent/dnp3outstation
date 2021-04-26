/*

	Author: Richard Chamberlain
		Data Flow Systems, INC

	Source: dfs.h

	Description:

		This is the master header file for all dfs applications.  This file contains all
		constants used to describe everything from module types to station and point numbers
		limits.  This file should be included in every compilation and the information in
		this file should be used to avoid conflicting values from another source.

		This is formatted as a C source file to be used in both c and c++ applications.
*/

#ifndef DFS_HPP
#define DFS_HPP


/* This is for use by C programs */
#if defined(__cplusplus)
extern "C" {
#endif

#define	TRUE	1
#define FALSE	0

#define	OFFLINE	1
#define	ONLINE	0


/* TYPEDEFS */
/*----------*/
typedef unsigned int  Uint;
typedef unsigned int  UINT;

typedef unsigned char Uchar;
typedef unsigned char UCHAR;

typedef char	flag;



/* STATION LIMITS */
/* all station variables should be of type int */
					/* polling 1-250, 256-511. Logical 1-999 */
#define 	MINSTN		1		/* lowest station number */
#define 	MAXSTN		511		/* highest station number */
#define 	STBRKLOW	251		/* cant use 251 - 255 */
#define 	STBRKHIGH	255
#define 	MAX_LOG_STN	999		/* logical station address max, for aliasing */
#define		MAXREPEATERS	4


/* MODULE TYPES AND MAPPINGS */
/*---------------------------*/

#define		MINMOD		1
#define		MAXMOD		0xF
#define		MINMODC		'A'
#define		MAXMODC		'O'
#define		RIMMOD		0x10
#define		RIMMODC		'R'
#define		PLCMOD		0x11
#define		PLCMODC		'Q'

#define		DIGMON		100
#define		DIGCON		101
#define		ANAMON		102
#define		ANACON		103
#define		DPULSE		104
// new types 01/22/04 -RC
#define		ENG_IN		105
#define		ENG_IO		106
#define		PULSE_IN	107
#define		PULSE_OUT	108	// support pulse out func on dcm's

#define		DMM001		1
#define		DMM002		2
#define		DCM001		3
#define		DCM002		4
#define		AMM001		5
#define		AMM002		6
#define		ACM001		7
#define		RIM004		8
#define		RIM005		9
#define		RIM006		10
#define		PCU001		11
#define		PCM001		12
#define		BPR001		13
#define		DCM003		14
#define		PAM001		15
#define		DCM031		16 		/* DCM003 -1 */
#define		DCM032		17		/* DCM003 -2 */
#define		PLC001		18

/* update high mod def when adding new module types. It should equal the highest integer from the
   module types in the previous def
*/
#define		LOW_MOD_DEF	1
#define		HI_MOD_DEF	18
#define		MAXMODS		16		/* 15 modules per station + rim */

/* MODULE STRINGS */
#define		DMM001_S	"DMM001"
#define		DMM002_S	"DMM002"
#define		DCM001_S	"DCM001"
#define		DCM002_S	"DCM002"
#define		AMM001_S	"AMM001"
#define		AMM002_S	"AMM002"
#define		ACM001_S	"ACM001"
#define		RIM004_S	"RIM004"
#define		RIM005_S	"RIM005"
#define		RIM006_S	"RIM006"
#define		PCU001_S	"PCU001"
#define		PCM001_S	"PCM001"
#define		BPR001_S	"BPR001"
#define		DCM003_S	"DCM003"
#define		PAM001_S	"PAM001"
#define		DCM031_S	"DCM031"
#define		DCM032_S	"DCM032"
#define		PLC001_S	"PLC001"


/* STRING LENGTHS */
/*----------------*/
/* updated 9/29/04 to match actual sql field widths!!! */
#define		MOD_STR_LEN	6
#define		MAX_LABEL_LEN	10	/* was 8 */
#define		UNIT_LABEL_LEN	10	/* was 7 */
#define		PNT_COMMENT_LEN	15	/* obsolete */
#define		STN_NAME_LEN	20
#define		PNT_NAME_LEN	20
#define		GRP_NAME_LEN	20	/* used for driver name */
#define 	MAXPORTLEN 	30	/* was 20 */
#define 	MAXPATHLEN 	32	/* obsolete */
#define		MAXPARTITIONLEN 20	/* was 10 */
#define 	MAXPHONENUMLEN	25	/* obsolete */
#define		MAXUSERLEN	24
#define		MAXPASSLEN	32	/* was 8 */
#define		MAXPRINTERLEN	12	/* shorten to 8? */
#define		MAXKEYWORDLEN	20	/* was 12 */

/* POLLING TYPES */
#define		DFS		1
#define		MOT		2		/* motorola */
#define		CDP		3		/* cellular data packet */
#define		ABD		4		/* Allen Bradley DF1 */
#define		NIM		5		/* network interface module for DFS */
#define		MOD		6		/* modbus */
#define		NETDFP		7
#define     MES     8

#define		LOWPOLLTYPE	1
#define		HIPOLLTYPE	6

/* STRINGS */					/* any new types MUST have a 3-CHAR str */
						/* strings over 3 chars will cause trouble*/

#define		DFS_S		"DFS"
#define		MOT_S		"MOT"
#define		CDP_S		"CDP"
#define		ABD_S		"ABD"		/* was AET_S "AET" */
#define		NIM_S		"NIM"
#define		MOD_S		"MOD"
#define		MES_S		"MES"

/* PORT (POLLING) PARAMATERS */
/*---------------------------*/

#define 	MAXPORTS   	8

#define		OUT_OF_SERVICE	1		/* this (name only) must always be lowest */
#define		LOW_PRIORITY	2
#define		NRM_PRIORITY	3
#define		HI_PRIORITY	4
#define		ENG_PRIORITY	5		/* this (name only) must always be highest */

						/* these constants must be only 1 DIGIT!! */
#define		LOW_POLL_MIN	9		/* at least poll once every 20 loops */
#define		LOW_POLL_DEF	4		/* by default, poll low pri once every 4 loops */
#define		LOW_POLL_MAX	2		/* for low pri, no more than every other loop */
#define		HI_POLL_MIN	2		/* no less than twice per loop */
#define		HI_POLL_DEF	2		/* by default, poll hi pri twice per loop */
#define		HI_POLL_MAX	4		/* any more than 4 and polling gets crazy */

#define		OFFLINE_COUNT_MIN	1
#define		OFFLINE_COUNT_DEF	8
#define		OFFLINE_COUNT_MAX	99

#define		RETRIES_MIN	1
#define		RETRIES_DEF	2
#define		RETRIES_MAX	9


#define		TIMEOUT_MIN	200		/* in milliseconds */
#define		TIMEOUT_DEF	1000
#define		TIMEOUT_MAX	5000

/* PARTITION INFO */
/*----------------*/

#define		MINPARTITION	0
#define		MAXPARTITION	7
#define		MAXPARTITIONS	8


/* STREAM PARAMETERS */
/*-------------------*/

#define 	COMMA_S		","
#define		COMMA_C		','
#define		COMMA		0x2C


/* FILES */
/*-------*/
#define		PORTFILE   	"/dfs/hypertac/dbase/port.dbf"
#define		PORTCDX		"/dfs/hypertac/dbase/port.cdx"

#define 	STNFILE   	"/dfs/hypertac/dbase/stations.dbf"
#define		STNCDX		"/dfs/hypertac/dbase/stations.cdx"

#define		DFSDIGFILE	"/dfs/hypertac/dbase/digpnt.dbf"
#define		DFSDIGCDX	"/dfs/hypertac/dbase/digpnt.cdx"

#define		DFSANAFILE	"/dfs/hypertac/dbase/anapnt.dbf"
#define		DFSANACDX	"/dfs/hypertac/dbase/anapnt.cdx"

#define		VRTLFILE	"/dfs/hypertac/dbase/vrtlpnts.dbf"
#define		VRTLCDX		"/dfs/hypertac/dbase/vrtlpnts.cdx"

#define		PARTFILE   	"/dfs/hypertac/dbase/partition.dbf"
#define		PARTCDX		"/dfs/hypertac/dbase/partition.cdx"

#define		CONTROLFILE	"/dfs/hypertac/dbase/control.dbf"
#define		CONTROLCDX	"/dfs/hypertac/dbase/control.cdx"

#define		ALARMFILE	"/dfs/hypertac/dbase/alarms.dbf"
#define		ALARMCDX	"/dfs/hypertac/dbase/alarms.cdx"

#define		KEYWORDFILE	"/dfs/hypertac/dbase/keywords.dbf"
#define		KEYWORDCDX	"/dfs/hypertac/dbase/keywords.cdx"

#define		KEYDEFSFILE	"/dfs/hypertac/dbase/keydefs.dbf"
#define		KEYDEFSCDX	"/dfs/hypertac/dbase/keydefs.cdx"

#define		QUALFILE	"/dfs/hypertac/dbase/qual.dbf"
#define		QUALCDX		"/dfs/hypertac/dbase/qual.cdx"

#define		USERSFILE	"/dfs/hypertac/dbase/users.dbf"

#define		JOURNALDIR	"/dfs/hypertac/dbase/journals/"
#define		SCHEDULEDIR	"/dfs/hypertac/dbase/schedules/"
#define		ALARMLOGDIR	"/dfs/hypertac/dbase/logs/alarms/"
#define		CONTROLOGDIR	"/dfs/hypertac/dbase/logs/controls/"
#define		ACCESSLOGDIR	"/dfs/hypertac/dbase/logs/access/"
#define		MANUALOGDIR	"/dfs/hypertac/dbase/logs/manual/"
#define		RADIOERRDIR	"/dfs/hypertac/dbase/logs/raderr/"

#define 	VOICEDIR	"/dfs/hypertac/dbase/voice/"
#define		VOICEDB		"/dfs/hypertac/dbase/voicedb.dbf"
#define		VOICEFILE	"/dfs/hypertac/voice/"

#define		CUSTOM_DIR	"/dfs/hypertac/dbase/screens"

#define		PATCHDIR	"/dfs/hypertac/dbase/patching"
#define		PATCHFILE	"/dfs/hypertac/dbase/patching/patching.dbf"

/* ADDRESSING CONSTANTS */
/*----------------------*/

#define		DFS_ADDR_LEN	9		/* 4xSTN, 1xMOD, 3xPNT + 1 for expansion */
#define		MAXPOINT	999		/* allow three digits for point */
#define		MAXADDRLEN	12		/* all addresses must fit in 10 char */


/* ALARM AND ALARM DELAY CONSTANTS */
/*---------------------------------*/

#define		MINALARMDELAY	10L		/* min delay of 10 seconds */
#define		MAXALARMDELAY	172800L		/* max delay of 48 hours */

#define		MINSNOOZEDELAY	10L		/* min snooze of 10 seconds */
#define		MAXSNOOZEDELAY	86400L		/* max snooze of 24 hours */

/* REPORTING CONSTANTS */
/*---------------------*/
						/* pre-defined reports */
#define		MAXRPTLEN		20	/* length of report name string */

#define		DETAIL_RPT		"DETAIL"
#define		RADIO_ERR_RPT		"RADIO ERROR"
#define		DERIVED_FLOW_RPT	"DERIVED FLOW"
#define		TRANSITION_RPT		"TRANSITION"
#define		ANALOG_RPT		"ANALOG"
#define		PULSE_ACC_RPT		"PULSE"

/* RADIO ERROR CONSTANTS */
/* --------------------- */

#define RERR_ADDRLEN	5			/* dont use MAXADDRLEN, we only want station and mod at most */
#define RERR_DESCLEN	46
#define RERR_MAXCOUNT	100			/* only allow 500 records per hour */
						/* pre-defined radio error types */
#define RERR_UNKNOWN	0			/* unknown error */
#define RERR_NORESPONSE	1
#define RERR_ABORT	2
#define RERR_BADADDR	3			/* bad address format or wrong address responded */
#define RERR_BADMSG	4			/* the body of the message was in error or unexpected */
#define RERR_CHECK	5			/* bad checksum or CRC */


/* COMMON STRUCTURES  */
/*--------------------*/

typedef struct {				/* dfs type address structure */

	int station;
	int point;
	int type;

	char module;
} DFSADDR;

/* NETWORK SERVICES */
/* ---------------- */

#define		DFSBASE		3115
#define		DFS_DRIVER	3140		/* this uses 3140 - 3147 for up to 8 dfs drivers */

// This def is the path to all runtime unix socket pipes
#define RUNTIME_PATH "/dfs/hypertac/runtime"
#define RUNTIME_SERVER "infoserver"

/* VOICE CONSTANTS */
/*-----------------*/
#define 	DEADBAND_V	10			/* default dead band of 10 minutes */
#define		USERCODELEN 8

/* LIBRARY PROTOTYPES */
/*--------------------*/

						/* stncnvt.c */
char limits(int, int, int);
char ulimits(unsigned int, unsigned int, unsigned int);
int  llimits(long, long, long);
int validstn(int);				/* address number check */
int validpstn(int);				/* polling number check */
int validxstn(int);				/* extended checking for ports other than primary */
int validxpstn(int);				/* extended polling number checking */
void dfshex(int, char*);
char *dfshex_c(int);
int hex_to_int(char);
int get_dfshex(char, char);
char *msgaddr(int, char);
int  xstn(int);					/* remove the pre-pended port # and return station */
char dfs_point_hex(int);			/* dfs hex to fit a point number(1-36) into one char */
						/* modcnvt.c */
char *mod_char_type(int);
int  mod_int_type(const char *);
int  mod_char_to_int(char);
char mod_int_to_char(int);
int validmod(char);
char translate_mod(char);
char reverse_translate(char);			/* A-O -> 1-F */
int compatible_modtypes(int, int);		/* see if two module types are compatible */
						/* portcnvt.c */
int  port_int_type(const char *);
char *port_char_type(int);

						/* addr.c */
char *addr_to_string(DFSADDR *);
char *addrargs_to_string(int, char, int);
DFSADDR *addr_to_struct(char *);
int  valid_addr(char *, int);
int  valid_cfg_addr(char *, int, int);
int  validpoint(int, int, int);
int  checkpoint(int, int);
int  getpnttype(int, int);
int  decode_stn_num(const char *);
char decode_module(const char *);
int  decode_point(const char *);
int  countDecimalPlaces(double);		/* return the number of digits after the decimal */

						/* htime.c */
long now(void);
long timediff(long,long);
long tnap(long);				/* take a nap in milliseconds */
char *stringtime(long);				/* convert seconds past midnight to time of day string */
int hour_of_day(long);
int minute_of_day(long);


#if defined(__cplusplus)
}
#endif

#endif

