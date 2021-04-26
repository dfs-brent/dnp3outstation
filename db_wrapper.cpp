#include "db_wrapper.hpp"


using namespace std;

MYSQL* Db_wrapper::connection = NULL;
bool show_query = false;
std::string local_error = "";
FILE* sql_logfile = NULL;
FILE* sql_error_log = NULL;

void logMe(std::string msg){
    if( msg.empty()) return;


    if ( access( "/dfs/admin/logs/mesh_sql_logger", F_OK|W_OK ) == 0 ){// our log file exists
        if( sql_logfile == NULL ){
            if((sql_logfile = fopen("/dfs/admin/logs/mesh_sql_logger","w")) != NULL) {
                fprintf(sql_logfile, "successfully created logger\n");
            }
        }

        if( sql_logfile != NULL){

            msg.append("\n");
            fprintf(sql_logfile, msg.c_str() );
            fflush(sql_logfile);
            return;
        }

    }else if(sql_logfile != NULL){
        sql_logfile = NULL;

    }

    msg.append("\n");
    printf(msg.c_str() );
}

void logError(std::string msg){
    if( msg.empty()) return;


    if ( access( "/dfs/admin/logs/mesh_sql_error_logger", F_OK|W_OK ) == 0 ){// our log file exists
        if( sql_error_log == NULL ){
            if((sql_error_log = fopen("/dfs/admin/logs/mesh_sql_error_logger","w")) != NULL) {
                fprintf(sql_error_log, "successfully created logger\n");
            }
        }

        if( sql_error_log != NULL){

            msg.append("\n");
            fprintf(sql_error_log, msg.c_str() );
            fflush(sql_error_log);
            return;
        }

    }else if(sql_error_log != NULL){
        sql_error_log = NULL;

    }

    msg.append("\n");
    printf(msg.c_str() );
}



Db_wrapper::Db_wrapper(){
    Db_wrapper::result = NULL;
    Db_wrapper::connection = NULL;
}


Db_wrapper::~Db_wrapper()
{
    ///cout << "\nDestruction!\n" << endl;
    if( Db_wrapper::result != NULL )
        mysql_free_result(Db_wrapper::result);

    if( Db_wrapper::connection != NULL )
        mysql_close(Db_wrapper::connection);
}

bool Db_wrapper::isConnected(){
    return Db_wrapper::connected;
}

bool Db_wrapper::resetConnection(){
    Db_wrapper::connected = false;
    if( Db_wrapper::result != NULL )
        mysql_free_result(Db_wrapper::result);

    if( Db_wrapper::connection != NULL )
        mysql_close(Db_wrapper::connection);

    return Db_wrapper::initConnection();
}

void Db_wrapper::set_show_queries(bool mode) {
    show_query = mode;
}

std::string Db_wrapper::getLocalError(){
    return local_error;
}

bool Db_wrapper::initConnection(){
    Db_wrapper::connection = mysql_init(NULL);

    if ( mysql_real_connect(Db_wrapper::connection, "localhost", "dfs", "qball", "hypertacii", 0, NULL, 0) == NULL)
    {
        local_error = std::string("Database Db_wrapper::initConnection() error\n");
        logError(local_error);
        Db_wrapper::connected = false;
        return false;
    }
    Db_wrapper::connected = true;
    return true;
}

std::string Db_wrapper::set_sql(string sql) {

    if( Db_wrapper::connection == NULL ) {
        Db_wrapper::result = NULL;
        /*Db_wrapper::connection = mysql_init(NULL);

        if ( mysql_real_connect(connection, "localhost", "dfs", "qball", "hypertacii", 0, NULL, 0) == NULL)
        {
            cout << "Database Db_wrapper::connection error" << endl;
            // throw 1;
            return "";
        }*/
        if( ! initConnection() )
            return "";
    }
    sql_statement = sql;

    // if( show_query ) std::cout << "\n========= Query\n" << sql << "\n========== end query\n" << endl;
    if( show_query ){
        std::string x =  "\n========= Query\n" + sql + "\n========== end query\n\n";
        return x;
    }
    return "";
}

int Db_wrapper::execute_statement()
{
    char lmsg[2048];
    int r = mysql_real_query(Db_wrapper::connection, sql_statement.c_str(), sql_statement.length());
    if(r != 0){
        sprintf(lmsg,"\n*=*=*\nError Db_wrapper->execute_sql()\nsql text: %s\nmysql_errno: %d\n mysql_error: %s\n*=*=*\n",sql_statement.c_str(),mysql_errno(Db_wrapper::connection),mysql_error(Db_wrapper::connection));
        local_error = std::string(lmsg);
        logError(local_error);
        resetConnection();
    }
    return r;
}

bool Db_wrapper::execute_select()
{
    char lmsg[2048];
    Db_wrapper::free_result();

    int x = mysql_real_query(Db_wrapper::connection, sql_statement.c_str(), sql_statement.length());
    if( x != 0 ) {
        // cout << "\n*=*=*=*=*=*=*=*=*=*=\nError Db_wrapper->execute_sql()\nsql text: " << sql_statement<< "\nmysql_errno: " << mysql_errno(Db_wrapper::connection) << "\n mysql_error: " << mysql_error(Db_wrapper::connection) << "*=*=*=*=*=*=*=*=*=*" << endl;
        sprintf(lmsg,"\n*=*=*\nError Db_wrapper->execute_sql()\nsql text: %s\nmysql_errno: %d\n mysql_error: %s\n*=*=*\n",sql_statement.c_str(),mysql_errno(Db_wrapper::connection),mysql_error(Db_wrapper::connection));
        local_error = std::string(lmsg);
        logError(local_error);
        resetConnection();
        return false;
    }
    Db_wrapper::result = mysql_store_result(Db_wrapper::connection);
    return result != NULL;
}



MYSQL_ROW Db_wrapper::get_next_row() {
    if(Db_wrapper::result == NULL) return NULL;
    return mysql_fetch_row(Db_wrapper::result);
}

void Db_wrapper::free_result() {
    if( Db_wrapper::result != NULL ){
        mysql_free_result(Db_wrapper::result);
        Db_wrapper::result = NULL;
    }
}

int Db_wrapper::get_row_count() {
    if( Db_wrapper::result == NULL )
        return -1;
    return mysql_num_rows(Db_wrapper::result);
}
