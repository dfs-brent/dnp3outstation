#include "dfs_logger.hpp"


DFS_Logger::DFS_Logger(std::string full_filename){

  if(full_filename.empty())
    fname = std::string("/tmp/dfs_logger.log");
  else
    fname = full_filename;
  logfile = NULL;
  to_console = false;
  logging = this->checkLogFile();
}

DFS_Logger::~DFS_Logger(){
  if(logfile != NULL)
    fclose(logfile);
}

bool DFS_Logger::logExists(){
  return access( fname.c_str(), F_OK | W_OK ) == 0;
}

void DFS_Logger::printToForeground(){
  to_console = true;
}


bool DFS_Logger::checkLogFile() {
  bool r = false;
  if ( this->logExists() ) { // our log file exists
    if ( logfile == NULL ) {
      if ((logfile = fopen(fname.c_str(), "w")) != NULL) {
        fprintf(logfile, "successfully created logger\n");
        r = true;
      }
    }else r = true;

  } else if (logfile != NULL) {
    logfile = NULL;
  }

  logging = r;
  return r;
}

void DFS_Logger::logMe(const char* msg){
  if( !this->logExists() ) return;
  // std::string s = std::string(this->RESET);
  // s.append(std::string(msg));
  // this->logMe(s);
  this->logMe(std::string(msg));
}
void DFS_Logger::logMe(std::string msg) {
  if( !this->logExists() ) return;

  if ( msg.empty()) return;
  msg.insert(0,this->RESET);
  msg.append("\n"+this->RESET);
  this->doOut(msg);

}

void DFS_Logger::logMeSL(const char* msg){
  if( !this->logExists() ) return;
  this->logMeSL(std::string(msg));
}
void DFS_Logger::logMeSL(std::string msg) {
  if( !this->logExists() ) return;
  if ( msg.empty()) return;
  msg.insert(0,this->RESET);
  this->doOut(msg);

}

void DFS_Logger::logError(const char* msg){
  if( !this->logExists() ) return;
  this->logError(std::string(msg));
}
void DFS_Logger::logError(std::string msg){
  if( !this->logExists() ) return;
  if ( msg.empty()) return;
  msg.insert(0,std::string(this->RED));
  msg.append(std::string(this->RESET));
  msg.append("\n");
  this->doOut(msg);
}



void DFS_Logger::logGood(const char* msg){
  if( !this->logExists() ) return;
  this->logGood(std::string(msg));
}
void DFS_Logger::logGood(std::string msg){
  if( !this->logExists() ) return;
  if ( msg.empty()) return;
  msg.insert(0,std::string(this->GREEN));
  msg.append(std::string(this->RESET));
  msg.append("\n");
  this->doOut(msg);
}


void DFS_Logger::logWarning(const char* msg){
  if( !this->logExists() ) return;
  this->logWarning(std::string(msg));
}
void DFS_Logger::logWarning(std::string msg){
  if( !this->logExists() ) return;
  if ( msg.empty()) return;
  msg.insert(0,std::string(this->YELLOW));
  msg.append(std::string(this->RESET));
  msg.append("\n");
  this->doOut(msg);
}


std::string DFS_Logger::getColor(std::string color){
  std::string c = color;
  if( c.empty() ) return this->RESET;
  std::transform(c.begin(), c.end(),c.begin(), ::toupper);
  if( c == "RED")
    return this->RED;
  else if( c == "GREEN")
    return this->GREEN;
  else if( c == "YELLOW")
    return this->YELLOW;
  else if( c == "BLUE")
    return this->BLUE;
  else if( c == "MAGENTA")
    return this->MAGENTA;
  else if( c == "CYAN")
    return this->CYAN;
  else
    return this->RESET;
}

void DFS_Logger::logColor(const char* msg, const char* color){
  this->logColor(std::string(msg), std::string(color));
}
void DFS_Logger::logColor(std::string msg, std::string color){
  if( !this->logExists() || msg.empty() ) return;
  msg.insert(0,this->getColor(color));
  msg.append(std::string(this->RESET));
  msg.append("\n");
  this->doOut(msg);
}







void DFS_Logger::doOut(std::string msg){
  if ( msg.empty()) return;

  if ( checkLogFile() && logfile != NULL ) {
    fprintf(logfile, msg.c_str() );
    fflush(logfile);
    return;
  }

  if(to_console){
    printf( msg.c_str() );
  }

}
