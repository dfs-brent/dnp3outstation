#ifndef DFS_LOGGER_HPP
#define DFS_LOGGER_HPP

// See note at bottom for usage notes

#include <stdio.h>
#include <unistd.h>
#include <string>
#include <algorithm>


class DFS_Logger
{
private:
  FILE *logfile;
  std::string fname;
  bool to_console;
  bool logging;

  bool logExists();
  bool checkLogFile();
  void doOut(std::string msg);

  std::string RED     = std::string("\x1b[31m");
  std::string GREEN   = std::string("\x1b[32m");
  std::string YELLOW  = std::string("\x1b[33m");
  std::string BLUE    = std::string("\x1b[34m");
  std::string MAGENTA = std::string("\x1b[35m");
  std::string CYAN    = std::string("\x1b[36m");
  std::string RESET   = std::string("\x1b[0m");

public:
  DFS_Logger(std::string full_filename);
  ~DFS_Logger();

  void printToForeground();
  void logMe(std::string msg);
  void logMe(const char* msg);
  void logMeSL(std::string msg);
  void logMeSL(const char* msg);
  void logError(std::string msg);
  void logError(const char* msg);
  void logGood(std::string msg);
  void logGood(const char* msg);
  void logWarning(std::string msg);
  void logWarning(const char* msg);

  std::string getColor(std::string color);
  void logColor(const char* msg, const char* color);
  void logColor(std::string msg, std::string color);
};

extern DFS_Logger* dl;
/**
  The above dl variable can be accessed via the -> operator if you do this:
    1. add this include in your hpp file
        #include "dfs_logger.hpp"
    2. add this as a file scope variable in your cpp file:
        DFS_Logger* dl;
    3. create instance in your main() with:
      dl = new DFS_Logger(std::string("/dfs/admin/logs/unique_name"));
    4. print to log via dl->logMe(std::string("print this"));
    5. newlines added automatically
    6. Most important, touch/create the log file first OR there is nothing
      to log too. The code checks for the log file's existence before printing
*/
#endif
