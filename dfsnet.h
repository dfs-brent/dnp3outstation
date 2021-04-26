/*
	DFS Networking Definitions
*/


#ifndef DFSNETWORKING_H
#define DFSNETWORKING_H

#define MAX_PACKET_LEN	32768
/*	**************************	*/

/*	STATUS SERVER			*/
/*	COMM TYPES			*/

#define	NET_MIN_VALUE		1
#define	NET_MAX_VALUE		100

/*	These constants represent communication specifiers to all networked
	programs in hypertac.
*/

#define	NEW_STATUS		1
#define	GET_STATUS		2
#define	NEW_RADIO_ERROR		3
#define NEW_OFFLINE		4

#define REGISTER_CLIENT		5
#define ACK_REGISTER		6
#define	CANCEL_CLIENT		7

#define NEW_CONFIG		8
#define	GET_CONFIG		9

#define KILL_TELEMETRY		10
#define SHUTDOWN		11

#define ALARM_ACK		19
#define	ALARM_INFO		20

#define	NEW_LOGIN		21
#define NEW_LOGOUT		22

#define ALARM_PHACK		23		// phone acknowledge
#define LOG_ENTRY		24
#define NEW_NOCHANGE		25

#define CONTROL_FLOAT_ACK	26
#define CONTROL_FLOAT_NACK	27

#define ALARM_LISTENER		12
#define STATUS_LISTENER		13
#define CFG_CHG_LISTENER	14
#define LOGIN_LISTENER		16
#define ALL_LISTENER		17
#define CONTROL_LISTENER	31		// listen for control points

#define SERVER_CHECK_IN		15

#define ALARMS_ONLY		0x00		// listen to all alarms but only alarms
#define STATUS_ONLY		0x01		// listen to status only from all points
#define CONFIG_ONLY		0x02		// listen for config changes
#define LISTEN_ALL		0x04		// listen to everything
#define ALARMS_N_STATUS		0x08		// listen to alams and status
#define	CUSTOM_LISTENER		0x20		// custom screen listener, status and alarms for a list of points



#define CONTROL_MSG			16
#define CONTROL_ACK			17
#define CONTROL_NACK			18

/*	MESSAGE TYPES			*/

#define NET_DIGTT				101
#define	ACK_RESET				102
#define NACK_RESET				103
#define	VERSION					104
#define ACK_CONTROL				17
#define NACK_CONTROL				18
#define NET_ANALOG				107
#define NET_PCUBANK1        			108
#define NET_PCUBANK2        			109
#define NET_PCUBANK3        			110
#define NET_DIGP				111
#define NET_DIGITAL				111
#define	NET_ANALOG_CONTROL			112
#define NET_BATTERYTEST				113
#define NET_BATTERYTEST_ACK			114
#define NET_BATTERYTEST_NACK			115
#define NET_NIM					116
#define NEW_STATUS_UPDATE			117

#define	CONFIG_DRIVERS		50
#define	CONFIG_DFS_STATIONS	51
#define	CONFIG_DIGITAL_POINTS	53
#define	CONFIG_ALARMS		54
#define	CONFIG_AUTO_CONTROLS	55
#define	CONFIG_PARTITIONS	56
#define	CONFIG_USERS		57
#define	CONFIG_ANALOG_POINTS	58
#define	CONFIG_CUSTOM_SCREENS	59
#define CONFIG_VIRTUAL_POINTS	60
#define CONFIG_SCHEDULES	61
#define CONFIG_KEYDEFS		62
#define CONFIG_KEYWORDS		63
#define	CONFIG_POLLING_CHANGE	64

// New HyperServer Config Editor Messages - R.Chamberlain 7-9-01
#define CONFIG_UPDATE		65
#define CONFIG_DRIVER_UPDATE	66


/* Historical data requests */
#define HISTORY_REQUEST		80
#define HISTORY_REQUEST_DATA	81		/* want data within last X hours (0 - 24) */
#define HISTORY_REQUEST_SPAN	82		/* want all data within given span */
#define HISTORY_REQUEST_FROM	83		/* want all data FROM to now */
#define HISTORY_REQUEST_INFO	84

/* Database diagnostic queries */
#define DIAGS			70
#define DIAG_STATION_ALL	70
#define DIAG_STATION		71
#define DIAG_ALMLIST		72
#define DIAG_CTRLIST		73
#define DIAG_VRTLIST		74
#define DIAG_USRLIST		75
#define DIAG_STNLIST		76
#define DIAG_DIGLIST		77
#define DIAG_ANALIST		78
#define DIAG_NETLIST		79

/* DFS Driver Network messages */
#define DRIVER_CONFIGCHANGE	300
#define DRIVER_INJECT		301
#define DRIVER_CONTROL		302
#define DRIVER_DIAGNOSTICS	303
#define DRIVER_SHUTDOWN		304
#define DRIVER_RTLADD		305
#define DRIVER_RTLDELETE	306
#define DRIVER_VERSION		307
#define DRIVER_DIGSTATUS	308
#define DRIVER_ANASTATUS	309
#define DRIVER_RESET		310
#define DRIVER_PULSE		312
#define DRIVER_LAMP		313
#define DRIVER_FLASH		314
#define DRIVER_PCUSTATUS	315
#define DRIVER_PRIORITY		316
#define DRIVER_RETRIES		317
#define DRIVER_AUTOSWITCH	318
#define DRIVER_START		319
#define DRIVER_POLLSCHEME	320
#define DRIVER_BATTERYTEST	321
#define DRIVER_VERSIONALL	322
#define DRIVER_NEWDAY		323
#define DRIVER_LOGGING		324
#define DRIVER_EVENTLOG		325
#define DRIVER_NIMLOGIC		326
#define DRIVER_INJECT_FILE	327
#define MAXDRIVERMSG		327

/* Message sub-types for use by individual db's */
#define GET_FULL_RECORD		200
#define	ADD_NEW_RECORD		201
#define GET_NEXT_RECORD		202
#define DEL_RECORD		203
/* specific to DFS station query's */
#define ADD_MODULE		204
#define DEL_MODULE		205
#define CHG_MODULE		206
#define	LIST_MODULES		207 /* give a list of modules in this station */
#define	LIST_ALL		208 /* giva a list of all stations by number and name */
#define	LIST_NUMBERS		209 /* give a list of all station numbers (in a group)*/
#define LIST_BOTH_PNTS		210 /* list both ana and digital points, in order, for a module */
#define LIST_BY_TYPE		211
#define	COPY_RECORD		212 /* copy a record */
#define COPY_MODULE		213

/* specific to custom screens but can be used generically for other routines */
#define	CREATE_DB		210
#define	DELETE_DB		211
#define RETRIEVE_DB		212
#define DEFSCREEN_DB		213
#define CHECK_DB		214
#define SAVE_SCREEN		215
#define	SAVE_TREND		216
#define	DELETE_TREND		217
#define	RETRIEVE_TREND		218

/* specific to virtual points */
#define	SAVE_LADDER		219
#define UPDATE_RECORDS		220
/* test whether the addresses are configured */
/* in CONFIG_VIRTUAL_POINTS, tests all kinds of points */
/* in CONFIG_AUTO_CONTROLS, tests for auto-control destination points */
#define CHECK_RECORDS		221
/* test whether the ladder logic points are still installed */
/* in CONFIG_VIRTUAL_POINTS, checks ID and EQUATION, and correct order */
/* in CONFIG_AUTO_CONTROLS, checks in-memory config */
#define VERIFY_RECORDS		222

#endif


