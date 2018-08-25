#include <sqlite3.h>

//SQLlite
sqlite3 *db = NULL;
int result;
sqlite3_stmt * stmt = NULL;
char *errmsg = NULL;

char sql[100] = "create table Cmdtable(ID,name,model,command,parsetype,startadder,datenum,keep,info)";




