#ifndef DB_WRAPPER_H
#define DB_WRAPPER_H

#include <mysql/mysql.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <iostream>

/*
    Simple wrapper for database access
 */


class Db_wrapper
{
protected:

             MYSQL_RES*  result;
             std::string sql_statement;
             bool connected;

public:
            Db_wrapper();
            ~Db_wrapper();

            static MYSQL*      connection;

            std::string set_sql(std::string sql);    //user sets query string
            int execute_statement();          // executes a non-select statement
            bool execute_select();            // executes a select statement
            MYSQL_ROW get_next_row();         // get next row of select query
            void free_result();               // when doing multiple queries, allows freeing
            void set_show_queries(bool mode); // debug mode, shows every db access in log file
            int get_row_count();

            bool resetConnection();
            bool initConnection();
            bool isConnected();
            std::string getLocalError();

};
#endif
