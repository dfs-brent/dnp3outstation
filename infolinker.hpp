#ifndef INFOLINKER_HPP
#define INFOLINKER_HPP

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <iterator>
#include <vector>
#include <map>
#include <algorithm>
#include <mutex>
#include <queue>

// HTLIB includes
#include <network/tcpserver.hpp>
// #include <network/udpclient.hpp>
#include <time/nanosleeper.hpp>
#include <ipc/unixsocketclient.hpp>
#include <threads/basicmutex.hpp>
#include <data/queue.hpp>
#include <util/buffer.hpp>
#include <pthread.h>


// Local includes
#include "net.hpp"
#include "udpclient.hpp"
#include "dfsnet.h"
#include "dfs.h"
#include "dfs_logger.hpp"
#include "db_wrapper.hpp"

#define IPC_SELF "/dfs/hypertac/runtime/universal"
#define IPC_SERVER "/dfs/hypertac/runtime/infoserver"
//Buffer sizes and max station count
#define MAX_STATIONS 254
#define INPUT_BUFFER_SIZE 2048
#define INFOSERVER_OUT_BUFFER_SIZE 2048
// #define MAX_DRIVER_MSG 2048


/*TODO
*   create a watcher thread to keep tabs on the tcp & udp & unix socket connections.
*   If they go away, make a new conneciton
*/


class InfoLink {
  private:

    // constants
    std::string INFOLINK_VERSION = "0.0.1";
    int which_constructor;
    // this name of port is for debug only at this point. This server listens
    // for traffic that simulates a user of this class sending commands/requests
    // to this class. Used as a tcp connection
    std::string name_of_port_on_hsm = "whatcha";
    int tcp_connections = 50;

    std::string udp_host_ip = "localhost";
    std::string udp_host_port = "dfsinfo";


    // buffers
    char infobuf[INFOSERVER_OUT_BUFFER_SIZE];

    // clients and servers
    UnixSocketClient *info_client = NULL;
    long usc_last_comm = 0;
    UDPServer *udp_server = NULL;
    long udp_last_comm = 0;
    TcpServer *server = NULL;
    long tcp_last_comm = 0;
    Db_wrapper* dbw;            // database interface


    static UDPClient *status_conduit;
    static struct sockaddr_in status_conduit_port;

    static UDPClient *alarm_conduit;
    static struct sockaddr_in alarm_conduit_port;

    // TODO: need to handle multiple clients on station server. This is simply
    // to keep a ref to the last socket index which will never change since
    // the code only handles one connection. Allows me to write some functions
    // that will handle multiple socket indicies
    static int LAST_SOCKET;


    pthread_t info_control_thread;
    pthread_t status_listener_thread;

    // state variables
    bool do_logging = false;
    static bool alarm_change;
    static bool status_change;
    static bool have_command;
    static bool have_station_comm;

    static bool log_analog_points;
    static bool log_digital_points;

    int driver_num = -1;
    int driver_type = 0;//dfs.h dfs=1, modbus=6, netdfp=7, mesh=8, uni=2
    std::string driver_type_string = "";
    static BasicMutex *sendMutex;
    std::string driver_msg;
    int killall = 1;
    std::list<std::string> addresses;

    // these maps store state
    static std::map<std::string,std::vector<std::string>> status;
    static std::map<std::string,std::vector<std::string>> alarms;

    // these queues store the address of changes. The user will walk the queue
    // and use the addresses stored there to pull values from the maps above
    static std::queue<std::string> changed_alarms;
    static std::mutex changed_alarms_mutex;

    static std::queue<std::string> changed_status;
    static std::mutex changed_status_mutex;

    static std::vector<std::string> commands;
    static std::mutex command_mutex;

    static std::vector<std::string> station_comm;
    static std::mutex station_comm_mutex;



    // functions PRIVATE
    bool sendToInfoserver(char* message);
    bool sendToInfoserver(std::string message);
    void makeTcpServer(int connections, std::string port);
    static void* listenToInfoserver (void *s);
    static void* listenToInfoserverStatus (void *s);
    static void* listenToInfoserverAlarm (void *s);

    static void initStatusSocket();
    static void initStatusSocket(std::string ip, std::string port);
    static void initAlarmSocket();


    // helper functions
    std::vector<std::string> split(std::string split_me, char using_this);
    std::string messageVectorToString(std::vector<std::string> vec);
    void sendThesePoints(std::string s);

    static void resetAlarmChange();
    static void gotAlarmChange();
    static void resetStatusChange();
    static void gotStatusChange();
    static void resetCommand();
    static void gotCommand();
    static void resetStationComm();
    static void gotStationComm();

    static void setCommand(std::string cmd);
    static void setStatus(std::string status);
    static void setAlarm(std::string alarm);
    static void setStationComm(std::string comm);

    bool loadDriverAddresses(int driver);
    void setInitial();




















  public:
    // dfs logging library
    DFS_Logger* dl = NULL;
    static DFS_Logger* static_logger;

    void (*func_ptr_alarm)() = NULL;
    void (*func_ptr_status)() = NULL;
    void (*func_ptr_command)() = NULL;

    /**
        use class from a driver's perspective **********************************
    */

    InfoLink(int driver_number,
            std::string driver_type,
            std::string path_to_addresses,
            std::string log_file_path);
    ~InfoLink();

    // initialization of object
    void loadAddresses(std::string a);
    bool loadUASP(std::string path);

    void startListenerThreadFromInfoserver(void(*func_ptr)());

    void startStationMonitoring(void(*func_ptr)());

    void startStatusListenerThreadFromInfoserver(void(*func_ptr)());
    void startAlarmListenerThreadFromInfoserver(void(*func_ptr)());

    void setLogFile(std::string p);

    // getters/setters
    int getDriverNumber();
    int getDriverType();
    void setRemoteIP(std::string ip);
    void setRemotePort(std::string port);
    void setRemotePort(int port);
    std::list<std::string> getWatchedAddresses();



    // event flags
    bool alarmChange();
    bool statusChange();
    bool haveCommand();
    bool haveStationComm();


    // data source
    // NOTE: these return copies of the internal container AND the internal
    // container is CLEARED since we assume you will handle the change events
    // contained in the containers
    std::queue<std::string> getAlarmChanges();
    std::queue<std::string> getStatusChanges();
    std::vector<std::string> getCommands();
    std::vector<std::string> getStationComm();


    // status request functions that show current state of alarm & status
    std::vector<std::string> getAlarm(std::string address);
    std::vector<std::string> getStatus(std::string address);


    //  End Class as a driver ==================================================





    /**
        Use class as a status/control client ***********************************
    */

    // use class as a client to watch/control points on 1 or more drivers
    InfoLink(std::vector<int> drivers_to_watch, std::string log_file_path);

    // use class as a client to watch/control points from an address file
    InfoLink(std::string path_to_addresses, std::string log_file_path);

    InfoLink(int driver_number, std::string driver_t, std::string log_file_path);

    void setAddresses(std::list<std::string> addrs);

    bool requestAnalog(std::string address);
    bool requestDigital(std::string address);


    //  End class as a status/client ===========================================







    /**
        Infoserver Messages Functions *****************************************
    */

    void sendToInfoserver();



    //  End Infoserver Messages Functions =====================================




    /**
        DFS stations -------------------------------------------------------------
    */

    bool setAnalogPointRaw(int driver_number, std::string driver_type, int station_number, char module_char, int point_number, int raw_value, int time_tag);
    bool setAnalogPointRaw(int driver_number, std::string driver_type, int station_number, char module_char, int point_number, int raw_value);
    bool setAnalogPointRaw(int station_number, char module_char, int point_number, int raw_value, int time_tag);
    bool setAnalogPointRaw(int station_number, char module_char, int point_number, int raw_value);

    bool setAnalogQPoint(int driver_number, std::string driver_type, int station_number, char module_char, int point_number, float float_value, int time_tag);
    bool setAnalogQPoint(int driver_number, std::string driver_type, int station_number, char module_char, int point_number, float float_value);
    bool setAnalogQPoint(int station_number, char module_char, int point_number, float float_value, int time_tag);
    bool setAnalogQPoint(int station_number, char module_char, int point_number, float float_value);




    // set a digital point where you can specify the driver number
    // driver type is "DFS" at this point but will add MOD and DNP
    bool setDigitalPoint(int driver_number, std::string driver_type, int station_number, char module_char, int point_number, int value, int time_tag);
    bool setDigitalPoint(int driver_number, std::string driver_type, int station_number, char module_char, int point_number, int value);

    // digital values as raw (int)
    // assumes you are using the driver number you set during instantiation
    bool setDigitalPoint(int station_number, char module_char, int point_number, int value, int time_tag);
    bool setDigitalPoint(int station_number, char module_char, int point_number, int value);


    bool setDigitalPulsePoint(int driver_number, std::string driver_type, int station_number, char module_char, int point_number, int pulses, int time_tag);
    bool setDigitalPulsePoint(int driver_number, std::string driver_type, int station_number, char module_char, int point_number, int pulses);
    bool setDigitalPulsePoint(int station_number, char module_char, int point_number, int pulses, int time_tag);
    bool setDigitalPulsePoint(int station_number, char module_char, int point_number, int pulses);



    // bool setModuleStatus(int station_number, char module_char, std::vector<std::string> statuses);
    bool setPoints(std::vector<std::string> points);

    // DFS station offline actions
    bool setModuleOffline(int station_number, char module_char);
    bool setModuleOffline(int driver_number, int station_number, char module_char);
    bool setModuleOnline(int station_number, char module_char);
    bool setModuleOnline(int driver_number, int station_number, char module_char);
    bool setDfsStationOffline(int station_number);
    bool clearDfsStationOffline(int station_number);

    // End DFS stations ========================================================



    // Modbus stations
    // bool setAnalogPoint(int station_number, int point_number, float value, int time_tag);
    // bool setAnalogPoint(int station_number, int point_number, float value);



    //  End Direct Manipulation Functions ======================================




/**
    Station Comm -------------------------------------------------------------
*/

    bool sendToStation(std::string s, int sockindex);
    bool show_station_send = false;
    void setStationLogging(bool b);


// End Station Comm ============================================================





















    /**
        Logging Functions ***************************************************
    */
    std::list<std::string> watch_list;
    void setWatchList(std::list<std::string> lst);
    bool shouldPrint(std::string);
    void logDigitalPoints(bool b);
    void logAnalogPoints(bool b);
    void ltf(std::string);//log to file
    void ltfc(const char* m);

    //  End Logging Functions ===============================================






};

#endif
