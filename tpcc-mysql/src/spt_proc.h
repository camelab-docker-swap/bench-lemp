#ifndef _SPT_PROC_H_
#define _SPT_PROC_H_

#include <mysql.h>

int error(MYSQL *mysql, MYSQL_STMT *mysql_stmt);

#define TIMESTAMP_LEN 80
#define STRFTIME_FORMAT "%Y-%m-%d %H:%M:%S"

#endif
