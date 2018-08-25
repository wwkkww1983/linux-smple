#include	<stdio.h>//printf
#include	<stdlib.h>//for exit(0)
#include	<sys/types.h>
#include	<time.h>
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
#include 	"net.h"

#define PORT 80
#define BUFSIZE 2048

int parse_response(char *buffer, char *delims){ 
   char *p,*q;
  int i;
   //printf("%s\n",buffer); 
   p=strchr(buffer,'{'); 
  
	q = strrchr(p,'}');

    q[1] = '\0';
  //  p = p - q;
  printf("%s\n",p); 

  //从缓冲区中解析出JSON结构
     cJSON * json= cJSON_Parse(p);

    //判断cJSON_Parse函数返回值确定是否打包成功
     if (!json)
     {
       printf("\njson打包失败\n");

        const char *error_ptr = cJSON_GetErrorPtr();
       
        printf("Error before: %s\n", error_ptr);

    }
    else
    {//打包成功
      
        printf("\njson打包成功\n");
  
        cJSON *code= cJSON_GetObjectItem(json, "code");
        if(!code) 
        {
            printf("No code !\n");       
        }
        else
        {
            printf("code type is %d\n",code->type);
            printf("code is %d\n",code->valueint);
        }
     
        cJSON* data_arr = cJSON_GetObjectItem(json,"data");
            
       
        if(!data_arr) 
        {
            printf("No data !\n");       
        }
        else
        {
            printf("data type is %d\n",data_arr->type);
           // printf("data is %s\n",data_arr->valuestring);
            int arr_size = cJSON_GetArraySize(data_arr);
            cJSON* arr_item = data_arr->child;
        
        //循环获取数组下每个字段的值并使用cJSON_Print打印
            cJSON* device_id = NULL;
            //
            for(i = 0;i <(arr_size-1);i++)
            {
                device_id = cJSON_GetObjectItem(arr_item,"DeviceId");
                printf("DeviceId type is %d\n",arr_item->type);
                printf("DeviceId is %d\n",arr_item->valueint);
                arr_item = arr_item->next;//下一个子对象
               // device_id = cJSON_GetObjectItem(arr_item,"DeviceId");
            }
          
          //  cJSON_Delete(arr_item);
           // printf("\narr_item 释放\n");
             //将JSON结构所占用的数据空间释放
            //cJSON_Delete(data_arr);
           // printf("\ndata_arr 释放\n");
        }
    }
    if(json)
    {
       cJSON_Delete(json);
       printf("\njson 释放\n"); 
    }
      printf("\n解析完成\n");


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


void connect_server(){
    int ret,i,h;
    struct sockaddr_in serveraddr;
    char str1[4096],str2[4096],buf[BUFSIZE],str[128];
    socklen_t len;
    fd_set t_set1;
    struct timeval tv;
    char ip[100];

    hostname_to_ip("test-collection.ycxz-china.com" , ip);
    printf("%s resolved to %s\n" , "test-collection.ycxz-china.com" , ip);

    if((sockfd=socket(AF_INET, SOCK_STREAM,0))<0){
        printf("create network connection failed, socket error");
        exit(0);
    }
    printf("ddfsf1\n");
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(PORT);
    if(inet_pton(AF_INET,ip,&serveraddr.sin_addr)<=0){
        printf("inet_pton error!\n");
        exit(0);
    }
    printf("ddfsf2\n");
    if(connect(sockfd,(struct sockaddr*)&serveraddr, sizeof(serveraddr))<0){
        printf("connect error!\n");
        exit(0);
    }
    printf("connection established!\n");

    memset(str2,'\0',4096);
    strcat(str2,"sum=11");
    //str=(char *)malloc(128);
    len=strlen(str2);
    sprintf(str,"%d",len);
    
    memset(str1,0,4096);
    strcat(str1, "POST /monitor/service HTTP/1.1\n");
    strcat(str1, "HOST: test-collection.ycxz-china.com\n");
    strcat(str1, "Content-Type: application/x-www-form-urlencoded\n");
    strcat(str1, "Content-Length: ");
    strcat(str1, str);
    strcat(str1,"\n\n");
    //str2的值为post的数据
    strcat(str1, str2);
    strcat(str1, "\r\n\r\n");
    printf("%s\n////////\n",str1);
  
    ret=write(sockfd,str1,strlen(str1));
    if(ret<0){
        printf("send failed");
    }else{
        printf("send success");
    }

    FD_ZERO(&t_set1);
    FD_SET(sockfd,&t_set1);
 
    while(1){
        sleep(2);
        tv.tv_sec=0;
        tv.tv_usec=0;
        h=0;
        printf("++++++++++++++++++\n");
        h=select(sockfd+1, &t_set1,NULL,NULL,&tv);
        printf("+++++++++++++++++2\n");
        if(h<0){
            close(sockfd);
            printf("exception1\n");
            return -1;
        }
        if(h>0){
            memset(buf,'\0',4096);
            i=read(sockfd,buf,4095);
            buf[i] = '\0';
			printf("\ni=%d\n",i);
            if(i==0){
               close(sockfd);
               printf("exception2\n");
            }
           // printf("buf:%s\n",buf);
            parse_response(buf, "{");
            break;
        }
    }
    close(sockfd);
    return 0;
}
