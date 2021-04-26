#include "infolinker.hpp"


/**
 *  Static references
 */
UDPClient *InfoLink::status_conduit = NULL;
struct sockaddr_in InfoLink::status_conduit_port;
UDPClient *InfoLink::alarm_conduit = NULL;
struct sockaddr_in InfoLink::alarm_conduit_port;

std::queue<std::string> InfoLink::changed_alarms;
std::mutex InfoLink::changed_alarms_mutex;
std::queue<std::string> InfoLink::changed_status;
std::mutex InfoLink::changed_status_mutex;
std::vector<std::string> InfoLink::commands;
std::mutex InfoLink::command_mutex;
std::vector<std::string> InfoLink::station_comm;
std::mutex InfoLink::station_comm_mutex;

std::map<std::string,std::vector<std::string>> InfoLink::status;
std::map<std::string,std::vector<std::string>> InfoLink::alarms;

bool InfoLink::status_change;
bool InfoLink::alarm_change;
bool InfoLink::have_command;
bool InfoLink::have_station_comm;
bool InfoLink::log_analog_points;
bool InfoLink::log_digital_points;

int InfoLink::LAST_SOCKET;

DFS_Logger* InfoLink::static_logger;
std::list<std::string> watch_list;


/*
    Local helper function declarations
 */
bool invalidChar (char c);
int buildStation(int dnum, int station);
int moduleCharToInt(char module);
int driverTypeToInt(std::string t);

std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r "){
    str.erase(0, str.find_first_not_of(chars));
    return str;
}

std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r "){
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}

std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r "){
    return ltrim(rtrim(str, chars), chars);
}

/**
    Object - Class Functions ***************************************************
*/

/**
 * Infoserver API
 *
 *  int driver_number         driver_num as configured on HSM
 *  string driver_type        DFS, MOD, DFP, DNP, MES
 *  string path_to_addresses  path & filename of address file. One address per line
 *  string log_file_path      optional. empty string OR path to log debug to
 */
InfoLink::InfoLink(int driver_number, std::string driver_t, std::string path_to_addresses, std::string log_file_path){

  this->which_constructor = 1;
  // Note: if the file exists it it will be logged to. Otherwise NO logging!
  this->setLogFile(log_file_path);
  InfoLink::static_logger = this->dl;
  this->ltf("please truncate log file");
  this->ltf("constructor 1");
  this->setInitial();

  char dport[16];
  sprintf(dport,"unidriver%d",driver_number);
  this->name_of_port_on_hsm = std::string(dport);
  this->driver_num = driver_number;
  this->driver_type_string = driver_t;
  this->driver_type = driverTypeToInt(driver_t);

  if( !path_to_addresses.empty() )
    this->loadAddresses(path_to_addresses);


  /*
    station to driver link. Note: this is used with tcp_tool.py
    This will likely not be used since it will be up to the user of this
    class to handle comm to their device and then make use of this class'
    functions to talk to infoserver
  */
  this->makeTcpServer(this->tcp_connections,this->name_of_port_on_hsm);

  char ipcself[strlen(IPC_SELF) + 3];
  sprintf(ipcself,"%s%d",IPC_SELF,driver_num);
  this->info_client = new UnixSocketClient(ipcself,IPC_SERVER);
  if( this->info_client->isInitialized() == false){
    this->ltf("NO handle to send events to infoserver via UnixSocketClient");
  }

}




/**
 * Constructor to use when you want to watch one or more drivers. This may not
 * work as expected if there are lots of points on the drivers. Infoserver is
 * only able to take in a certain string length of addresses to watch and that
 * length has yet to be determined.
 */

InfoLink::InfoLink(std::vector<int> drivers_to_watch, std::string log_file_path){

  this->which_constructor = 2;
  // Note: if the file exists it it will be logged to. Otherwise NO logging!
  this->setLogFile(log_file_path);
  InfoLink::static_logger = this->dl;
  this->setInitial();

  this->ltf("please truncate log file");
  this->ltf("constructor 2");

  this->dbw = new Db_wrapper();
  int d;

  this->addresses.clear();
  if(drivers_to_watch.size() > 0){
    std::vector<int>::iterator it = drivers_to_watch.begin();
    while(it != drivers_to_watch.end() ){
      d = *it;
      this->loadDriverAddresses(d);
      it++;
    }
  }


  /*
    station to driver link. Note: this is used with tcp_tool.py
    This will likely not be used since it will be up to the user of this
    class to handle comm to their device and then make use of this class'
    functions to talk to infoserver. okay is a named port on hsm 52443
  */
  this->makeTcpServer(50,std::string("okay"));
}


/**
 * Given a path to a file with point addresses to watch (one address per line)
 * InfoLink will tell Infoserver to provide status for those addresses. The max
 * number of addresses that we can specify is not know. If you pass in the word
 * 'ALL' Infoserver will send all change data for the given driver number.
 */
InfoLink::InfoLink(std::string path_to_addresses, std::string log_file_path){

  this->which_constructor = 3;
  // Note: if the file exists it it will be logged to. Otherwise NO logging!
  this->setLogFile(log_file_path);
  InfoLink::static_logger = this->dl;
  this->setInitial();
  this->ltf("please truncate log file");
  this->ltf("constructor 3");


  if( !path_to_addresses.empty() ){
    if(path_to_addresses == "ALL"){
      this->addresses.push_back("ALL");
      this->dl->logGood("asking for ALL address status");
    }
    else
      this->loadAddresses(path_to_addresses);
  }


  /*
    station to driver link. Note: this is used with tcp_tool.py
    This will likely not be used since it will be up to the user of this
    class to handle comm to their device and then make use of this class'
    functions to talk to infoserver. okay is a named port on hsm 52443
  */
  // this->makeTcpServer(50,string("okay"));

}



InfoLink::InfoLink(int driver_number, std::string driver_t, std::string log_file_path){

  this->which_constructor = 4;
  this->setLogFile(log_file_path);
  InfoLink::static_logger = this->dl;
  this->setInitial();
  this->ltf("please truncate log file");
  this->ltf("constructor 4");

  char dport[16];
  sprintf(dport,"unidriver%d",driver_number);
  this->name_of_port_on_hsm = std::string(dport);
  this->driver_num = driver_number;
  this->driver_type_string = driver_t;
  this->driver_type = driverTypeToInt(driver_t);

  this->makeTcpServer(this->tcp_connections,this->name_of_port_on_hsm);

}




InfoLink::~InfoLink(){
  this->dl->logError("\nstarting destruction....\n");
  if(InfoLink::status_conduit != NULL)
    delete InfoLink::status_conduit;
  if(InfoLink::alarm_conduit != NULL)
    delete InfoLink::alarm_conduit;
  /*
  // TODO: calling delete on udp_server causes segfault
  if(this->udp_server != NULL){
    printf("calling delete\n");
    delete this->udp_server;
  }

  if(this->server != NULL)
    delete this->server;
  */
  this->dl->logError("finished!");
  delete this->dl;
}
//  End Object - Class Functions ===============================================






/**
    Optional Setter Functions **************************************************
*/

void InfoLink::setRemoteIP(std::string ip){
  this->udp_host_ip = ip;
}

void InfoLink::setRemotePort(std::string port){
  this->udp_host_port = port;
}
void InfoLink::setRemotePort(int port){
  char p[10];
  sprintf(p,"%d",port);
  this->udp_host_port = std::string(p);
}

void InfoLink::setAddresses(std::list<std::string> addrs){
  this->addresses = addrs;
}


// End Optional Setters ========================================================








/**
    Loading Config Functions ***************************************************
*/

void InfoLink::loadAddresses(std::string a){
  if(a.empty()) return;

  std::ifstream f(a.c_str());
  std::string line;
  this->ltf("\nloading addresses from file");
  while (std::getline(f, line))
  {
    if( !line.empty() ){
      this->addresses.push_back(line);
      this->ltf(line);
    }
  }
  this->ltf("\n");
}


bool InfoLink::loadUASP(std::string path){
  if(path.empty()) return false;

  std::ifstream f(path.c_str());
  std::string line;
  std::string tmp;
  size_t spot = 0;
  this->ltf("\nloading UASP addresses from file");
  while (std::getline(f, line))
  {
    if( !line.empty() && line != "\n" && line.at(0) != ' ' && line.at(0) != '\t' && line.at(0) != '#' ){
      spot = line.find(":");
      tmp = line.substr(spot+1);
      trim(tmp);


      this->addresses.push_back(tmp);
      this->ltf(tmp);
    }
  }
  this->ltf("\n");
  return true;
}


void InfoLink::setInitial(){
  InfoLink::changed_alarms_mutex.unlock();
  InfoLink::changed_status_mutex.unlock();
  InfoLink::command_mutex.unlock();
  InfoLink::alarm_change = false;
  InfoLink::status_change = false;
  InfoLink::have_command = false;
  InfoLink::have_station_comm = false;
  InfoLink::LAST_SOCKET = -1;
  InfoLink::log_analog_points = false;
  InfoLink::log_digital_points = false;
}



bool InfoLink::loadDriverAddresses(int driver){
  char lmsg[256];
  sprintf(lmsg,"\nloading addresses from tables for driver %d",driver);
  this->ltf(lmsg);
  MYSQL_ROW row;
  char q[256];
  string addr;

  sprintf(q,"select address from anapnt where dnum=%d",driver);

  string look = string(q);
  string sqll = this->dbw->set_sql(look);

  this->dbw->execute_select();
  row = this->dbw->get_next_row();

  while( row ) {
      try {
          addr = string(row[0]);

          if(!addr.empty()){
            // this->ltf("pushing "+addr);
            this->addresses.push_back(addr);
          }
      }catch(int err) {
          this->dbw->free_result();
          this->ltf("try/catch fail loadDriverAddresses()");
          return false;
      }
      row = this->dbw->get_next_row();
  }
  this->dbw->free_result();




  sprintf(q,"select address from digpnt where dnum=%d",driver);

  look = string(q);
  sqll = this->dbw->set_sql(look);

  this->dbw->execute_select();
  row = this->dbw->get_next_row();

  while( row ) {
      try {
          addr = string(row[0]);

          if(!addr.empty()){
            // this->ltf("pushing "+addr);
            this->addresses.push_back(addr);
          }
      }catch(int err) {
          this->dbw->free_result();
          this->ltf("try/catch fail loadDriverAddresses()");
          return false;
      }
      row = this->dbw->get_next_row();
  }
  this->dbw->free_result();

  return true;
}

//  End Loading Config Functions ===============================================







/**
    Helper Functions ***************************************************
*/

/*
  station to driver link
  This will likely not be used since it will be up to the user of this
  class to handle comm to their device and then make use of this class'
  functions to talk to infoserver

*/
void InfoLink::makeTcpServer(int connections, std::string port) {

    this->server = new TcpServer(connections);
    this->server->setTimeout(0,0);

    // const char port[9] = "dfsmesh\0";
    // if( !server->bindPort(MESH_PORT) ){
    if( !this->server->bindPort(port.c_str()) ){
        this->dl->logError("Failed to TCP station server");
    }else{
      this->server->setTimeout(0,0);
      this->tcp_last_comm = time(NULL);
      port.append(" TCP server ready");
      this->dl->logGood(port);
    }

}

/*
  split delimited string into a vector
*/
std::vector<std::string> InfoLink::split(std::string split_me, char using_this)
{
    std::stringstream ss(split_me);
    std::string item;
    std::vector<std::string> results;
    while (std::getline(ss, item, using_this))
    {
       results.push_back(item);
    }
    return results;
}

/**
  sanitize message to only have printable ascii
*/
bool invalidChar (char c){
    return !(c>=32 && c <127);
}

/**
 * Parse separate driver number and station number into a full station number
 * @param  dnum    driver number
 * @param  station station number
 * @return         full station number as int
 */
int buildStation(int dnum, int station){
  return ( (dnum*1000) + station );
}

int moduleCharToInt(char module){
  if (module >= MINMODC && module <= MAXMODC)
    return module - 0x40;   /* A=>1 ... O->15 */
  else if (module == RIMMODC)
    return RIMMOD;
  else if (module == PLCMODC)
    return PLCMOD;
  return -1;
}

int driverTypeToInt(std::string t){
  if(t == "DFS") return 1;
  else if(t == "MOD") return 6;
  else if(t == "DFP") return 7;
  else if(t == "DNP") return 2;
  else if(t == "MES") return 8;
  return 1; //default to dfs type
}



/**
  Turn our stored message as vector back into a string
*/
std::string InfoLink::messageVectorToString(std::vector<std::string> vec){

  if (vec.empty()){
    return std::string("no data");
  }
  if(vec.size() == 1)
    return vec[0];

  std::string r;

  std::vector<std::string>::iterator it = vec.begin();
  while(it != vec.end()-1){
    r.append(*it+",");
    it++;
  }
  r.append(vec.back());
  return r;
}

//  End Helper Functions ===============================================









/**
    Getters Setters Functions *******************************************
*/



int InfoLink::getDriverNumber() {
  return this->driver_num;
}

int InfoLink::getDriverType() {
  return this->driver_type;
}

std::list<std::string> InfoLink::getWatchedAddresses(){
  return this->addresses;
}

//  End Getters Setters Functions =======================================









/**
    Flag Functions ***************************************************
*/
bool InfoLink::statusChange(){
  return InfoLink::status_change == true;
}
void InfoLink::gotStatusChange(){
  InfoLink::status_change = true;
}
void InfoLink::resetStatusChange(){
  InfoLink::status_change = false;
}

bool InfoLink::alarmChange(){
  return InfoLink::alarm_change == true;
}
void InfoLink::gotAlarmChange(){
  InfoLink::alarm_change = true;
}
void InfoLink::resetAlarmChange(){
  InfoLink::alarm_change = false;
}

bool InfoLink::haveCommand(){
  return InfoLink::have_command == true;
}
void InfoLink::gotCommand(){
  InfoLink::have_command = true;
}
void InfoLink::resetCommand(){
  InfoLink::have_command = false;
}

bool InfoLink::haveStationComm(){
  return InfoLink::have_station_comm == true;
}
void InfoLink::gotStationComm(){
  InfoLink::have_station_comm = true;
}
void InfoLink::resetStationComm(){
  InfoLink::have_station_comm = false;
}
//  End Flag Functions ===============================================









/**
    Data Functions ***************************************************
*/
  std::queue<std::string> InfoLink::getAlarmChanges(){
    // InfoLink::static_logger->logWarning("need alarms mutex getAlarmChanges()");
    InfoLink::changed_alarms_mutex.lock();
    // InfoLink::static_logger->logGood("got alarms mutex getAlarmChanges()");
    std::queue<std::string> r;

    if(InfoLink::changed_alarms.size() > 0){
      // make copy for user
      std::queue<std::string> q(InfoLink::changed_alarms);
      r = q;
      // clear our container
      std::queue<std::string>().swap(InfoLink::changed_alarms);
      InfoLink::resetAlarmChange();
    }
    // InfoLink::static_logger->logMe("release alarms mutex getAlarmChanges()");
    InfoLink::changed_alarms_mutex.unlock();
    return r;
  }

  void InfoLink::setAlarm(std::string alarm){
    // InfoLink::static_logger->logWarning("need alarms mutex setAlarm()");
    InfoLink::changed_alarms_mutex.lock();
    // InfoLink::static_logger->logGood("got alarms mutex setAlarm()");
    InfoLink::changed_alarms.push(alarm);
    InfoLink::gotAlarmChange();
    // InfoLink::static_logger->logMe("release alarms mutex setAlarm()");
    InfoLink::changed_alarms_mutex.unlock();
  }



  std::queue<std::string> InfoLink::getStatusChanges(){
    // InfoLink::static_logger->logWarning("need status mutex getStatusChanges()");
    InfoLink::changed_status_mutex.lock();
    // InfoLink::static_logger->logGood("got status mutex getStatusChanges()");
    std::queue<std::string> r;

    if(InfoLink::changed_status.size() > 0){
      // make copy for user
      std::queue<std::string> q(InfoLink::changed_status);
      r = q;
      // clear our container
      std::queue<std::string>().swap(InfoLink::changed_status);
      InfoLink::resetStatusChange();
    }
    // InfoLink::static_logger->logMe("release alarms mutex getStatusChanges()");
    InfoLink::changed_status_mutex.unlock();
    return r;
  }

  void InfoLink::setStatus(std::string status){
    // char lmsg[256];
    // InfoLink::static_logger->logWarning("need status mutext setStatus()");
    InfoLink::changed_status_mutex.lock();
    // InfoLink::static_logger->logGood("got status mutext setStatus()");
    InfoLink::changed_status.push(status);
    // sprintf(lmsg,"++++++++++++++++++++++++++ changed status size:%d from %s",InfoLink::changed_status.size(),status.c_str() );
    // InfoLink::static_logger->logError(lmsg);
    // InfoLink::static_logger->logError(string("setting status:"+status));
    InfoLink::gotStatusChange();
    // InfoLink::static_logger->logMe("release status mutext setStatus()");
    InfoLink::changed_status_mutex.unlock();
  }



  std::vector<std::string> InfoLink::getCommands(){
    // InfoLink::static_logger->logWarning("need command mutext getCommands()");
    InfoLink::command_mutex.lock();
    // InfoLink::static_logger->logGood("got command mutext getCommands()");
    std::vector<std::string> r;

    if(InfoLink::commands.size() > 0){
      // make copy for user
      std::vector<std::string> q(InfoLink::commands);
      r = q;
      // clear vector now that we are giving a copy to the user to act on
      InfoLink::commands.clear();
      InfoLink::resetCommand();
    }
    // InfoLink::static_logger->logMe("release command mutext getCommands()");
    InfoLink::command_mutex.unlock();


    return r;
  }

  void InfoLink::setCommand(std::string cmd){
    // InfoLink::static_logger->logWarning("need command mutex setCommand()");
    InfoLink::command_mutex.lock();
    // InfoLink::static_logger->logGood("got command mutex setCommand()");
    InfoLink::commands.push_back(cmd);
    InfoLink::gotCommand();
    // InfoLink::static_logger->logMe("release command mutex setCommand()");
    InfoLink::command_mutex.unlock();
  }





  std::vector<std::string> InfoLink::getStationComm(){
    // InfoLink::static_logger->logWarning("need station comm mutext getStationComm()");
    InfoLink::station_comm_mutex.lock();
    // InfoLink::static_logger->logGood("got station comm mutext getStationComm()");
    std::vector<std::string> r;

    if(InfoLink::station_comm.size() > 0){
      // make copy for user
      std::vector<std::string> q(InfoLink::station_comm);
      r = q;
      // clear vector now that we are giving a copy to the user to act on
      InfoLink::station_comm.clear();
      InfoLink::resetStationComm();
    }
    // InfoLink::static_logger->logMe("release station comm mutext getStationComm()");
    InfoLink::station_comm_mutex.unlock();


    return r;
  }

  void InfoLink::setStationComm(std::string comm){
    // InfoLink::static_logger->logWarning("need station comm mutex setStationComm()");
    InfoLink::station_comm_mutex.lock();
    // InfoLink::static_logger->logGood("got station comm mutex setStationComm()");
    InfoLink::station_comm.push_back(comm);
    InfoLink::gotStationComm();
    // InfoLink::static_logger->logMe("release station comm mutex setStationComm()");
    InfoLink::station_comm_mutex.unlock();
  }





//  End Data Functions ===============================================









/**
    Status Request Functions ***************************************************
*/
  std::vector<std::string> InfoLink::getAlarm(std::string address){
    std::vector<std::string> s;
    std::map<std::string,std::vector<std::string>>::iterator it;
    it = this->alarms.find(address);
    if(it != this->alarms.end())
      s = it->second;

    return s;
  }
  std::vector<std::string> InfoLink::getStatus(std::string address){
    std::vector<std::string> s;
    std::map<std::string,std::vector<std::string>>::iterator it;
    it = this->status.find(address);
    if(it != this->status.end())
      s = it->second;

    return s;
  }


//  End Status Request Functions ===============================================


































/**
    Infoserver IPC Functions ***************************************************
*/



  void InfoLink::initAlarmSocket(){

    InfoLink::alarm_conduit = new UDPClient();
    int s = -1;
    s = InfoLink::alarm_conduit->getSocket();
    if(s < 0){
      InfoLink::static_logger->logError("NO handle to alarm socket!");
    }
    InfoLink::alarm_conduit->makeRecipient(&InfoLink::alarm_conduit_port,"localhost","dfsinfo");
    InfoLink::alarm_conduit->setNonBlocking(1);

  }

  void InfoLink::initStatusSocket(){
    if(InfoLink::status_conduit != NULL){
      delete InfoLink::status_conduit;
    }
    InfoLink::status_conduit = new UDPClient();
    int s = -1;
    s = InfoLink::status_conduit->getSocket();
    if(s < 0){
      InfoLink::static_logger->logError("NO handle to status socket!");
    }
    InfoLink::status_conduit->makeRecipient(&InfoLink::status_conduit_port,"localhost","dfsinfo");
    InfoLink::status_conduit->setNonBlocking(1);
  }

  void InfoLink::initStatusSocket(std::string ip, std::string port){
    if(InfoLink::status_conduit != NULL){
      delete InfoLink::status_conduit;
    }
    InfoLink::status_conduit = new UDPClient();
    int s = -1;
    s = InfoLink::status_conduit->getSocket();
    if(s < 0){
      InfoLink::static_logger->logError("NO handle to status socket!");
    }
    InfoLink::status_conduit->makeRecipient(&InfoLink::status_conduit_port,ip.c_str(),port.c_str());
    InfoLink::status_conduit->setNonBlocking(1);
  }



/**
  Kick off thread to watch for status changes
*/
void InfoLink::startStatusListenerThreadFromInfoserver( void (*func_ptr)() ){
  this->dl->logGood("starting STATUS listener thread");
  this->func_ptr_status = (*func_ptr);
  sleep(2);
  pthread_create(&info_control_thread,NULL,&InfoLink::listenToInfoserverStatus, this);
}


/**
  Any change in status of the points contained in our config file is sent from
  infoserver to this function
*/
void* InfoLink::listenToInfoserverStatus (void *s) {
  int BUFF_SIZE = 33000;
  char lmsg[BUFF_SIZE];
  InfoLink* local_il = static_cast<InfoLink*>(s);
  char* spot;
  std::string worker;
  std::vector<std::string> items;
  std::vector<std::string> points;
  std::string addr,recv_buffer,temp_buffer;
  // bool ack_only = false;


  BYTE buf[BUFF_SIZE];
  memset(buf,0,sizeof(buf));
  // int msg_type;

  // Buffer *packet;
  // packet = new Buffer(BUFF_SIZE);

  InfoLink::initStatusSocket(local_il->udp_host_ip,local_il->udp_host_port);

  // const char* reg = "5 13 -1 6150.40001 6150.1 6150.30001";

  std::string action = "5 13 -1";
  std::list<std::string>::const_iterator it;
  it = local_il->addresses.begin();
  while( it != local_il->addresses.end() ){
    action.append(" "+(*it));
    it++;
  }
  local_il->ltf("built list:"+action);

  const char* reg = (char*)action.c_str();
  bool am_printing = false;
  std::map<std::string,std::vector<std::string>>::iterator payload;

  if(InfoLink::status_conduit){
    InfoLink::status_conduit->sendMessage(reg,&InfoLink::status_conduit_port);

    local_il->ltf("Entering status loop");

    while(1){
      am_printing = false;
      memset(buf,0,sizeof(buf));
      if(InfoLink::status_conduit->readMessage((BYTE *)buf,sizeof(buf)-1) < 1) {
        usleep(100000);
        continue;
      }

      temp_buffer = std::string((const char*)buf);
      if(temp_buffer.find("ACK") == 0)
        continue;

      if(local_il->shouldPrint(std::string((const char *)buf))){
        sprintf(lmsg,"\r------status udp MSG:%s", (const char *)buf);
        am_printing = true;
        local_il->dl->logGood(lmsg);
      }

      InfoLink::setStatus( std::string((const char*)buf) );

      // if user passed in a callback, call it!
      if(local_il->func_ptr_status != NULL){
        (*local_il->func_ptr_status)();
        continue;// user will handle the message with the callback
      }


      // multi part message
      if(temp_buffer.find("MORE+") == 0){
        if(recv_buffer.empty())
          recv_buffer = std::string((const char*)buf);
        else
          recv_buffer.append(std::string((const char*)buf));
        // wait for next message
        continue;
      }

      if(temp_buffer.find(":STATUS") == 0){
        spot = (char*)buf+8;
        worker = std::string(spot);
        // local_il->dl->logGood(worker);

        if( worker.find(",") == std::string::npos )
          continue;

        items = local_il->split(worker,',');

        if(am_printing){
          std::vector<std::string>::iterator vt;
          vt = items.begin();
          local_il->dl->logError("items");
          while(vt != items.end() ){
            local_il->dl->logMe(*vt);
            vt++;
          }
        }


        // if( items[0] == "STATUS" ){
          addr = items[0];
        // }
        // else if( items[0] == "ACK" && items.size() > 3 ){
        //   addr = items[2];
        //   ack_only = true;
        // }

          payload = local_il->status.find(addr);
          if( payload == local_il->status.end() ){
            local_il->status.insert(std::make_pair(addr,items));
          }
          else{
            // if( items.size() > 1 ){
            //   if( items[2] == string("OFFLINE_EVENT") ){
            //     if( local_il->status[addr].size() > 0 ){
            //       sprintf(lmsg,"--------------- we have the power");
            //       std::vector<std::string>::iterator vt;
            //       vt = local_il->status[addr].begin();

            //       local_il->dl->logMe("inner:"+*vt);
            //       vt++;

            //     }
            //   }

            //   local_il->dl->logMe(lmsg);
            // }
            // else // normal usage
              local_il->status[addr] = items;
          }


/*
        std::map<std::string,std::vector<std::string>>::iterator it = status.begin();
        while(it != status.end()){

          this->ltf(it->first);
          std::vector<std::string>::iterator vi = it->second.begin();
          while(vi != it->second.end()){
            this->ltf("\t"+ (*vi));
            vi++;
          }
          it++;
        }
*/
        continue;
      }

      if(temp_buffer.find(":CONTROL") == 0){
        // local_il->dl->logMe(string("have control:")+temp_buffer);
        continue;
      }

      if(temp_buffer.find(":ALARM") == 0){
        // local_il->dl->logMe(string("have alarm:")+temp_buffer);
        continue;
      }

      // reaching here, we probably have other parts of MORE+ message
      if(!recv_buffer.empty()){
        recv_buffer.append(temp_buffer);
      }else{
        recv_buffer = temp_buffer;
      }

      // process colon separated messages
      if(recv_buffer.find("MORE+") == 0){
        recv_buffer = recv_buffer.substr(5);
      }

      local_il->dl->logMe(std::string("full message:")+recv_buffer);

      worker = recv_buffer;
      if( worker.find(",") == std::string::npos )
        continue;

      points.clear();
      points = local_il->split(worker,':');
      std::vector<std::string>::iterator it = points.begin();
      while(it != points.end()){
        local_il->dl->logMe(*it);

        items = local_il->split(*it,',');
        addr = items[0];

        if( local_il->status.find(addr) == local_il->status.end() ){
          local_il->status.insert(std::make_pair(addr,items));
        }else{
          local_il->status[addr] = items;
        }

        it++;
      }

      recv_buffer.clear();
      temp_buffer.clear();

    }

  }
  // delete packet;
  local_il->dl->logError("leaving listenToInfoserverStatus() thread");
  return (void*)NULL;
}



































/**
  Kick off thread to watch for alarm state changes
*/

void InfoLink::startAlarmListenerThreadFromInfoserver( void (*func_ptr)() ){
  this->dl->logGood("starting Alarm listener thread");
  sleep(2);//seems to work better if this is paused
  this->func_ptr_alarm = (*func_ptr);
  pthread_create(&info_control_thread,NULL,&InfoLink::listenToInfoserverAlarm, this);
}


/**
  Any change in alarm state is sent from infoserver to this function
*/
void* InfoLink::listenToInfoserverAlarm (void *s) {
  int BUFF_SIZE = 33000;
  char lmsg[BUFF_SIZE];
  InfoLink* local_il = static_cast<InfoLink*>(s);
  char* spot;
  std::string worker;
  std::vector<std::string> items;
  std::string addr;
  bool ack_only = false;


  BYTE buf[BUFF_SIZE];
  memset(buf,0,sizeof(buf));
  // int msg_type;

  // Buffer *packet;
  // packet = new Buffer(BUFF_SIZE);


  InfoLink::initAlarmSocket();
  const char* reg = "5 12 -1";


  if(InfoLink::alarm_conduit){
    InfoLink::alarm_conduit->sendMessage(reg,&InfoLink::alarm_conduit_port);

    local_il->ltf("Entering alarm loop");

    while(1){
      memset(buf,0,sizeof(buf));
      if(InfoLink::alarm_conduit->readMessage((BYTE *)buf,sizeof(buf)-1) < 1) {
        usleep(100000);
        continue;
      }
      if(local_il->shouldPrint(std::string((const char *)buf))){
        sprintf(lmsg,"\r++++++ alarm udp MSG: %s", (const char *)buf);
        local_il->dl->logError(lmsg);
      }

      InfoLink::setAlarm(std::string((const char*)buf));

      // if user passed in a callback, call it!
      if(local_il->func_ptr_alarm != NULL){
        (*local_il->func_ptr_alarm)();
        continue;// if user is processing messages via their function, continue
      }

      spot = (char*)buf+7;
      worker = std::string(spot);
      // local_il->dl->logError(worker);

      if( worker.find(",") == std::string::npos )
        continue;

      items = local_il->split(worker,',');

      if( items[0] == "NEW" || items[0] == "CLEAR" ){
        addr = items[4];
      }else if( items[0] == "ACK" && items.size() > 3 ){
        addr = items[2];
        ack_only = true;
      }

      if(!ack_only){
        if( local_il->alarms.find(addr) == local_il->alarms.end() ){
          local_il->alarms.insert(std::make_pair(addr,items));
        }else{
          local_il->alarms[addr] = items;
        }
      }else if(ack_only && !addr.empty()){// check if this ack is for a cleared alarm
          std::map<std::string,std::vector<std::string>>::iterator it;
          it = local_il->alarms.find(addr);
          if(it != local_il->alarms.end() && it->second[0] == "CLEAR" ){
            // local_il->dl->logGood("removing cleard alarm");
            local_il->alarms.erase(it);
          }else{// user ack'd an active alarm so update what we got
            local_il->dl->logGood("update alarm state");
            local_il->alarms[addr] = items;
          }
      }

      ack_only = false;

    }

  }
  // delete packet;
  local_il->dl->logError("leaving listenToInfoserverAlarm() thread");
  return (void*)NULL;
}



























/**
  Kick off thread to listen for commands from infoserver
*/

void InfoLink::startListenerThreadFromInfoserver( void (*func_ptr)() ){
  this->dl->logGood("starting listener thread for Infoserver commands");
  this->func_ptr_command = (*func_ptr);
  pthread_create(&info_control_thread,NULL,&InfoLink::listenToInfoserver, this);
}


/**
  Infoserver can send commands to its Drivers. Any
*/
void* InfoLink::listenToInfoserver (void *s) {
  char lmsg[1024];
  InfoLink* local_il = static_cast<InfoLink*>(s);
  std::string worker;

  int d = local_il->getDriverNumber();
  if(d < 0){
    local_il->dl->logError("will not listen for commands from infoserver");
    return (void*)NULL;
  }
  sprintf(lmsg,"inside listenToInfoserver() on driver %d",d);
  local_il->dl->logMe(lmsg);


  BYTE buf[1024];
  memset(buf,0,sizeof(buf));
  int msg_type;


  UDPServer *udp_server = NULL;
  char netservice[12];
  sprintf(netservice,"driver%d",local_il->getDriverNumber());

  sprintf(lmsg,"binding to %s",netservice);
  local_il->dl->logMe(lmsg);


  udp_server = new UDPServer();
  udp_server->bindPort(netservice);
  if(udp_server->isValidSocket() == false){
    local_il->ltf("ERROR: do not have udp server socket!");
    InfoLink::static_logger->logError("ERROR: do not have udp server socket!");
  }

  while(udp_server) {// blocks at readMessage()
      if (udp_server->readMessage(buf,1024)) {
        sprintf(lmsg,">>>>> command udp MSG: %s", (const char *)buf);
        local_il->ltf(lmsg);

        // Note: this function has a mutex
        InfoLink::setCommand(std::string((const char *)buf));

        // if user passed in a callback, call it!
        if(local_il->func_ptr_command != NULL){
          (*local_il->func_ptr_command)();
          continue;// if user is handling commands, let them do it
        }




        size_t spot = 0;

        // we only get here if there is no function pointer
        msg_type = atoi((const char*)buf);
        switch(msg_type){
          case DRIVER_SHUTDOWN:
            local_il->dl->logMe("got DRIVER_SHUTDOWN");
          break;

          case DRIVER_CONFIGCHANGE:
            local_il->dl->logMe("got DRIVER_CONFIGCHANGE");
          break;

          case DRIVER_CONTROL:
            local_il->dl->logMe("got DRIVER_CONTROL");

            worker = std::string((const char *)buf);
            spot = worker.find_first_of(" ");
            if(spot != std::string::npos){
              local_il->dl->logMe("calling setCommand("+worker+")");
              InfoLink::setCommand(worker.substr(spot+1));
            }

          break;

          default:
          sprintf(lmsg,">>>>>>>>>>>>>> unknown command");
          local_il->ltf(lmsg);
            continue;
        }



        memset(buf,0,sizeof(buf));
      }


  }
  // delete packet;
  local_il->dl->logMe("leaving listenToInfoserver() thread");
  return (void*)NULL;//brad
}




















void InfoLink::sendToInfoserver() {
  //
  this->info_client->sendMessage((BYTE*)this->infobuf, strlen(this->infobuf));
}

bool InfoLink::sendToInfoserver(char* message){
  if(message == NULL) return false;
  int len = strlen(message);

  char lmsg[1024];
  // sprintf(lmsg,"would send this:%s",message);
  this->ltf(lmsg);
  // if(this->info_client != NULL)
    return this->info_client->sendMessage((BYTE*)message, len) == 0;
  // return false;
}
bool InfoLink::sendToInfoserver(std::string message){
  return this->sendToInfoserver(message.c_str());
}







//  End Infoserver IPC Functions ===============================================













/**
    Message Functions ***************************************************
*/


//DFS stations

bool InfoLink::setAnalogPointRaw(int driver_number, std::string driver_type, int station_number, int point_number, char module_char, int value, int time_tag){
  char lmsg[1024];
  int stn;
  stn = buildStation(driver_number,station_number);
  char msg[256];
  int mchar;
  mchar = moduleCharToInt(module_char);
  std::string addr;
  sprintf(lmsg,"%d%c%d",stn,module_char,point_number);
  addr = std::string(lmsg);

  memset(msg,0,sizeof(msg));
  if(mchar == 16)
    sprintf(msg,"1 %d %d %d %d R %d %d %d",driverTypeToInt(driver_type), driver_num, SET_ANALOG_POINT,stn,point_number,value,time_tag);
  else if(mchar == 17)
    sprintf(msg,"1 %d %d %d %d Q %d %d %d",driverTypeToInt(driver_type), driver_num, SET_ANALOG_POINT,stn,point_number,value,time_tag);
  else
    sprintf(msg,"1 %d %d %d %d %d %d %d %d",driverTypeToInt(driver_type), driver_num, SET_ANALOG_POINT,stn,mchar,point_number,value,time_tag);
  // this->sendToInfoserver(msg);
  if(InfoLink::status_conduit){
    if(this->shouldPrint(addr) || InfoLink::log_analog_points){
      sprintf(lmsg,"setAnalogPointRaw() %s:%s",addr.c_str(),msg);
      this->ltf(lmsg);
    }
    return InfoLink::status_conduit->sendMessage((const char*)msg,&InfoLink::status_conduit_port);
  }
  return false;
}

bool InfoLink::setAnalogPointRaw(int driver_number, std::string driver_type, int station_number, int point_number, char module_char, int raw_value){
  return this->setAnalogPointRaw(driver_number,driver_type,station_number,point_number,module_char,raw_value,0);
}


bool InfoLink::setAnalogPointRaw(int station_number, int point_number, char module_char, int value){
  return this->setAnalogPointRaw(this->driver_num,this->driver_type_string, station_number,point_number,module_char,value,0);
}

bool InfoLink::setAnalogPointRaw(int station_number, int point_number, char module_char, int value, int time_tag){
  return this->setAnalogPointRaw(this->driver_num,this->driver_type_string, station_number,point_number,module_char,value,time_tag);
}





bool InfoLink::setAnalogQPoint(int driver_number, std::string driver_type, int station_number, int point_number, char module_char, float value, int time_tag){
  char lmsg[1024];
  int stn;
  stn = buildStation(driver_number,station_number);
  char msg[256];
  // int mchar;
  // mchar = moduleCharToInt(module_char);
  std::string addr;
  sprintf(lmsg,"%d%c%d",stn,module_char,point_number);
  addr = std::string(lmsg);

  memset(msg,0,sizeof(msg));

  sprintf(msg,"1 %d %d %d %d Q %d %f %d",driverTypeToInt(driver_type), driver_num, SET_ANALOG_POINT,stn,point_number,value,time_tag);

  if(InfoLink::status_conduit){
    if(this->shouldPrint(addr) || InfoLink::log_analog_points){
      sprintf(lmsg,"setAnalogQPoint() %s:%s",addr.c_str(),msg);
      this->ltf(lmsg);
    }
    return InfoLink::status_conduit->sendMessage((const char*)msg,&InfoLink::status_conduit_port);
  }
  return false;
}

bool InfoLink::setAnalogQPoint(int driver_number, std::string driver_type, int station_number, int point_number, char module_char, float value){
  return this->setAnalogQPoint(driver_number,driver_type,station_number,point_number,module_char,value,0);
}


bool InfoLink::setAnalogQPoint(int station_number, int point_number, char module_char, float value){
  return this->setAnalogQPoint(this->driver_num,this->driver_type_string, station_number,point_number,module_char,value,0);
}

bool InfoLink::setAnalogQPoint(int station_number, int point_number, char module_char, float value, int time_tag){
  return this->setAnalogQPoint(this->driver_num,this->driver_type_string, station_number,point_number,module_char,value,time_tag);
}




























bool InfoLink::setDigitalPulsePoint(int driver_number, std::string driver_type, int station_number, int point_number, char module_char, int pulses, int time_tag){
  char lmsg[312];
  int stn;
  stn = buildStation(driver_number,station_number);
  char msg[256];
  int mchar;
  mchar = moduleCharToInt(module_char);
  std::string addr;
  sprintf(lmsg,"%d%c%d",stn,module_char,point_number);
  addr = std::string(lmsg);

  memset(msg,0,sizeof(msg));

  if(mchar == 16)// R
    sprintf(msg,"1 %d %d %d %d R %d %d %d",driverTypeToInt(driver_type), driver_number, SET_DP_POINT,stn,point_number,pulses,time_tag);
  else if(mchar == 17)// Q
    sprintf(msg,"1 %d %d %d %d Q %d %d %d",driverTypeToInt(driver_type), driver_number, SET_DP_POINT,stn,point_number,pulses,time_tag);
  else
    sprintf(msg,"1 %d %d %d %d %d %d %d %d",driverTypeToInt(driver_type), driver_number, SET_DP_POINT,stn,mchar,point_number,pulses,time_tag);


  // this->sendToInfoserver(msg);
  if(InfoLink::status_conduit){
    if(this->shouldPrint(addr) || InfoLink::log_digital_points){
      sprintf(lmsg,"setDigitalPulsePoint() %s:%s",addr.c_str(),msg);
      this->ltf(lmsg);
    }
    return InfoLink::status_conduit->sendMessage((const char*)msg,&InfoLink::status_conduit_port);
  }
  return false;
}

bool InfoLink::setDigitalPulsePoint(int driver_number, std::string driver_type, int station_number, int point_number, char module_char, int pulses){
  return this->setDigitalPulsePoint(driver_number,driver_type,station_number,point_number,module_char,pulses,0);
}

bool InfoLink::setDigitalPulsePoint(int snum, int pnum, char mchar, int pulses, int time_tag){
  return this->setDigitalPulsePoint(this->driver_num,this->driver_type_string,snum,pnum,mchar,pulses,time_tag);
}

bool InfoLink::setDigitalPulsePoint(int snum, int pnum, char mchar, int pulses){
  return this->setDigitalPulsePoint(this->driver_num,this->driver_type_string,snum,pnum,mchar,pulses,0);
}

/**
 * Set the value for a digital point
 * @param  driver_num  [description]
 * @param  driver_type [description]
 * @param  snum        [description]
 * @param  pnum        [description]
 * @param  module_char [description]
 * @param  val         [description]
 * @param  tt          seconds in the past. This is an offset into the past
 * @return             [description]
 */
bool InfoLink::setDigitalPoint(int driver_num, std::string driver_type, int snum, int pnum, char module_char, int val, int tt){
  char lmsg[312];
  int stn;
  stn = buildStation(driver_num,snum);
  char msg[256];
  int mchar;
  mchar = moduleCharToInt(module_char);
  std::string addr;
  sprintf(lmsg,"%d%c%d",stn,module_char,pnum);
  addr = std::string(lmsg);

  memset(msg,0,sizeof(msg));

  if(mchar == 16)// R
    sprintf(msg,"1 %d %d %d %d R %d %d %d",driverTypeToInt(driver_type), driver_num, SET_DIGITAL_POINT,stn,pnum,val,tt);
  else if(mchar == 17)// Q
    sprintf(msg,"1 %d %d %d %d Q %d %d %d",driverTypeToInt(driver_type), driver_num, SET_DIGITAL_POINT,stn,pnum,val,tt);
  else
    sprintf(msg,"1 %d %d %d %d %d %d %d %d",driverTypeToInt(driver_type), driver_num, SET_DIGITAL_POINT,stn,mchar,pnum,val,tt);


  // this->sendToInfoserver(msg);
  if(InfoLink::status_conduit){
    if(this->shouldPrint(addr) || InfoLink::log_digital_points){
      sprintf(lmsg,"setDigitalPoint() %s:%s",addr.c_str(),msg);
      this->ltf(lmsg);
    }
    return InfoLink::status_conduit->sendMessage((const char*)msg,&InfoLink::status_conduit_port);
  }
  return false;
}



bool InfoLink::setDigitalPoint(int driver_num, std::string driver_type, int snum, int pnum, char module_char, int val){
  return this->setDigitalPoint(driver_num,driver_type,snum,pnum,module_char,val,0);
}

bool InfoLink::setDigitalPoint(int snum, int pnum, char mchar, int val, int tt){
  return this->setDigitalPoint(this->driver_num,this->driver_type_string,snum,pnum,mchar,val,tt);
}
bool InfoLink::setDigitalPoint(int snum, int pnum, char mchar, int val){
  return this->setDigitalPoint(this->driver_num,this->driver_type_string,snum,pnum,mchar,val,0);
}



void InfoLink::sendThesePoints(std::string s){
  const char* lmsg;
  lmsg = s.c_str();
this->ltf(lmsg);
  this->info_client->sendMessage((BYTE *)lmsg,strlen(lmsg));
}



/**
 * Set status on multiple points at once.
 * @param  points vector of strings in this format:
 *                full point address <space> point type(AO,AI,DI etc) <space> value
 * @return        success/fail
 */
bool InfoLink::setPoints(std::vector<std::string> points){
  if(points.size() == 0) return true;
  std::string out;
  // std::vector<std::string> out;
  std::vector<std::string>::iterator it = points.begin();
  int count_me = 0;
  out = "117 LIST";
  while(it != points.end()){
    if(count_me > 20){// limit to only 50 addresses per msg
      count_me = 0;
      sendThesePoints(out);
      out = "117 LIST";
    }
    out.append(" "+ *it);
    it++;
    count_me++;
  }
  sendThesePoints(out);
  return true;
}

bool InfoLink::setModuleOffline(int driver_number, int station_number, char module_char){
  int snum = driver_number * 1000 + station_number;
  return this->setModuleOffline(snum, module_char);
}

bool InfoLink::setModuleOffline(int station_number, char module_char){
  if( station_number > 9999 || station_number < 1 || module_char < 'A' || module_char > 'R' || module_char == 'P')
    return false;

  int snum = 0;
  if(this->driver_num > 0 && station_number < 1000)
    snum = this->driver_num * 1000 + station_number;
  else
    snum = station_number;

  std::string out;
  int mod = -1;
  char msg[256];
  char lmsg[1024];
  std::string addr;
  sprintf(lmsg,"%d%c",snum,module_char);
  addr = std::string(lmsg);

  mod = moduleCharToInt(module_char);
  sprintf(msg,"4 %d %d 1", snum,mod);

  if(InfoLink::status_conduit){
    if(this->shouldPrint(addr) || true){
      sprintf(lmsg,"setModuleOffline():%s",msg);
      this->ltf(lmsg);
    }
    return InfoLink::status_conduit->sendMessage((const char*)msg,&InfoLink::status_conduit_port);
  }

  return false;
}


bool InfoLink::setModuleOnline(int driver_number, int station_number, char module_char){
  int snum = driver_number * 1000 + station_number;
  return this->setModuleOnline(snum, module_char);
}

bool InfoLink::setModuleOnline(int station_number, char module_char){
  if( station_number > 9999 || station_number < 1 || module_char < 'A' || module_char > 'R' || module_char == 'P')
    return false;

  int snum = 0;
  if(this->driver_num > 0 && station_number < 1000)
    snum = this->driver_num * 1000 + station_number;
  else
    snum = station_number;


  std::string out;
  int mod = -1;
  char msg[256];
  char lmsg[1024];
  std::string addr;
  sprintf(lmsg,"%d%c",snum,module_char);
  addr = std::string(lmsg);

  if(module_char == 'R'){
    sprintf(msg,"43 %d R 0", snum);
  }else{
    mod = moduleCharToInt(module_char);
    sprintf(msg,"43 %d %d 0", snum,mod);
  }

  if(InfoLink::status_conduit){
    if(this->shouldPrint(addr) || true){
      sprintf(lmsg,"setModuleOnline():%s",msg);
      this->ltf(lmsg);
    }
    return InfoLink::status_conduit->sendMessage((const char*)msg,&InfoLink::status_conduit_port);
  }

  return false;
}

bool InfoLink::setDfsStationOffline(int station_number){
  if( station_number > 9999 || station_number < 1 )
    return false;

  std::string out;
  char msg[256];
  char lmsg[1024];
  std::string addr;
  sprintf(lmsg,"%d",station_number);
  addr = std::string(lmsg);

  sprintf(msg,"4 %d R 1", station_number);

  if(InfoLink::status_conduit){
    if(this->shouldPrint(addr) || true){
      sprintf(lmsg,"setStationOffline():%s",msg);
      this->ltf(lmsg);
    }
    return InfoLink::status_conduit->sendMessage((const char*)msg,&InfoLink::status_conduit_port);
  }

  return false;
}

bool InfoLink::clearDfsStationOffline(int station_number){
  if( station_number > 9999 || station_number < 1 )
    return false;

  std::string out;
  char msg[256];
  char lmsg[1024];
  std::string addr;
  sprintf(lmsg,"%d",station_number);
  addr = std::string(lmsg);

  sprintf(msg,"42 %d R 1", station_number);

  if(InfoLink::status_conduit){
    if(this->shouldPrint(addr) || true){
      sprintf(lmsg,"clearDfsStationOffline():%s",msg);
      this->ltf(lmsg);
    }
    return InfoLink::status_conduit->sendMessage((const char*)msg,&InfoLink::status_conduit_port);
  }

  return false;
}



bool InfoLink::requestAnalog(std::string address){
  char lmsg[1024];
  char msg[256];

  sprintf( msg,"2 CUSTOM %s",address.c_str() );

  if(InfoLink::status_conduit){
    if(this->shouldPrint(address)){
      sprintf(lmsg,"request point:%s",msg);
      this->ltf(lmsg);
    }
    return InfoLink::status_conduit->sendMessage((const char*)msg,&InfoLink::status_conduit_port);
  }
  return false;
}


bool InfoLink::requestDigital(std::string address){
  // infoserver does not care about A/D in this context
  return this->requestAnalog(address);
}




//  End Message Functions ===============================================





















/**
    Logging Functions ***************************************************
*/

// Note: currently logging is set to on in constructor
void InfoLink::setLogFile(std::string p){
  this->do_logging = true;
  // Note: if the file exists it it will be logged to. Otherwise NO logging!
  if( p.empty() )
    this->dl = new DFS_Logger(std::string("/dfs/admin/logs/infolinker.log"));
  else
    this->dl = new DFS_Logger(p);
}

void InfoLink::ltf(std::string message)// log to file
{
  if(this->dl != NULL)
    this->dl->logMe(message);
}

void InfoLink::ltfc(const char* m){
  this->ltf(std::string(m));
}

void InfoLink::setWatchList(std::list<std::string> lst){
  this->watch_list = lst;
}

bool InfoLink::shouldPrint(std::string input){

  if( this->watch_list.empty() ) return false;
  std::list<std::string>::iterator it;
  it = this->watch_list.begin();
  while( it != this->watch_list.end() ){
    if(*it == std::string("ALL") ) return true;
    if( input.find(*it) != std::string::npos ){
      return true;
    }
    it++;
  }
  return false;
}

void InfoLink::logDigitalPoints(bool b){
  InfoLink::log_digital_points = b;
}
void InfoLink::logAnalogPoints(bool b){
  InfoLink::log_analog_points = b;
}

//  End Logging Functions ===============================================





























































































































































/**
    Test Functions ******************************************************
*/


/**
  example function to talk to a remote station
  This line is in the constructor to create the server used by this function
  where dfsunitest is a named socket in /etc/services

  this->makeTcpServer(50,string("dfsunitest"));

  The file  universal/tcp_tool.py is a way to simulate a remote station. It is
  a bit arcane in that you are required to enter values that mirror what the
  message to infoserver will look like. Here is an example of how to start
  this tool:
    python tcp_tool.py -t 192.168.20.144 -p 52444

  An example use of the tool is to type this command at the prompt:
    set digital 1 1 G 1
  This command sets station 1, point 1 on module G to value 1
  The driver number is added from this->driver_number
*/
void InfoLink::startStationMonitoring(void(*func_ptr)()){
  this->dl->logGood("starting station monitoring");
  char lmsg[2049];
  bool did_print = false;
  std::string sanitize_me,consume,addr,r,stn,cmd,d_type;
  int dnum,snum,pnum,timetag,lenn,int_val;
  char mchar;
  char checker;
  // float val;


  BYTE buf[MAX_DRIVER_MSG];
  char mbuf[MAX_DRIVER_MSG];
  int sockindex = -1;

  bool shouldsleep = true;
  // time_t rawtime;

  while(1) {


    did_print = false;
    shouldsleep = TRUE;


    if(this->server != NULL && this->server->isMessageReady() ) {
      lenn =this->server->readMessage(buf,MAX_DRIVER_MSG);
      sprintf(lmsg,"have bytes:%d",lenn);
      this->ltf(lmsg);

      // TODO: will have to segregate messages in containers tied to their socket index
      // when we allow more than one client to attach
      sockindex = this->server->getLastSocketIndex();
      InfoLink::LAST_SOCKET = sockindex;

      if(lenn > 0){
        memset(mbuf,0,sizeof(mbuf));
        if(!sanitize_me.empty()) sanitize_me.clear();
        if(!addr.empty()) addr.clear();
        if(!r.empty()) r.clear();

        snum = -1; pnum = -1; timetag = -1; mchar = ' '; int_val = -1.0;


        // TODO: traffic monitoring. how to send to thread we kicked off?
        // udpServer->broadcast((char *)buf);      // Send to any traffic monitors


        // sockindex = this->server->getLastSocketIndex();
        shouldsleep = FALSE;

        // time(&rawtime);
        // strncpy(mbuf, (char *)buf, sizeof(mbuf));
        strncpy(mbuf, (char *)buf, lenn);
        if(strchr(mbuf,'\n')){
          *strchr(mbuf,'\n')=0;
        }

        sanitize_me = std::string(mbuf);
        sanitize_me.erase(std::remove_if(sanitize_me.begin(),sanitize_me.end(), invalidChar), sanitize_me.end());

        // reply[0] = '\0';
        // sprintf(lmsg,"station buffer RECV:%s",mbuf);
        sprintf(lmsg,"station buffer RECV on %d:%s",sockindex,sanitize_me.c_str());
        this->ltf(lmsg);



        // echoes comm
        // sprintf(lmsg,"%d : %s",sockindex,sanitize_me.c_str());
        // this->server->sendMessage((BYTE*)lmsg, strlen(lmsg), sockindex);





        checker = sanitize_me.at(0);
        if( isdigit(checker) ){// message with magic number has arrived
          this->setStationComm(sanitize_me);



        }else{// this else section is for debug only. rather large, eh?


          if(sanitize_me == "status"){
            this->ltf("\n\nCurrently Stored Status");
            std::map<std::string,std::vector<std::string>>::iterator it = status.begin();
            while(it != status.end()){
              if( this->shouldPrint(it->first) ){
                this->ltf(it->first);
                std::vector<std::string>::iterator vi = it->second.begin();
                while(vi != it->second.end()){
                  this->ltf("\t"+ (*vi));
                  vi++;
                }
              }
              it++;
            }
            did_print = true;

          }

          else if(sanitize_me == "changed status"){
            this->ltf("\n\nChanged Status");
            std::queue<std::string> q = this->getStatusChanges();//this will clear status array
            std::string here_it_is;

            while( !q.empty() ){
              here_it_is = q.front();
              if(this->shouldPrint(here_it_is)){
                this->ltf(here_it_is);
              }
              q.pop();
            }
            did_print = true;


          }

          else if(sanitize_me == "alarms"){
            this->ltf("\n\nCurrently Stored Alarms");
            std::map<std::string,std::vector<std::string>>::iterator it = alarms.begin();
            while(it != alarms.end()){

              this->ltf(it->first);
              std::vector<std::string>::iterator vi = it->second.begin();
              while(vi != it->second.end()){
                this->ltf("\t"+ (*vi));
                vi++;
              }
              it++;
            }
            did_print = true;
            this->getAlarmChanges();//this will clear alarms

          }

          else if(sanitize_me.find("get status") == 0 && sanitize_me.size() > 11){
            addr = sanitize_me.substr(11);
            this->ltf("looking for "+addr);
            std::vector<std::string> v = this->getStatus(addr);
            r = this->messageVectorToString(v);
            this->ltf("status request for "+addr+" is:"+r);

          }

          else if(sanitize_me.find("get alarm") == 0){
            addr = sanitize_me.substr(10);
            this->ltf("looking for "+addr);
            std::vector<std::string> v = this->getAlarm(addr);
            r = this->messageVectorToString(v);
            this->ltf("alarm request for "+addr+" is:"+r);

          }

          else if(sanitize_me == "reset status"){
            InfoLink::initStatusSocket();
            this->ltf("did the status socket deed");
          }

          else if(sanitize_me == "reset alarm"){
            InfoLink::initAlarmSocket();
            this->ltf("did the alarm socket deed");
          }


















          else if(sanitize_me.find("set analog tt") == 0){

            std::istringstream istr(sanitize_me);
            istr >> consume >> consume >> consume >> snum >> pnum >> mchar >> int_val >> timetag;
            sprintf(lmsg,"the tokens: %d %d %c %d %d",snum,pnum,mchar,int_val,timetag);
            this->ltf(lmsg);

            this->setAnalogPointRaw(snum, pnum, mchar, int_val, timetag);

          }

          else if(sanitize_me.find("set analog dt") == 0){

            std::istringstream istr(sanitize_me);
            istr >> consume >> consume >> consume >> dnum >> d_type >> snum >> pnum >> mchar >> int_val;
            sprintf(lmsg,"the tokens: %d %d %c %d",snum,pnum,mchar,int_val);
            this->ltf(lmsg);

            this->setAnalogPointRaw(dnum, d_type, snum, pnum, mchar, int_val);

          }

          else if(sanitize_me.find("set analog") == 0){

            std::istringstream istr(sanitize_me);
            istr >> consume >> consume >> snum >> pnum >> mchar >> int_val;
            sprintf(lmsg,"the tokens: %d %d %c %d",snum,pnum,mchar,int_val);
            this->ltf(lmsg);

            this->setAnalogPointRaw(snum, pnum, mchar, int_val);

          }




























          else if(sanitize_me.find("set digital tt") == 0){
            std::istringstream istr(sanitize_me);
            istr >> consume >> consume >> consume >> snum >> pnum >> mchar >> int_val >> timetag;
            sprintf(lmsg,"dig tt tokens: %d %d %c %d %d ",snum,pnum,mchar,int_val,timetag);
            this->ltf(lmsg);
            if(this->setDigitalPoint(snum, pnum, mchar, int_val,timetag) ){
              sprintf(lmsg,"this was true");
            }else{
              sprintf(lmsg,"this was NOT true");
            }
            this->ltf(lmsg);
          }


          else if(sanitize_me.find("set digital driver tt") == 0){
            std::istringstream istr(sanitize_me);
            istr >> consume >> consume >> consume >> consume >> dnum >> d_type >> snum >> pnum >> mchar >> int_val >> timetag;

            sprintf(lmsg,"the tokens: %d %s %d %d %c %d %d",dnum,d_type.c_str(),snum,pnum,mchar,int_val,timetag);
            this->ltf(lmsg);
            if( this->setDigitalPoint(dnum,d_type,snum, pnum, mchar, int_val, timetag ) ){
              sprintf(lmsg,"this was true");
            }else{
              sprintf(lmsg,"this was NOT true");
            }
            this->ltf(lmsg);

          }


          else if(sanitize_me.find("set digital driver pp") == 0){
            std::istringstream istr(sanitize_me);
            istr >> consume >>consume >> consume >> consume >> dnum >> d_type >> snum >> pnum >> mchar >> int_val;

            sprintf(lmsg,"the tokens: %d %s %d %d %c %d",dnum,d_type.c_str(),snum,pnum,mchar,int_val);
            this->ltf(lmsg);
            if( this->setDigitalPulsePoint(dnum,d_type,snum, pnum, mchar, int_val ) ){
              sprintf(lmsg,"this was true");
            }else{
              sprintf(lmsg,"this was NOT true");
            }
            this->ltf(lmsg);

          }

          else if(sanitize_me.find("set digital driver") == 0){
            std::istringstream istr(sanitize_me);
            istr >> consume >> consume >> consume >> dnum >> d_type >> snum >> pnum >> mchar >> int_val;

            sprintf(lmsg,"the tokens: %d %s %d %d %c %d",dnum,d_type.c_str(),snum,pnum,mchar,int_val);
            this->ltf(lmsg);
            if( this->setDigitalPoint(dnum,d_type,snum, pnum, mchar, int_val ) ){
              sprintf(lmsg,"this was true");
            }else{
              sprintf(lmsg,"this was NOT true");
            }
            this->ltf(lmsg);

          }

          else if(sanitize_me.find("set digital") == 0){
            std::istringstream istr(sanitize_me);
            istr >> consume >> consume >> snum >> pnum >> mchar >> int_val;
            sprintf(lmsg,"the tokens: %d %d %c %d",snum,pnum,mchar,int_val);
            this->ltf(lmsg);
            if( this->setDigitalPoint(snum, pnum, mchar, int_val ) ){
              sprintf(lmsg,"this was true");
            }else{
              sprintf(lmsg,"this was NOT true");
            }
            this->ltf(lmsg);

          }











          else if(sanitize_me.find("set module offline dr") == 0){
            std::istringstream istr(sanitize_me);
            istr >> consume >> consume >> consume >> consume >> dnum >> snum >> mchar;
            sprintf(lmsg,"the tokens: %d %c",snum,mchar);
            this->ltf(lmsg);
            this->setModuleOffline(dnum, snum, mchar);

          }

          else if(sanitize_me.find("set module offline") == 0){
            std::istringstream istr(sanitize_me);
            istr >> consume >> consume >> consume >> snum >> mchar;
            sprintf(lmsg,"the tokens: %d %c",snum,mchar);
            this->ltf(lmsg);
            this->setModuleOffline(snum, mchar);

          }

          else if(sanitize_me.find("set module online") == 0){
            std::istringstream istr(sanitize_me);
            istr >> consume >> consume >> consume >> snum >> mchar;
            sprintf(lmsg,"the tokens: %d %c",snum,mchar);
            this->ltf(lmsg);
            this->setModuleOnline(snum, mchar);

          }


          else if(sanitize_me.find("set dfs station offline") == 0){
            std::istringstream istr(sanitize_me);
            istr >> consume >> consume >> consume >> consume >> snum;
            sprintf(lmsg,"the tokens: %d",snum);
            this->ltf(lmsg);
            this->setDfsStationOffline(snum);

          }

          else if(sanitize_me.find("clear station offline") == 0){
            std::istringstream istr(sanitize_me);
            istr >> consume >> consume >> consume >> snum;
            sprintf(lmsg,"the tokens: %d",snum);
            this->ltf(lmsg);
            this->clearDfsStationOffline(snum);

          }












          else if(sanitize_me.find("set command") == 0){
            std::istringstream istr(sanitize_me);
            istr >> consume >> consume >> cmd;
            sprintf(lmsg,"the tokens: %s",cmd.c_str());
            this->ltf(lmsg);
            this->setCommand(cmd);

          }

          else if(sanitize_me == "commands"){
            this->ltf("\n\nCurrent Commands");
            InfoLink::command_mutex.lock();
            std::vector<std::string>::iterator it = commands.begin();
            while(it != commands.end()){
              this->ltf(*it);
              it++;
            }
            InfoLink::command_mutex.unlock();
            did_print = true;

          }

          else if(sanitize_me.find("mass update") == 0){
            std::istringstream istr(sanitize_me);
            // int stn,module;
            // char mchar;
            istr >> consume >> consume;
            std::vector<std::string> v;
            std::string s;
            char group[128];
            char tag[MAXADDRLEN * 2];
            char iotype[16];
            char value[32];

            while(istr){
              memset(tag, 0, sizeof(tag));
              memset(iotype, 0, sizeof(iotype));
              memset(value, 0, sizeof(value));
              memset(group, 0, sizeof(group));
              istr >> tag >> iotype >> value;
              // istr >> tag >> value;
              if (tag[0] == '\0')
                  continue;

              sprintf(group,"%s %s %s",tag,iotype,value);
                // sprintf(group,"%s %s",tag,value);
              v.push_back(std::string(group));
            }
            this->setPoints(v);

          }


          if(did_print)
            this->ltf("\n");
        }

        memset(buf,0,sizeof(buf));
      }
    }// end read if()

    if(shouldsleep){
      // this->dl->logMeSL("#");//log print flag to see if loop is going
      sleep(1);
      if( (*func_ptr) != NULL )
        (*func_ptr)();
    }

  }  //end while

}


bool InfoLink::sendToStation(std::string s, int sockindex){
  if(s.size() < 1) return false;
  char lmsg[256];
  bool result;
  // TODO: until we handle more connections, alsway get last
  sockindex = InfoLink::LAST_SOCKET;
  if(sockindex < 0) return false;

  if(show_station_send){
    sprintf(lmsg,"sendToStation() %s  on %d",s.c_str(),sockindex);
    this->ltf(lmsg);
  }

  result = this->server->sendMessage((BYTE*)s.c_str(), s.size(), sockindex);
  if(result == 0){
    this->dl->logError("lost socket");
    delete this->server;
    this->server = NULL;
    this->makeTcpServer(this->tcp_connections,this->name_of_port_on_hsm);
  }
  return (bool)result;
}

void InfoLink::setStationLogging(bool b){
  show_station_send = b;
}


/*
int main(){
  InfoLink* il = new InfoLink();
  il->testMe();
  delete il;

  return 0;
}
*/
//    End Test Functions ================================================
