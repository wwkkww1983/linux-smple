#include <stdio.h>


int main(int argc, char *argv[])
{
	sqlite3 *db;
	sqlite3_stmt * stmt = NULL;
	int result;
	char *errmsg = NULL;
	result = sqlite3_open("mdatabase.db",&db);  
	if(result != SQLITE_OK)
	{
		printf("Can't open datebase\n%s\n",sqlite3_errmsg(db));
		return -1;
	}
	result = sqlite3_exec(db,"create table Cmdtable(ID,name,model,command,parsetype,startadder,datenum,keep,info)",0,0, &errmsg);
	//创建一个表，表名Cmdtable
	if(result != SQLITE_OK)
	{
		printf("warning:Create table fail! May table already.\n");
	}
	result = sqlite3_exec(db,"create table Datedable(ID,type,vaule)",0,0, &errmsg);
	//创建一个表，表名Datetable
	if(result != SQLITE_OK)
	{
		printf("warning:Create table fail! May table already.\n");
	}
	char sql[512] = "insert into Cmdtable (ID,command) values (?,?)"; //插入数据库语句

	int ret = sqlite3_prepare(db,sql,-1,&stmt,NULL); //准备好SQL语句
	if (ret != SQLITE_OK){
                printf("prepare fail \n");
                return ret;
        }

	int id = 1213;
    char a[20] = "13245";
	sqlite3_bind_int(stmt,1,id); //绑定参数, 注意绑定参数的初始index值为1
    sqlite3_bind_text(stmt,2,a,32,NULL);
    ret = sqlite3_step(stmt); //执行语句
	 if(ret == SQLITE_DONE){   //执行结果
                ret = SQLITE_OK;
        }
        sqlite3_finalize(stmt); //释放资源

	result = sqlite3_exec(db,"insert into Cmdtable (name,model,parsetype,startadder,datenum,keep,info) values('temp','taida','float',3,4,2,'not')",0,0,&errmsg);
	//插入一条记录
	if(result != SQLITE_OK) {
		printf("Can't insert into datebase：%s\n",errmsg);
	}
	result = sqlite3_exec(db,"select * from Mytable",0,0,&errmsg);

	int nrow = 0, ncol = 0, i,j;
	char **table;
	
	printf("\n");
	sqlite3_get_table(db,"select * from Cmdtable",&table,&nrow,&ncol,&errmsg);
	printf("row:%d col:%d\n",nrow,ncol);
	printf("The result of querying is:\n");
	for (i = 0; i < nrow+1; i++) {
		for (j = 0; j < ncol; j++) {
			printf("%-9s	",table[i*ncol+j]);
		}
		printf("\n");
	}

	sqlite3_free_table(table);
	
	sqlite3_close(db); 
	return 0;
}

