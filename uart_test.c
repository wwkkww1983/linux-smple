#include     <stdio.h> 
#include     <stdlib.h>
#include     <string.h>
#include     <unistd.h>
#include     <sys/types.h>
#include     <sys/stat.h>  
#include     <fcntl.h>  
#include     <termios.h>
#include     <errno.h> 
#include     <pthread.h> 
#include     <sys/ioctl.h> 
#include     <time.h> 
#include    <sqlite3.h>
#include    <math.h>
#include	<errno.h>//For errno-the error number
#include	<signal.h>
#include	<string.h>//memset
#include	<sys/wait.h>
#include	<sys/time.h>
#include	<sys/socket.h>
#include	<arpa/inet.h>
#include	<netinet/in.h>
#include	<unistd.h>
#include	<netdb.h> //hostent
#include	"cJSON.h"
#include 	<ctype.h>

#define FALSE 1
#define TRUE 0


int fd = -1;//串口文件描述符 

char buff[30];//接收RS485数据缓存 

int flag_rec = 0; //串口接收完成标志 
int flag_up = 0;//上传数据标志（1：表示正在上传 0：表示没有） 
int flag_down = 1;//下载设备信息标志 
int flag_net = 0;//网络连接状态（1：标志连接正常） 
int flag_data = 0;//数据库数据存在标志（1：表示数据库有数据等待上传） 
int flag_collec = 0;//采集标志（1：表示正在采集） 
//SQLlite
sqlite3 *db = NULL;//数据库指针 
int result;//数据库操作结果 
sqlite3_stmt * stmt = NULL;//动态执行数据库 
char *errmsg = NULL;//执行错误信息 

char sql[100] = "create table Cmdtable(ID,name,command,parsetype,startadder,datanum,keep,quotaId,mode)";



int speed_arr[] = {  B115200, B57600, B38400, B19200, B9600, B4800,
		    B2400, B1200};
int name_arr[] = {115200, 57600, 38400,  19200,  9600,  4800,  2400, 1200};

#define debugnum(data,len,prefix)  \
{ \
        unsigned int i;   \
        for (i = 0;i < len;i++) { \
                if(prefix)  \
                        printf("0x%02x ",data[i]); \
                else  \
                        printf("%02x ",data[i]); \
        } \
}

#define PORT 80
#define BUFSIZE 2048
//net
int sockfd;
//int flag_up_down = 0;  //up = 1; down =0;

int parse_response(char *buffer){ 
	char *p,*q;
  	int i;
	cJSON *data, *json,*code,*data_arr,*arr_item;
	int arr_size;
	const char *error_ptr;
    char temp_datBits[10],temp_decimalNum[10];

   //printf("%s\n",buffer); 
   	p=strchr(buffer,'{'); 
	q = strrchr(p,'}');

    q[1] = '\0';

  	printf("\njson:%s\n",p); 

 	//从缓冲区中解析出JSON结构
    json = cJSON_Parse(p);

    //判断cJSON_Parse函数返回值确定是否打包成功
     if (!json)
     {
       printf("\njson error\n");

        error_ptr = cJSON_GetErrorPtr();
       
        printf("\nError before: %s\n", error_ptr);

    }
    else
    {//打包成功
      
        printf("\njson ok\n");
  
        code= cJSON_GetObjectItem(json, "code");
        if(!code) 
        {
            printf("\nNo code !\n");       
        }
        else if(code->valueint != 200)
		{
           // printf("recv error ");
 			//printf("code type is %d\n",code->type);
            printf("\nrecv error code is %d\n",code->valueint);
		}
        else//recv ok
        {
			printf("code type is %d\n",code->type);
            printf("\nrecv error code is %d\n",code->valueint);
			data_arr = cJSON_GetObjectItem(json,"data");

			if(!data_arr) 
        	{
            	printf("\nNo data !\n");       
        	}
        	else
        	{
           		//printf("data type is %d\n",data_arr->type);
          	    //printf("\ndata is %s\n",data_arr->valuestring);
            	arr_size = cJSON_GetArraySize(data_arr);
            	arr_item = data_arr->child;
        
       		 //循环获取数组下每个字段的值并使用cJSON_Print打印
    			strcpy(sql,"insert into Cmdtable (ID,name,command,parsetype,startadder,datanum,keep,quotaId,mode) values (?,?,?,?,?,?,?,?,?)");
				result = sqlite3_prepare(db,sql,-1,&stmt,NULL); //准备好SQL语句
            
            	if (result != SQLITE_OK)
				{
                	printf("\nprepare fail \n");
					//continue;
        		}
           // printf("////%d\n",arr_size);
				if(arr_size > 0)
				{
					sqlite3_exec(db,"delete from Cmdtable",0,0,&errmsg);
				}
                
            	for(i = 0;i <arr_size;i++)
            	{
                    
					data = cJSON_GetObjectItem(arr_item,"deviceId");
                	sqlite3_bind_text(stmt,1,data->valuestring,32,NULL);

					data = cJSON_GetObjectItem(arr_item,"type");
   					sqlite3_bind_text(stmt,2,data->valuestring,32,NULL);

					data = cJSON_GetObjectItem(arr_item,"queryConfiguration");
   					sqlite3_bind_text(stmt,3,data->valuestring,32,NULL);

					data = cJSON_GetObjectItem(arr_item,"dataType");
   					sqlite3_bind_text(stmt,4,data->valuestring,32,NULL);

					data = cJSON_GetObjectItem(arr_item,"adddress");
   					sqlite3_bind_text(stmt,5,data->valuestring,32,NULL);

				
					data = cJSON_GetObjectItem(arr_item,"datsBits");
					sprintf(temp_datBits,"%d",data->valueint);
   					sqlite3_bind_text(stmt,6,temp_datBits,32,NULL);

					data = cJSON_GetObjectItem(arr_item,"decimalNum");
					sprintf(temp_decimalNum,"%d",data->valueint);
   					sqlite3_bind_text(stmt,7,temp_decimalNum,32,NULL);

					data = cJSON_GetObjectItem(arr_item,"quotaId");
					//sprintf(temp,"%d",data->valueint);
   					sqlite3_bind_text(stmt,8,data->valuestring,32,NULL);

					data = cJSON_GetObjectItem(arr_item,"portType");
                	sqlite3_bind_text(stmt,9,data->valuestring,32,NULL);

    				result = sqlite3_step(stmt); //执行语句

					if(result == SQLITE_DONE){   //执行结果
                		result = SQLITE_OK;
        			}
   
                	arr_item = arr_item->next;//下一个子对象
           	 }//for
         	//
          
       	 }//else
           
        }  
        
    }
    if(json)
    {
       cJSON_Delete(json);
       printf("\njson delete\n"); 
    }
      printf("\nparss ok\n");

   return 0; 
}

//hostname to ip
int hostname_to_ip(char * hostname , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
         
    if ( (he = gethostbyname( hostname ) ) == NULL) 
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }
 
    addr_list = (struct in_addr **) he->h_addr_list;
     
    for(i = 0; addr_list[i] != NULL; i++) 
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
     
    return 1;
}

void connect_server()
{
	
    struct sockaddr_in serveraddr;
    char str1[5000],str2[4000],buf[BUFSIZE],str[100];
	socklen_t len;
	int ret,h,i,nrow,ncol;
    fd_set t_set1;
    struct timeval tv;

    char ip[100],*p,**table;

    hostname_to_ip("test-collection.ycxz-china.com" , ip);
    printf("\n%s resolved to %s\n" , "test-collection.ycxz-china.com" , ip);

    

    if((sockfd=socket(AF_INET, SOCK_STREAM,0))<0){
        printf("\ncreate network connection failed, socket error");
        flag_net = 0;
    }

    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(PORT);
    if(inet_pton(AF_INET,ip,&serveraddr.sin_addr)<=0){
        printf("\ninet_pton error!\n");
         flag_net = 0;
    }

    if(connect(sockfd,(struct sockaddr*)&serveraddr, sizeof(serveraddr))<0){
        printf("\nconnect error!\n");
         flag_net = 0;
    }
    printf("connection established!\n");
 	flag_net = 1;
    if(flag_down == 1 && flag_up == 0){//down
    	//memset(str2,'\0',4096);
    	strcpy(str2,"deviceId=");

		strcpy(sql,"select * from devicetable");
		sqlite3_get_table(db,sql,&table,&nrow,&ncol,&errmsg);
		//printf("...........%s\n",table[0]);			
        //printf("...........%s\n",table[1]);
		strcat(str2,table[1]);
		//printf("///////////deviceID:%s",devicesID);
    	len=strlen(str2);
    	sprintf(str,"%d",len);
    
    	//memset(str1,0,4096);
    	strcpy(str1, "POST /data/deviceInfo HTTP/1.1\n");
    	strcat(str1, "HOST: test-collection.ycxz-china.com\n");
    	strcat(str1, "Content-Type: application/x-www-form-urlencoded\n");
    	strcat(str1, "Content-Length: ");
    	strcat(str1, str);
    	strcat(str1,"\n\n");
    	//str2的值为post的数据
    	strcat(str1, str2);
    	strcat(str1, "\r\n\r\n");
    	printf("\n%s\n\n",str1);
		ret=write(sockfd,str1,strlen(str1));
    	if(ret<0)
		{
        	printf("\nsend failed\n");
    	}
		else
		{
        	printf("\nsend success\n");
    	}
	}
    else if(flag_down == 0 && flag_up == 1)//up
	{
    	memset(str2,'\0',1024);
    	strcat(str2,"collectData=[{");
		strcat(str2,"deviceId:");

		//getdata
		strcpy(sql,"select * from Datatable");
		sqlite3_get_table(db,sql,&table,&nrow,&ncol,&errmsg);
        
		if(nrow == 0)
		{
			printf("\nDatatable is null\n");
     		return;
		}
		printf("\nrow:%d col:%d\n",nrow,ncol);
	 	printf("\nThe result of querying is:\n");

		memset(str1,0,2048);
    	strcat(str1, "POST /data/collect_all HTTP/1.1\n");
    	strcat(str1, "HOST: test-collection.ycxz-china.com\n");
    	strcat(str1, "Content-Type: application/x-www-form-urlencoded\n");
    	strcat(str1, "Content-Length: ");

       // printf("11111111\n");

        strcpy(str2,"collectData=[");
		for (i = 1; i < nrow+1; i++) {
			printf("%d\n",i);
			sprintf(str,"{\"deviceId\": %s,\"quotaId\":%s,\"value\":%s},",table[i*ncol+0],table[i*ncol+1],table[i*ncol+2]);
			strcat(str2,str);
		}
        
       // p = strrchr(str2,',');
       // *p = ']';
			
		len=strlen(str2);
		str2[len-1] = ']';
    	sprintf(str,"%d",len);
		//printf("str2len:%d\n",len);
    	strcat(str1, str);
    	strcat(str1,"\n\n");
		//str2的值为post的数据
    	strcat(str1, str2);
    	strcat(str1, "\r\n\r\n");
    	printf("\n%s\n\n",str1);

		ret=write(sockfd,str1,strlen(str1));
    	if(ret<0)
		{
        	printf("\nsend failed");
    	}
		else
		{
        	printf("\nsend success");
    	}
    	

	}	
    else
	{
		printf("\nerror!!!!!!!!!!!!!\n");
		flag_up = 0;
		flag_down = 0;
        return;
	}
 
    FD_ZERO(&t_set1);
    FD_SET(sockfd,&t_set1);

    sleep(2);
    tv.tv_sec=0;
    tv.tv_usec=0;
    h=0;
    //printf("++++++++++++++++++\n");
    h=select(sockfd+1, &t_set1,NULL,NULL,&tv);
   // printf("+++++++++++++++++2\n");
    if(h<0){
    	close(sockfd);
    	printf("\nexception1\n");
    	return;
    }
    if(h>0){
    	memset(buf,'\0',1024);
    	i = read(sockfd,buf,1024);
     	buf[i] = '\0';
	// printf("\ni=%d\n",i);
    	if(i == 0){
        	close(sockfd);
            printf("\nexception2\n");
        }
         
        if(flag_down)
		{
			parse_response(buf);
		}
        else
		{
           sqlite3_exec(db,"delete from Datatable",0,0,&errmsg);
	       printf("\nbuf:%s\n",buf);
		}
         
          //  break;
     }
   //	close(sockfd);

    return ;


}

/*将大写字母转换成小写字母*/  
int tolower(int c)  
{  
    if (c >= 'A' && c <= 'Z')  
    {  
        return c + 'a' - 'A';  
    }  
    else  
    {  
        return c;  
    }  
}  
//将十六进制的字符串转换成整数  
int htoi(char s[])  
{  
    int i;  
    int n = 0;  
    if (s[0] == '0' && (s[1]=='x' || s[1]=='X'))  
    {  
        i = 2;  
    }  
    else  
    {  
        i = 0;  
    }  
    for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >='A' && s[i] <= 'Z');++i)  
    {  
        if (tolower(s[i]) > '9')  
        {  
            n = 16 * n + (10 + tolower(s[i]) - 'a');  
        }  
        else  
        {  
            n = 16 * n + (tolower(s[i]) - '0');  
        }  
    }  
    return n;  
} 

// 将str字符以spl分割,存于dst中，并返回子字符串数量
int split(char dst[][80], char* str, const char* spl)
{
    int n = 0;
    char *result = NULL;
    result = strtok(str, spl);
    while( result != NULL )
    {
        strcpy(dst[n++], result);
        result = strtok(NULL, spl);
    }
    return n;
}

void set_speed(int fd, int speed)
{
    int   i;
    int   status;
    struct termios   Opt;
    tcgetattr(fd, &Opt);
    for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)
    {
   	if (speed == name_arr[i])
   	{
   	    tcflush(fd, TCIOFLUSH);
	    cfsetispeed(&Opt, speed_arr[i]);
	    cfsetospeed(&Opt, speed_arr[i]);
	    status = tcsetattr(fd, TCSANOW, &Opt);
	    if (status != 0)
		perror("tcsetattr fd1");
	    return;
     	}
	tcflush(fd,TCIOFLUSH);
    }
}
int set_Parity(int fd,int databits,int stopbits,int parity)
{
    struct termios options;
    if  ( tcgetattr( fd,&options)  !=  0)
    {
  	perror("SetupSerial 1");
  	return(FALSE);
    }
    options.c_cflag &= ~CSIZE;
    switch (databits)
    {
    case 7:
	options.c_cflag |= CS7;
	break;
    case 8:
	options.c_cflag |= CS8;
	break;
    default:
	fprintf(stderr,"Unsupported data size\n");
	return (FALSE);
    }
    switch (parity)
    {
    case 'n':
    case 'N':
	options.c_cflag &= ~PARENB;   
	options.c_iflag &= ~INPCK;   
	break;
    case 'o':
    case 'O':
	options.c_cflag |= (PARODD | PARENB); 
	options.c_iflag |= INPCK;           
	break;
    case 'e':
    case 'E':
	options.c_cflag |= PARENB;     
	options.c_cflag &= ~PARODD;
	options.c_iflag |= INPCK;     
	break;
    case 'S':
    case 's':  
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	break;
    default:
	fprintf(stderr,"Unsupported parity\n");
	return (FALSE);
    }
    switch (stopbits)
    {
    case 1:
	options.c_cflag &= ~CSTOPB;
	break;
    case 2:
	options.c_cflag |= CSTOPB;
	break;
    default:
	fprintf(stderr,"Unsupported stop bits\n");
	return (FALSE);
    }


	options.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
	options.c_oflag &= ~OPOST;
	options.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);

    /* Set input parity option */
    
    if (parity != 'n')
	options.c_iflag |= INPCK;
    options.c_cc[VTIME] = 150; // 15 seconds
    options.c_cc[VMIN] = 0;
    
    
		tcflush(fd,TCIFLUSH); /* Updata the options and do it NOW */
    if (tcsetattr(fd,TCSANOW,&options) != 0)
    {
	perror("SetupSerial 3");
	return (FALSE);
    }
    return (TRUE);
}
/**
  * 函数功能: 十六进制字节组转换为float
  * 输入参数: 字节组
  * 返 回 值: float
  * 说    明：无
  */
float BitToFloat(unsigned char *bit)
{
    unsigned char m0 = bit[0];
    unsigned char m1 = bit[1];
    unsigned char m2 = bit[2];
    unsigned char m3 = bit[3];
    
    // 求符号位
    float sig = 1.;
    if (m0 >=128.)
        sig = -1.;
   // printf("sig %f\n",sig);

    //求阶码
    float jie = 0.;
        if (m0 >=128.)
        {
            jie = m0-128.;
        }
        else
        {
            jie = m0;
        }
    jie = jie * 2.;
    if (m1 >=128.)
        jie += 1.;
    //printf("jie %f\n",jie);
    jie -= 127.;
    //printf("jie %f\n",jie);
    //求尾码
    float tail = 0.;
    if (m1 >=128.)
        m1 -= 128.;
    tail =  m3 + (m2 + m1 * 256.) * 256.;
    //printf("tail %f\n",tail);
    tail  = (tail)/8388608;   //   8388608 = 2^23
    //printf("tail %f\n",tail);
    float f = sig * pow(2., jie) * (1+tail);
    return f;
}
/**
  * 函数功能: 十六进制字节组转换为int
  * 输入参数: 字节组
  * 返 回 值: int
  * 说    明：无
  */
int BitToInt(unsigned char *bit)
{
    int a = bit[0]<<24;
  	int b = bit[1]<<16;
  	int c = bit[2]<<8;
  	a = a + b + c + bit[3]; 
  	
  	return a;
  
}


void receivethread(void)
{
  	int nread,i;
   

    while(1)
	{     
		usleep(200);
        if(!flag_collec)
		{
 			continue;
		}
		if((nread = read(fd,buff,30))>0) {
	    	printf("\n[RECEIVE] Len is %d,content is :\n",nread);
	    	buff[nread]='\0';
			i = 0;
       		// puts(buff);
			while(i < nread){
				printf("%02x ",buff[i]);
				i++;
			}
        	printf("\n");

			flag_rec = 1;
   		}
   		else
		{
			printf("\nreceive error!\n");
			flag_rec = 0;
		} 
	}
 	return;
}

void collecdata(char* cmd)
{

    char dst[20][80];
    int n,i;
    int cnt;
	unsigned char command[50] = {0};
    printf("\ncollection cmd:%s\n",cmd);
	 if(strlen(cmd)>0){
		cnt = split(dst, cmd, ",");
   		for (i = 0; i < cnt; i++){
    		n = htoi(dst[i]);
    		command[i] = (char)n;
		}
		write(fd, command, cnt); 
		//flag_collec = 1;
		printf("\nSend num:%d\n ",cnt);
		usleep(100);
		//receivethread();  
		
	  }   
}

float movedigit(const int n,const int num)
{
	float f = (float)n;
    printf("......%.2f\n",f);
    switch(num)	
	{
		case 0:
			f = f * 0.01;
			break;
		case 1:
			f = f * 0.1;
			break;
		case 2:
			f = f * 0.01;
			break;
		case 3:
			f = f * 0.001;
			break;
		case 4:
			f = f * 0.0001;
			break;
		case 5:
			f = f * 0.00001;
			break;
		case 6:
			f = f * 0.000001;
			break;
		case 7:
			f = f * 0.0000001;
			break;
		case 8:
			f = f * 0.00000001;
			break;
		case 9:
			f = f * 0.000000001;
			break;
		default:
			printf("num erro!\n");
			break;

	}
   
	return f;

}

void collectionthread(void)
{
	int nread,i;
	int nrow = 0, ncol = 0;
	char **table;
	char bit[10];
	int start,num,keep;
    float vaule_float,vaule;
	int vaule_int;
	char buf[10];
    char id[20],quotaId[50]; 
	
	while(1)    
 	{ 
		sleep(2);
		//printf("///////////deviceID:%s",devicesID);
		if(flag_up || flag_down)
		{
			printf("\nup or down!\n");
			continue;
			
		}
        flag_collec = 1;
		printf("\ncollection\n");
        //getdata
		strcpy(sql,"select * from Cmdtable");
		sqlite3_get_table(db,sql,&table,&nrow,&ncol,&errmsg);
		printf("\nSelect row:%d col:%d\n",nrow,ncol);
	 //	printf("The result of querying is:\n");
		for (i = 1; i < nrow+1; i++) {
			//time(&timep); 
			strcpy(id,table[i*ncol+0]);
           
      		strcpy(quotaId,table[i*ncol+7]); 
            //printf("\n%s\n",table[i*ncol+2]);
			collecdata(table[i*ncol+2]);
			//analysis
            
            if(flag_rec)
			{
				
				start = atoi(table[i*ncol+4]);
				num = atoi(table[i*ncol+5]);
				keep = atoi(table[i*ncol+6]);
               // strcpy(mode,table[i*ncol+8]);

				printf("\nstart-adder:%d\n",start);
				printf("\nnum:%d\n",num);
				printf("\nkeep:%d\n",keep);
               // int j = 0;
             	/*while(j < 7){
					printf("%02x ",buff[j]);
					j++;
				}*/
				//printf("\n..........%s\n",table[i*ncol+3]);
					
                strcpy(sql,"insert into Datatable (ID,quotaId,vaule) values (?,?,?)");
				result = sqlite3_prepare(db,sql,-1,&stmt,NULL); //准备好SQL语句
				sqlite3_bind_text(stmt,1,id,32,NULL); //绑定参数
				sqlite3_bind_text(stmt,2,quotaId,32,NULL);

				if (result != SQLITE_OK){
                	printf("\nprepare fail \n");
					continue;
        		}
				//printf();
				if(!strcmp(table[i*ncol+3],"Float"))
				{
					if(!strcmp(table[i*ncol+8],"big"))
					{
						bit[2] = buff[start-1];
                		bit[3] = buff[start];
 						bit[0] = buff[start + 1];
 						bit[1] = buff[start + 2];

					}
					else
					{
						bit[0] = buff[start-1];
                		bit[1] = buff[start];
 						bit[2] = buff[start + 1];
 						bit[3] = buff[start + 2];
					}
					
					vaule_float = BitToFloat(bit);	

					sprintf(buf, "%.2f", vaule_float);

					//save_data
             		printf("\nvaule-float:%.2f\n",vaule_float);
    				sqlite3_bind_text(stmt,3,buf,32,NULL);
    				result = sqlite3_step(stmt); //执行语句
					//return;
                    //printf("ahdgajkdh\n");
				}
				else if(!strcmp(table[i*ncol+3],"Integer"))
				{
					if(num == 2)
					{
						bit[0] = 0x00;
                		bit[1] = 0x00;
						bit[2] = buff[start - 1];
 						bit[3] = buff[start];
					
					}
					else
					{
						bit[0] = buff[start-1];
                		bit[1] = buff[start];
 						bit[2] = buff[start + 1];
 						bit[3] = buff[start + 2];
					}
		    		
                  
					vaule_int = BitToInt(bit);
					
					vaule = movedigit(vaule_int,keep);
					//vaule = movedigit(156,2);
					printf("\nvaule-int:%d\n",vaule_int);
					printf("\nvaule-int:%.2f\n",vaule);

					sprintf(buf, "%.2f", vaule);
					
					sqlite3_bind_text(stmt,3,buf,32,NULL);
    				result = sqlite3_step(stmt); //执行语句
				}
				else
				{
					printf("\nNOt compatibility\n");
					continue;
				}
				//插入一条记录
				if(result == SQLITE_DONE){   //执行结果
                	result = SQLITE_OK;
        		}
                flag_rec = 0;
				//flag_up = 1;
                flag_data = 1;
			}//if
			else
			{
				printf("\nbuff is null!");
				continue;
			}
			printf("\ncollection OK!\n");
            flag_collec = 0;
		}//for
		
   }//while 
 
 	return;
}

void getdatathread(void)
{
	int i,j;
	int nrow = 0, ncol = 0;
	char **table;
	
	while(1)    
 	{ 
		sleep(60);
		printf("\ngetdata\n");	
		if(!flag_up && flag_net)
		{
			flag_down = 1;
            connect_server();
			flag_down = 0;
		}
        else if(flag_net)
		{
			printf("\ngetdata error!\n");
		}
		else
		{
			printf("\nUping!\n");
			sleep(5);
			flag_down = 1;
            connect_server();
			flag_down = 0;
		}

     }
 
 	return;
}


void updatathread(void)
{

	
	while(1)    
 	{ 
		sleep(60);
		printf("\nUpdata\n");	
        if(!flag_down && flag_data && flag_net)
		{
			flag_up = 1;	
			printf("\nstart Updata\n");		
			connect_server();
			flag_up = 0;
		}
		else if(!flag_data)
		{
			printf("\ndata is null!\n");
		}
		else
		{
			printf("\nDowning!\n");
			sleep(5);
			flag_up = 1;	
			//printf("Updata\n");		
			connect_server();
			flag_up = 0;
		}
		
   } 
    
 	return;
}


int main(int argc, char *argv[])
{
    pthread_t updatapthread,collectionpthread,getdatapthread,receivpethread;
    char devicesID[50] ;
	/* 	
		./uart_test 23454115(deviceID)
	*/
	if (argc < 2) {
		printf("Useage: %s deviceID\n", argv[0]);
		exit(0);
    } 

    strcpy(devicesID,argv[1]);
    
    printf("\ndevicesID:%s\n",argv[1]);
	printf("xx-test\n");
	result = sqlite3_open("mdatabase.db",&db);  //打开数据库mdatabase.db，如果不存在就创建它
	if(result != SQLITE_OK)
	{
		printf("Can't open database\n%s\n",sqlite3_errmsg(db));
		return -1;
	}
	result = sqlite3_exec(db,sql,0,0, &errmsg);
	//创建一个表，表名Cmdtable
	if(result != SQLITE_OK)
	{
		printf("warning:Create table fail! May table already.\n");
	}
    strcpy(sql,"create table Datatable(ID,quotaId,vaule)");
	result = sqlite3_exec(db,sql,0,0, &errmsg);
	//创建一个表，表名Datatable
	if(result != SQLITE_OK)
	{
		printf("warning:Create table fail! May table already.\n");
	}

	sqlite3_exec(db,"delete from devicetable",0,0,&errmsg);
	strcpy(sql,"create table devicetable(ID)");
	result = sqlite3_exec(db,sql,0,0, &errmsg);
	//创建一个表，表名devicetable
	if(result != SQLITE_OK)
	{
		printf("warning:Create table fail! May table already.\n");
	}
	strcpy(sql,"insert into devicetable (ID) values (?)");
	result = sqlite3_prepare(db,sql,-1,&stmt,NULL); //准备好SQL语句
            
    if (result != SQLITE_OK)
	{
        printf("prepare fail \n");
	}
	sqlite3_bind_text(stmt,1,devicesID,32,NULL);
    result = sqlite3_step(stmt); //执行语句
    


    fd = open("/dev/ttyO3", O_RDWR);
	set_speed(fd,9600); 
	set_Parity(fd,8,1,'N'); 
	
	pthread_create(&updatapthread,NULL,(void*)updatathread,NULL);//
	pthread_create(&collectionpthread,NULL,(void*)collectionthread,NULL);//
	pthread_create(&getdatapthread,NULL,(void*)getdatathread,NULL);//
	pthread_create(&receivpethread,NULL,(void*)receivethread,NULL);//

	//flag_down = 1;
   // flag_up = 0;
	connect_server();
    flag_down = 0;
	
	

    while(1)
    {
	  sleep(100);	
	}   
      	      
    close(fd);
	close(sockfd);
	sqlite3_close(db); //关闭数据库
    exit(0);
}
