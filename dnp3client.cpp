// ****************************************************************************
// Dnp3 client (outstation) routines

//	Uses database rt_status table to build an indexed list of all of the points to be sent to server
//	The point tag string and index are stored in database table dnp3tagIx
//	A database trigger(dnp3event) is set up by script dnp3trigger.mysql to copy changes in rt_status to event table dnp3events
//	We check this database once per second to see if there are any changes and forward them to the server

// TBD:
//
//	1. Point offline status - add offline param to queueDigitalEvent nad queueAnalogEvent
//	2. Digital and analog controls - update libopendnp3 to write to a named pipe when control received
//	3. Digital and analog control point status - add modify controlAction routines to only do builder.Update.
//	4. Implement ht4 config changed flag and vtscada autoconfiguration feature
//	5. Make opendnp3 database size configurable on a type-by-type basis (i.e. 100 DI's, 20 DO's, etc.)
//	
// ****************************************************************************
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <opendnp3/app/DNPTime.h>

#include "log.hpp"
#include "dnp3.hpp"
#include "db_wrapper.hpp"

#define IO_FOLDER "/home/dfs/io/"
#define DCON_FIFO "/home/dfs/io/dnp3DigitalControl"
#define ACON_FIFO "/home/dfs/io/dnp3AnalogControl"

#define IX_MAX 2147483647			// Maximum ht4 event table index

// dnp3events database table column definitions
#define EVENT_DB_IX   0				// Event database index
#define EVENT_DB_TAG  1				// Event database point tag
#define EVENT_DB_TYP  2 			// Event database point type
#define EVENT_DB_VAL  3 			// Event database point value
#define EVENT_DB_STMP 4 			// Event database point time stamp
#define EVENT_DB_REL  5 			// Event database point reliability

// rt_status database table column definitions
#define RTSTATUS_ADR  0				// Real time status database address 
#define RTSTATUS_TYP  1				// Real time status database type 
#define RTSTATUS_VAL  2				// Real time status database value
#define RTSTATUS_STMP 4				// Real time status database time stamp
#define RTSTATUS_REL  5		 		// Real time status database reliability

// dnp3tagIx database table column definitions
#define TAG_DB_TYPE 0		 		// Tag Index database IO type
#define TAG_DB_IX   1		 		// Tag Index database IO index
#define TAG_DB_TAG  2		 		// Tag Index database TAG

// digpnt/anapnt database table column definitions
#define PNT_ADR		0				// Point address
#define PNT_NAME	1				// Point name

#define TAG_FILE  "/dfs/hypertac/bin/drivers/dnp3tags.csv"		// Tag index file for export to remote server
#define TMP_FILE  "/dfs/hypertac/bin/drivers/dnp3tmp.csv"
#define BAK_FILE  "/dfs/hypertac/bin/drivers/dnp3bak.csv"
#define TRIG_FILE "/dfs/hypertac/bin/drivers/dnp3event.mysql"	// Sql script to set up trigger
#define CONT_FILE "/tmp/outstation.json"

extern time_t epoch(char *timeString);

static class Db_wrapper *db = new Db_wrapper();

// From dnp3 api
extern bool queueDigitalEvent(uint16_t index, bool value, bool hasTime, struct opendnp3::DNPTime time, bool offline);
extern bool queueAnalogEvent(uint16_t index, double value, bool hasTime, struct opendnp3::DNPTime time, bool offline);
extern bool queueDigitalControl(uint16_t index, bool value, bool hasTime, opendnp3::DNPTime time, bool offline);
extern bool queueAnalogControl(uint16_t index, double value, bool hasTime, opendnp3::DNPTime time, bool offline);
extern bool queueCounterEvent(uint16_t index, uint32_t value, bool hasTime, struct opendnp3::DNPTime time, bool offlne);

// ****************************************************************************
// Build the dnp3 index database and file
//	Database is used internally to translate between tag string and index
//	File is used to configure remote server indexes

void dnp3buildConfigs(void){
	
	// there is a dnp3 indexe for each i/o type
	int ixDI = 1;			// Digital Input  dnp3 index
	int ixDO = 1;			// Digital Output dnp3 index
	int ixDP = 1;			// Digital Pulse  dnp3 index
	int ixAI = 1;			// Analog Input   dnp3 index
	int ixAO = 1;			// Analog Output  dnp3 index
	char *type = NULL;		// I/O type (i.e. DI,DO,DP,AI,AO)
	char *tag = NULL;		// I/O tag (i.e. "1200B3")
	char *name = NULL;		// I/O name (i.e. "Pump Run")
	char cmd[100]={0};		// Database command string
	
	MYSQL_ROW rtStatus;	
	MYSQL_ROW point;
	
	FILE *tagFile = fopen(TAG_FILE, "w");
	static class Db_wrapper *dbStat = new Db_wrapper();
	static class Db_wrapper *dbTag = new Db_wrapper();

	if(tagFile){
		fprintf(tagFile, "type,ix,tag,name\n");
	}
	else{
		printf("Cannot write to Tag File %s!  Are you root?\n", TAG_FILE);
		exit(0);
	}
	// Get string addresses, index and type from real time status table
	dbStat->set_sql("SELECT * FROM rt_status");
	dbStat->execute_select();

	// Add them as tags into the tag database	
	dbTag->set_sql("DROP TABLE IF EXISTS dnp3tagIx;");
	dbTag->execute_statement();
	dbTag->set_sql("CREATE TABLE dnp3tagIx(type CHAR(3), ix int, tag VARCHAR(100))");
	dbTag->execute_statement();

	while((rtStatus = dbStat->get_next_row())){
		type = rtStatus[RTSTATUS_TYP];
		tag = rtStatus[RTSTATUS_ADR];
		
		// Increment dnp3 indexes based on point type
		int ix = 0;
		if(*type == 'E') *type = 'A'; 		// Engineering points are same as analog
		if(strstr(type, "DI")) ix = ixDI++;	// Each type has it's own index
		if(strstr(type, "DO")) ix = ixDO++;
		if(strstr(type, "DP")) ix = ixDP++;
		if(strstr(type, "AI")) ix = ixAI++;
		if(strstr(type, "AO")) ix = ixAO++;

		sprintf(cmd, "INSERT INTO dnp3tagIx (type,ix,tag) VALUES ('%s',%d,'%s')", type, ix, tag); 
		dbTag->set_sql(cmd);
		dbTag->execute_statement();
		
		// write them also to tag file
		if(tagFile){
			fprintf(tagFile, "%s,%d,%s,NO_NAME\n", type, ix, tag);
		}
	}
	if(tagFile)	fclose(tagFile);	
	else{	printd("Error: could not create Tag file\n");		exit(1);	}
	
	// Add point names to tag file
	tagFile = 	fopen(TAG_FILE, "r");
	FILE *tmpFile = fopen(TMP_FILE, "w");
	char line[500] = {0};
	
	if(tagFile && tmpFile){
		while(fgets(line, sizeof(line), tagFile)){
			char pointType[3] = {0};
			int pointIx = 0;
			char pointTag[50] = {0};
			char *pointName = NULL;
			//printf("line: %s\n", line);		
			if(sscanf(line, "%2s,%d,%s", pointType, &pointIx, pointTag) == 3){
				if(strchr(pointTag, ',')) *strchr(pointTag, ',') = 0;
				
				if(strstr(pointType, "DI") || strstr(pointType, "DO")){
					sprintf(cmd, "SELECT * FROM digpnt WHERE address='%s'", pointTag); 
				}
				if(strstr(pointType, "AI") || strstr(pointType, "AO") || strstr(pointType, "DP")){
					sprintf(cmd, "SELECT * FROM anapnt WHERE address='%s'", pointTag); 
				}
				//printf("cmd: %s\n", cmd);
				if(cmd){
					db->set_sql(cmd);
					db->execute_select();
					if((point = db->get_next_row())){
						pointName = point[PNT_NAME];
						//printf("pointName: %s\n", pointName);
						if(pointName && *pointName){
							fprintf(tmpFile, "%s,%d,%s,%s\n", pointType, pointIx, pointTag, pointName);
						}
						else{
							fprintf(tmpFile, "%s,%d,%s,%s\n", pointType, pointIx, pointTag, pointTag);
						}
					}
					else{
						fprintf(tmpFile, "%s,%d,%s,%s\n", pointType, pointIx, pointTag, pointTag);
					}
				}		
			}
			else{
				fprintf(tmpFile, "%s", line);
			}
		}
		fclose(tagFile);
		fclose(tmpFile);
		rename(TMP_FILE, TAG_FILE);	
	}
}

// ****************************************************************************
// Get dnp3 point index from point tag

int dnp3index(char *pointTag){
	static int ix = 0;
	char cmd[100];
	MYSQL_ROW dbRow;
	
	if(db){
		sprintf(cmd, "SELECT ix FROM dnp3tagIx WHERE tag = '%s'", pointTag);
		//printd("dnp3index:: cmd: %s\n", cmd);
		db->set_sql(cmd);
		db->execute_select();
		dbRow = db->get_next_row();
		if(dbRow){
			ix = atoi(dbRow[0]);
			//printd("dnp3index:: ix: %d tag: %s\n", ix, pointTag);
		}
		else{
			printd("dnp3index:: No index for tag: %s\n", pointTag);
		}
		return(ix);			
	}
	else{
		printd("dnp3index:: No database access\n");
		exit(1);
	}
}
// ****************************************************************************
// Get dnp3 point type from point tag

char *dnp3type(char *pointTag){
	static char *type = NULL;
	char cmd[100];
	MYSQL_ROW dbRow;
	
	if(db){
		sprintf(cmd, "SELECT type FROM dnp3tagIx WHERE tag = '%s'", pointTag);
		//printd("dnp3index:: cmd: %s\n", cmd);
		db->set_sql(cmd);
		db->execute_select();
		dbRow = db->get_next_row();
		if(dbRow){
			type =  dbRow[0];
			printd("dnp3type:: type: %s tag: %s\n", type, pointTag);
		}
		else{
			printd("dnp3type:: No index for tag: %s\n", pointTag);
		}
		return(type);	
	}
	else{
		printd("dnp3type:: No database access\n");
		exit(1);
	}
}
// ****************************************************************************
bool dnp3timeSync = true; /// Probably need some work here

struct opendnp3::DNPTime dnpTime(time_t eTime){
	static struct opendnp3::DNPTime DnpTime;

	DnpTime.value = eTime*1000;
	if(dnp3timeSync)	DnpTime.quality =	opendnp3::TimestampQuality::SYNCHRONIZED;
	else				DnpTime.quality =	opendnp3::TimestampQuality::INVALID;

	return(DnpTime);
}	

// ****************************************************************************
// Get dnp3 tag from point index and type

char *dnp3tag(char *pointType, int pointIndex){
	char cmd[100];
	MYSQL_ROW tagIx;

	if(db){
		sprintf(cmd, "SELECT tag FROM dnp3tagIx WHERE type = '%s' && ix = %d", pointType, pointIndex);
		db->set_sql(cmd);
		db->execute_select();
		if((tagIx = db->get_next_row())){
			return(tagIx[0]);
		}
	}
	return(NULL);
}

// ****************************************************************************
//	Find max entries in rt_status table of a type

int maxPoints(const char *pointType){
	MYSQL_ROW row;
	char cmd[100];
	
	if(db){
		sprintf(cmd, "SELECT count(*) FROM rt_status WHERE type = '%s'", pointType);
		db->set_sql(cmd);
		db->execute_select();
		row = db->get_next_row();
		if(row){
			//printf("%s maxPoints: %d\n", pointType, atoi(row[0]));
			return(atoi(row[0]));
		}
	}
	//printf("%s maxPoints: 0\n", pointType);
	return(0);
}

// ****************************************************************************
//	Queue event to dnp3 api

char type_di[] = "DI ";
char type_do[] = "DO ";
char type_ai[] = "AI EI ";
char type_ao[] = "AO EO ";
char type_dp[] = "DP ";	

void dnp3QueueEvent(int index, char *type, float val, time_t tstamp, bool offline){
	struct opendnp3::DNPTime dtime = dnpTime(tstamp);
	
	printd("dnp3QueEvent: index: %d type: %s val: %g tstamp: %ld offline: %d\n", index, type, val, tstamp, offline);

	if     (strstr(type_di, type))	queueDigitalEvent  (index, (bool)val, true, dtime, offline);
	else if(strstr(type_do, type))	queueDigitalControl(index, (bool)val, true, dtime, offline);
	else if(strstr(type_ai, type))	queueAnalogEvent   (index,       val, true, dtime, offline);
	else if(strstr(type_ao, type))	queueAnalogControl (index,       val, true, dtime, offline);
	else if(strstr(type_dp, type))	queueCounterEvent  (index,       val, true, dtime, offline);
	else printd("dnp3QueueEvent: Bad point type: %s on ix: %d!\n", type, index);
}

// ****************************************************************************
//	Initialize the client service

bool dnp3clientInit(void){
	char cmd[100];
    struct stat st;
	
	// Set up database event queue
	sprintf(cmd, "cat %s | mysql -udfs -pqball hypertacii", TRIG_FILE);
	system(cmd);
	
	// Make control point fifos
	if(stat(DCON_FIFO, &st))
	{	printd("Control point fifo does not exist! Will create it");
		mkdir(IO_FOLDER, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		umask(0);
		mkfifo(DCON_FIFO, 0666);
		
	}
	if(stat(ACON_FIFO, &st))
	{	printd("Control point fifo does not exist! Will create it");
		mkdir(IO_FOLDER, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		umask(0);
		mkfifo(ACON_FIFO, 0666);
		
	}	
	return(true);
}

// ****************************************************************************
//	Shut down the dnp3 service

void dnp3shutDown(void){

	if(db){
		db->set_sql(" \
			DROP TRIGGER IF EXISTS dnp3event; \
		");
		db->execute_statement();	
	}
}

// ****************************************************************************
//	Get dnp3 controls from api

struct Dnp3Point *Dnp3ControlReceived(void){
	static struct Dnp3Point control;
	static int digCon = 0, anaCon = 0;
	char line[100] = {0}, state[100] = {0};
	int index = 0;
	
	strcpy(control.pointTag, "");

	// Handle digital controls
	if(digCon <= 0){
		digCon = open(DCON_FIFO,  O_RDONLY | O_NDELAY);
	}
	if(digCon > 0){
		int len=read(digCon, line, sizeof(line));
		if(len > 0){
			//printd("Got digital control from dnp3: %s\n", line);

			strcpy(control.pointType, "DO");

			// parse it (ex: "{Type":"Control","Command":"Operate","Index":1,"Value":"Set On"})
			if(sscanf(line, "{\"Type\":\"Control\",\"Command\":\"Operate\",\"Index\":%d,\"Value\":\"Set %s", \
			 &control.pointIndex, state) == 2){
					
				if(strstr(state, "On"))	control.value = 1;
				else					control.value = 0;
				
				printd("Turn digital index: %d value: %g\n", control.pointIndex, control.value);//
				return(&control);
			}
		}
	}
	
	// Handle analog controls
	if(anaCon <= 0){
		anaCon = open(ACON_FIFO,  O_RDONLY | O_NDELAY);
	}
	if(anaCon > 0){
		int len=read(anaCon, line, sizeof(line));
		if(len > 0){
			strcpy(control.pointType, "AO");

			// parse it (ex: "{"Type":"SetpointInt32","Command":"Operate","Index":1,"Value":1433})
			if(sscanf(line, "{\"Type\":\"SetpointInt32\",\"Command\":\"Operate\",\"Index\":%d,\"Value\":%g}", \
			 &control.pointIndex, &control.value) == 2){
							
				printd("Set analog index: %d value: %g\n", control.pointIndex, control.value);//
				return(&control);
			}
		}
	}
	return(NULL);
}

// ****************************************************************************
// See if we need to send full status

bool dnp3sendStatus(void){
	static bool first = true;
	
	// Once we send full status to api at startup, 
	// it handles retransmission from there
	if(first){
		first = false;
		return(true);
	}
	return(false);
}
float scaleAnalog(char *tag, int raw){
	static class Db_wrapper *dbClient = new Db_wrapper();
	float lowEng=0, lowRaw=0, hiEng=0, hiRaw=0;
	float slope=0, offset=0, scaled=0;
	char cmd[100] = {0};
	MYSQL_ROW scale;
	
	sprintf(cmd, "SELECT lowEng,lowRaw,hiEng,hiRaw FROM anapnt where address='%s'", tag);
	dbClient->set_sql(cmd);
	dbClient->execute_select();
	scale = dbClient->get_next_row();
	lowEng=atoi(scale[0]); lowRaw=atoi(scale[1]); hiEng=atoi(scale[2]); hiRaw=atoi(scale[3]);
	
	slope = (hiEng - lowEng) / (hiRaw - lowRaw);
	offset = hiEng - (slope * hiRaw);
	scaled = slope * (float)raw + offset;
	if(scaled < 0) scaled = 0;
	
	//printd("raw: %d lowEng: %g lowRaw: %g hiEng: %g hiRaw: %g slope: %g offset: %g scaled: %g\n",
	//	raw,lowEng,lowRaw,hiEng,hiRaw,slope,offset,scaled);
	return(scaled);
}
//*****************************************************************************
//	Handle the dnp3 client interface

void dnp3clientService(){
	MYSQL_ROW event, status;
	unsigned int ev_ix = 0, ev_max = 0;	// Event index and max index
	char pointTag[100] = {0};			// Point tag as string (i.e. 12A1 for station 12 module A point 1)
	int pointIx = 0;					// Point index number assigned by dnp3
	char type[3] = {0};					// Point type
	float val = 0;						// Value of point from database
	time_t tstamp = 0; 					// Time point changed from database
	struct Dnp3Point *dnp3Point;
	char *rxControl = NULL;				// Received control
	static class Db_wrapper *dbClient = new Db_wrapper();
	bool offline = true;
	
	// See if there are any events in the ht4 database queue
	if(dbClient){
		dbClient->set_sql("SELECT * FROM dnp3events");
		dbClient->execute_select();
		do{
			event = dbClient->get_next_row();
			if(event){
				//printd("dnp3ClientService:: db_event: %s %s %s %s %s %s\n", event[0], event[1], event[2], event[3], event[4], event[5]);

				// Convert database strings to values for dnp3 event and put the event in the dnp3 queue
				ev_ix = atoi(event[EVENT_DB_IX]);
				if(ev_max < ev_ix) ev_max = ev_ix;
				strncpy(pointTag, event[EVENT_DB_TAG], sizeof(pointTag)-1);
				strncpy(type, event[EVENT_DB_TYP], sizeof(type)-1);
				val = atof(event[EVENT_DB_VAL]);
				tstamp = epoch(event[EVENT_DB_STMP]);
				offline = !atoi(event[EVENT_DB_REL]);
				pointIx = dnp3index(pointTag);
				
				// Don't send virtual or external
				//if((pointTag[0] != 'V') && (pointTag[0] != 'X')){	
					dnp3QueueEvent(pointIx, type, val, tstamp, offline);
				//{
			}
		} while(event);
		
		// Remove events from database queue
		if(ev_ix){
			char cmd[100];
			sprintf(cmd, "delete from dnp3events where ix <= %d;", ev_max);
			dbClient->set_sql(cmd);
			dbClient->execute_statement();
			dbClient->set_sql("flush tables dnp3events;");
			dbClient->execute_statement();

			// Handle db index maxing out
			if(ev_max >= IX_MAX){
				dbClient->set_sql("ALTER TABLE dnp3events AUTO_INCREMENT = 1");
				dbClient->execute_statement();			
			}
		}
		
		// Handle controls when received
		if((dnp3Point = Dnp3ControlReceived())){
			int stn=0, pnt=0;
			char mod=0;
			char cmd[500] = { 0 };

			char *tag = dnp3tag(dnp3Point->pointType, dnp3Point->pointIndex);
			if(tag){
				if(strstr(dnp3Point->pointType, "AO")){
					dnp3Point->value = scaleAnalog(tag, dnp3Point->value);
				}
				sprintf(cmd, "UPDATE rt_control \
					SET setvalue=%g, \
					controlled='y', \
					controlled_by='dnp3' \
				  WHERE address='%s'", \
					dnp3Point->value, tag);
				//printd("Dnp3control:: cmd: %s\n", cmd);
				//printd("Dnp3control:: setting control '%s' to %g cmd: '%s'\n", tag, dnp3Point->value, cmd);
				dbClient->set_sql(cmd);
				dbClient->execute_statement();
			}
			else{
				printd("No tag name for control index %d\n", dnp3Point->pointIndex);
			}
		}
		usleep(2000000);
		
		// If we need to send full status, do it
		if(dnp3sendStatus()){
			dbClient->set_sql("SELECT * FROM rt_status");
			dbClient->execute_select();
			
			while((status = dbClient->get_next_row())){
				strncpy(pointTag, status[RTSTATUS_ADR], sizeof(pointTag)-1);
				pointIx = dnp3index(pointTag);
				strncpy(type, status[RTSTATUS_TYP], sizeof(type)-1);
				val = atof(status[RTSTATUS_VAL]);
				offline = !atoi(status[EVENT_DB_REL]);
				tstamp = epoch(status[RTSTATUS_STMP]); 
				dnp3QueueEvent(pointIx, type, val, tstamp, offline);
				//printd("dnp3FullStatus: DFS, %d %s %s(%d) = %g @ %ld", ev_ix, type, pointTag, pointIx, val, tstamp);
			}
		}
	}
}
