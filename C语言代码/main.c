/************************************************************
	Copyright (C), 2013-2099, Zhuhai RaySharp Technology Co., Ltd.
	
	FileName: main.c
	Author:        Version :          Date:
	Description:    
	Version:         
	Function List:  
	History:         
    <author>      <time>     <version >     <desc>
    huangguoju	  2018/03/06                添加SATVISION客户IPEYE功能
                  2018/11/23                在第三方提供的最新源程序上修改
************************************************************/
// makefile :
// rm IpEyeServer;arm-hisiv300-linux-gcc main.c  -lpthread -o IpEyeServer -Wall  编译指令
// rm test;gcc main.c -o test -lpthread -g
// valgrind --leak-check=full ./test   内存泄漏检测工具

#include    <stdio.h>
#include    <string.h>
#include    <sys/socket.h>
#include    <arpa/inet.h>
#include    <unistd.h>
#include    <stdlib.h>
#include    <pthread.h>
#include    <fcntl.h>
#include    <errno.h>
#include    <sys/file.h>

#if defined(__APPLE__) && defined(__MACH__)
#define PLATFORM_NAME 1
#include    <sys/types.h>
#include    <net/if_dl.h>
#include    <ifaddrs.h>
#else
#define PLATFORM_NAME 2
#include    <sys/ioctl.h>
#include    <net/if.h>
#endif


struct arg_struct {
    int* arg1;
    int* arg2;
    int* arg3;
};

enum {
    SERVER = 0,
    CAMERA = 1
};

#define PRINT(msg, ...) \
    fprintf(stderr, "[%s]F:%s, L:%d, "  msg, "IPEYE", __func__, __LINE__, ##__VA_ARGS__)

typedef struct thread {
    pthread_t thread_id;
    int       thread_num;
    char*     CloudServer;
    int*      CloudPort;
    char*     CloudID;
    char*     CameraIP;
    int       CameraPort;
    char*     Login;
    char*     Password;
    char*     Uri;
    char*     Model;
    char*     Vendor;
    char*     DevKey;
    char*     Status;
} ThreadData;

int  count = 1;
int  EnableDubug = 0;
int  EnableDubugVAL = 0;
char JsonTable[4096*32];
char JsonComp[4096*32];
char HtmlOut[256+4096*32];
char HtmlTable[4096*32];
char *ConfigDir = "";

char*   GetCloudID(char* ConfigDir, char* APIServer, int* APIPort, int EnableMAC, char *MACString );
int     GetCloudServerAndPort(char* APIServer, int* APIPort, char* CloudID, char** CloudServer, int* CloudPort, char* Vendor, char* Model);
int     CloudStart(int* sock, char* server_reply, unsigned long server_reply_size, char* CameraIP, int CameraPort);
void *  CloudLoopCamera(void *arguments);
void    CloudLoopServer(void *arguments);
void *  ChLoop(void *arguments);
void    removeChar(char *str, char garbage);
char*   substring(char* str, size_t begin, size_t len);
char*   GetBallancerCloudID(char* APIServer, int* APIPort);
char*   GetFsCloudID(char* ConfigDir);
char *  strndup(const char *__s1, size_t __n);
int     countSubstr(char string[], char substring[]);
void *  StatusSave(void *arguments);
char*   getJsonString(void *arguments);
char *  getHtmlString(void *arguments, char* Vendor, int HTTPCameraMode, char *HTTPLogoText, char *HTTPRegSite, char *HTTPLogoURL, char *HTTPaddURL);
int     SocketClient(char *ip_address,int port);
int 	CheckProcessMutex(void);



#if 0//PLATFORM_NAME == 1
int GetMac(char *macaddrstr) {
    struct ifaddrs *ifap, *ifaptr;
    unsigned char *ptr;
    if (getifaddrs(&ifap) == 0) {
        for(ifaptr = ifap; ifaptr != NULL; ifaptr = (ifaptr)->ifa_next) {
            if (!strcmp((ifaptr)->ifa_name, "en0") && (((ifaptr)->ifa_addr)->sa_family == AF_LINK)) {
                ptr = (unsigned char *)LLADDR((struct sockaddr_dl *)(ifaptr)->ifa_addr);
                sprintf(macaddrstr, "%02x:%02x:%02x:%02x:%02x:%02x", *ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5));
                break;
            }
        }
        freeifaddrs(ifap);
        return ifaptr != NULL;
    } else {
        return 0;
    }
    return 0;
}
#else
int GetMac(char *macaddrstr) {
    int fd;
    struct ifreq ifr;
    char *iface = "eth0";
    unsigned char *mac = NULL;
    memset(&ifr, 0, sizeof(ifr));
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name , iface , IFNAMSIZ-1);
    if (0 == ioctl(fd, SIOCGIFHWADDR, &ifr)) {
        mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
        sprintf(macaddrstr, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    close(fd);
    return mac != NULL;
}
#endif

int CheckProcessMutex() {
    PRINT("Check whether the thread already exists...\n");
	int lock_file = open("/tmp/ipeye_proc.lock", O_CREAT|O_RDWR, 0777);
	if(!lock_file) return 0;
	int rc = flock(lock_file, LOCK_EX|LOCK_NB); //suc:0  fail: -1
	if (rc){
		if (EWOULDBLOCK == errno){
            PRINT("The process has runing,exit...\n");
		}
		return 1;
	}
	//close(lock_file); // when close file, handle will free 
    PRINT("Check ok\n");
	return 0;
}

//main
int GetUUIDEZ(char *uuid) {
    FILE *fp;
    char path[1024];
    fp = popen("prtHardInfo", "r");
    if (fp == NULL) {
        PRINT("Failed to run command \n" );
        exit(1);
    }
    int i = 0;
    while (fgets(path, sizeof(path)-1, fp) != NULL) {
        i++;
        if (i == 2){
            char *tokenr;
            tokenr = strtok( path, "-" );
            while( tokenr != NULL ) {
                if (strstr(tokenr, "CCRR") != NULL) {
                    char *tokenrr;
                    tokenrr = strtok( tokenr, "CCRR" );
                    int is = 0;
                    while( tokenrr != NULL ) {
                        is++;
                        if (is == 3) {
                            removeChar(tokenrr, '\r');
                            removeChar(tokenrr, '\n');
                            removeChar(tokenrr, ' ');
                            removeChar(tokenrr, ' ');
                            PRINT("%s\n", tokenrr);
                            sprintf(uuid, "%s",tokenrr);
                            PRINT("%s\n", uuid);
                            return 1;
                        }
                        tokenrr = strtok( NULL, "CCRR" );
                    }
                }
                tokenr = strtok( NULL, "-" );
            }
        }
    }
    pclose(fp);
    return 0;
}

int main(int argc , char *argv[]) {

	if (CheckProcessMutex()) return 1;

    int x                      = 0;
    int APIPort                = 8111;
    //int CameraPort             = 554;
    char *APIServer            = "171.25.232.2";
    char *Vendor               = "satvision";
    char *DevKey               = "secret_key_demo";
    char *Model                = "";
	int EnableHTTP			   = 1;
	int HTTPDisableAdd		   = 0;
    int HTTPCameraMode         = 0;
	int HTTPPort               = 8282;
    int Sleep                  = 0;
    int EnableEZVIZ            = 0;
    int EnableMAC              = 0;
    char *MACString            = "";
    char *HTTPLogoText         = "IPEYE";
    char *HTTPLogoURL          = "";
    char *HTTPRegSite          = "https://ipeye.ru";
    char *HTTPaddURL           = "http://ipeye.ru/addcamera.php";
    int CloudPort			   = 5511;
    char *CloudServer 		   = "171.25.232.11";

	memset(JsonTable, 0, 4096*32);
	memset(JsonComp, 0, 4096*32);
	memset(HtmlOut, 0, 256+4096*32);
	memset(HtmlTable, 0, 4096*32);

    char *Streams = "rtsp://admin:@127.0.0.1:554/ch1/0,rtsp://admin:@127.0.0.1:554/ch1/1";
    for (x=0; x<argc; x++) {
        if (strstr(argv[x], "-http_disable_add=") != NULL) {
            HTTPDisableAdd  = atoi(substring(argv[x], strlen("-http_disable_add="), strlen(argv[x])-strlen("-http_disable_add=")));
        } else if (strstr(argv[x], "-enable_http=") != NULL) {
            EnableHTTP      = atoi(substring(argv[x], strlen("-enable_http="), strlen(argv[x])-strlen("-enable_http=")));
        } else if (strstr(argv[x], "-enable_ezviz=") != NULL) {
            EnableEZVIZ     = atoi(substring(argv[x], strlen("-enable_ezviz="), strlen(argv[x])-strlen("-enable_ezviz=")));
        } else if (strstr(argv[x], "-enable_debug=") != NULL) {
            EnableDubug     = atoi(substring(argv[x], strlen("-enable_debug="), strlen(argv[x])-strlen("-enable_debug=")));
        } else if (strstr(argv[x], "-enable_mac=") != NULL) {
            EnableMAC       = atoi(substring(argv[x], strlen("-enable_mac="), strlen(argv[x])-strlen("-enable_mac=")));
        } else if (strstr(argv[x], "-http_port=") != NULL) {
            HTTPPort        = atoi(substring(argv[x], strlen("-http_port="), strlen(argv[x])-strlen("-http_port=")));
        } else if (strstr(argv[x], "-sleep=") != NULL) {
            Sleep           = atoi(substring(argv[x], strlen("-sleep="), strlen(argv[x])-strlen("-sleep=")));
        } else if (strstr(argv[x], "-http_reg_site=") != NULL) {
            HTTPRegSite     = substring(argv[x], strlen("-http_reg_site="), strlen(argv[x])-strlen("-http_reg_site="));
            removeChar(HTTPRegSite, '"');
        } else if (strstr(argv[x], "-http_add_url=") != NULL) {
            HTTPaddURL = substring(argv[x], strlen("-http_add_url="), strlen(argv[x])-strlen("-http_add_url="));
            removeChar(HTTPaddURL, '"');
        } else if (strstr(argv[x], "-mac_string=") != NULL) {
            MACString       = substring(argv[x], strlen("-mac_string="), strlen(argv[x])-strlen("-mac_string="));
            removeChar(MACString, '"');
        } else if (strstr(argv[x], "-http_camera_mode=") != NULL) {
            HTTPCameraMode  = atoi(substring(argv[x], strlen("-http_camera_mode="), strlen(argv[x])-strlen("-http_camera_mode=")));
        } else if (strstr(argv[x], "-model=") != NULL) {
            Model           = substring(argv[x], strlen("-model="), strlen(argv[x])-strlen("-model="));
            removeChar(Model, '"');
        } else if (strstr(argv[x], "-vendor=") != NULL) {
            Vendor          = substring(argv[x], strlen("-vendor="), strlen(argv[x])-strlen("-vendor="));
            removeChar(Vendor, '"');
        } else if (strstr(argv[x], "-http_logo_text=") != NULL) {
            HTTPLogoText    = substring(argv[x], strlen("-http_logo_text="), strlen(argv[x])-strlen("-http_logo_text="));
            removeChar(HTTPLogoText, '"');
        } else if (strstr(argv[x], "-http_logo_url=") != NULL) {
            HTTPLogoURL     = substring(argv[x], strlen("-http_logo_url="), strlen(argv[x])-strlen("-http_logo_url="));
            removeChar(HTTPLogoURL, '"');
        } else if (strstr(argv[x], "-config_dir=") != NULL) {
            ConfigDir       = substring(argv[x], strlen("-config_dir="), strlen(argv[x])-strlen("-config_dir="));
            removeChar(ConfigDir, '"');
        } else if (strstr(argv[x], "-dev_key=") != NULL) {
            DevKey          = substring(argv[x], strlen("-dev_key="), strlen(argv[x])-strlen("-dev_key="));
            removeChar(DevKey, '"');
        } else if (strstr(argv[x], "-streams=") != NULL) {
            Streams         = substring(argv[x], strlen("-streams="), strlen(argv[x])-strlen("-streams="));
            removeChar(Streams, '"');
        } else if (x != 0) {
            PRINT("arg not support options %s\n", argv[x]);
        }
    }
    if (EnableDubug == 1) {
        PRINT("Service Started\n");
        PRINT("Vendor: %s \n", Vendor);
    }
    if (Sleep > 0) {
        if (EnableDubug == 1) {
            PRINT("Sleep %i second\n", Sleep);
        }
        sleep(Sleep);
    }
    char CloudIDS[17];
    if (EnableEZVIZ == 1) {
        int res = GetUUIDEZ(CloudIDS);
        if (res == 1) {
            MACString = CloudIDS;
            EnableMAC = 1;
        }else{
            PRINT("Parse EZID error\n");
            exit(1);
        }
    }

    char *CloudID = GetCloudID(ConfigDir, APIServer, &APIPort, EnableMAC, MACString);
    if (EnableDubug == 1) {
        PRINT("Service recive CloudID = %s\n", CloudID);
    }

    GetCloudServerAndPort(APIServer, &APIPort, CloudID, &CloudServer, &CloudPort, Vendor, Model);
    if (EnableDubug == 1) {
        PRINT("Service recive CloudServer = %s CloudPort = %d\n", CloudServer, CloudPort);
        PRINT("Cloud param: APIServer = %s APIPort = %d CloudID = %s CloudServer = %s CloudPort = %d\n", APIServer, APIPort, CloudID, CloudServer, CloudPort);
    }
    count = countSubstr(Streams, ",")+1;
	PRINT(" count: %d\n", count);
    int i = 0;
    ThreadData thread[count];
    char *ch = strtok(Streams, ",");
    while (ch != NULL) {
        int succ_parsing = 0;
        char *ip    = calloc(sizeof(char),32);
        char *user  = calloc(sizeof(char),32);
        char *pass  = calloc(sizeof(char),32);
        int port    = 554;
        char *page  = calloc(sizeof(char),32);
		
        if (sscanf(ch, "rtsp://%99[^:]:%99[^@]@%99[^:]:%i/%199[^\n]", user, pass, ip, &port, page) == 5) {
            succ_parsing = 1;
        } else if (sscanf(ch, "rtsp://%99[^:]:@%99[^:]:%i/%199[^\n]", user, ip, &port, page) == 4) {
            succ_parsing = 1;
        } else if (sscanf(ch, "rtsp://%99[^:]:%99[^@]@%99[^/]/%199[^\n]", user, pass, ip, page) == 4) {
            succ_parsing = 1;
        } else  if (sscanf(ch, "rtsp://%99[^:]:%99[^@]@%99[^:]:%i[^\n]", user, pass, ip, &port) == 4) {
            succ_parsing = 1;
        } else if (sscanf(ch, "rtsp://%99[^:]:%i/%199[^\n]", ip, &port, page) == 3) {
            succ_parsing = 1;
        } else if (sscanf(ch, "rtsp://%99[^/]/%199[^\n]", ip, page) == 2) {
            succ_parsing = 1;
        } else if (sscanf(ch, "rtsp://%99[^:]:%i[^\n]", ip, &port) == 2) {
            succ_parsing = 1;
        } else if (sscanf(ch, "rtsp://%99[^\n]", ip) == 1) {
            succ_parsing = 1;
        }
       
        if (succ_parsing == 1) {
            thread[i].thread_num=i;
            thread[i].CloudServer = CloudServer;
            thread[i].CloudPort = &CloudPort;
            thread[i].CloudID = CloudID;
            thread[i].CameraIP = ip;
            thread[i].CameraPort = port;
            thread[i].Model = Model;
            thread[i].Vendor = Vendor;
            thread[i].Login = user;
            thread[i].Password = pass;
            thread[i].Uri = page;
            thread[i].DevKey = DevKey;
            thread[i].Status = "0";
            pthread_create(&(thread[i].thread_id), NULL, ChLoop, (void *)(thread+i));
            i++;
        } else {
            if (EnableDubug == 1) {
                PRINT("Parse Url Error\n");
            }
        }
        ch = strtok(NULL, ",");
    }
    sleep(2);
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, StatusSave, &thread)) {
        if (EnableDubug == 1) {
            PRINT("create thread error\n");
        }
        return 1;
    }
    if (EnableHTTP == 1) {
        int listenfd = 0, connfd = 0;
        struct sockaddr_in serv_addr;
        char sendBuff[4096];

		if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
			if (EnableDubug == 1){
                PRINT("socket error\n");
			}
		}
		bzero(&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(HTTPPort);
        int enable = 1;
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
            if (EnableDubug == 1) {
                PRINT("http setsockopt error\n");
            }
        }
        if (bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            if (EnableDubug == 1) {
                PRINT("http bind error\n");
            }
        }
		if(listen(listenfd, 4096) < 0) {
			if (EnableDubug == 1){
                PRINT("listen error\n");
			}
		}
        while(1) {
            connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
            if (connfd < 0) {
                if (EnableDubug == 1) {
                    PRINT("http accept error\n");
                }
                sleep(1);
                continue;
            }
            recv(connfd , sendBuff , 4096, 0);
            if (strstr(sendBuff, "GET /status/json") != NULL) {
                char * String = getJsonString(&thread);
                char result[256+strlen(String)];
                snprintf(result, sizeof result,"HTTP/1.1 200 OK\r\nServer: Golang\r\nCache-Control: no-cache\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %lu\r\nConnection: Closed\r\n\r\n%s", (unsigned long)strlen(String), String);
                write(connfd,result, strlen(result));
            } else if (HTTPDisableAdd != 1) {
                char * String = getHtmlString(&thread, Vendor, HTTPCameraMode, HTTPLogoText, HTTPRegSite, HTTPLogoURL, HTTPaddURL);
                char result[256+strlen(String)];
                snprintf(result, sizeof result,"HTTP/1.1 200 OK\r\nServer: Golang\r\nCache-Control: no-cache\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %lu\r\nConnection: Closed\r\n\r\n%s", (unsigned long)strlen(String), String);
                write(connfd,result, strlen(result));
            }
			if(close(connfd) != 0){
				if (EnableDubug == 1){
                    PRINT("close(connfd)\n");
				}
			}
        }
    } else {
        while (1) {
            sleep(50);
        }
    }
	free(CloudID);
	free(CloudServer);	
	for (i = 0; i < count; i++) {
		if(pthread_join(thread[i].thread_id, NULL))
            PRINT("pthread_join error\n");
	}
	if(pthread_join(thread_id, NULL))
        PRINT("pthread_join error\n");
	if (EnableDubug == 1){
        PRINT("exit!\n");
    }
    return 0;
}
int SocketClient(char *ip_address,int port) {
    struct sockaddr_in sin;
    int s;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = inet_addr(ip_address);
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) <0) {
        PRINT("socket Err\n");
        return -1;
    }
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) <0) {
        //PRINT("Connection Cloud Server Error Offline\n");
        close(s);
        return -1;
    }
    PRINT("connect success\n");
    return s;
}
void *StatusSave(void *arguments) {
    char patch[256];
	memset(patch, 0, 256);
	char * String = NULL;
	if(strcmp(ConfigDir, ""))
		snprintf(patch, sizeof patch, "%s/status.cloud",  ConfigDir);
	else
    	snprintf(patch, sizeof patch, "%sstatus.cloud",  ConfigDir);
    while (1) {
		String = getJsonString(arguments);
        if (strcmp(String, JsonComp) == 0) {
            //PRINT("Entered strings are equal.\n");
        } else {
			FILE *fp = NULL;
            fp = fopen(patch, "w+");
            if (fp == NULL) {
                continue;
            }
            fputs(String, fp);
            fclose(fp);
			strcpy(JsonComp, String);
        }
        sleep(5);
    }
	pthread_exit(NULL);
}

char * getJsonString(void *arguments) {
    ThreadData *thread  = (ThreadData*)arguments;
    int i;
    for (i = 0; i < count; i++) {
        if (i == 0) {
            sprintf(JsonTable,"{\"chanels\": [\r\n{\"chanel\":\"%i\",\"real_chanel\":\"%s\",\"cloud_id\":\"%s/%i\",\"camera_login\":\"%s\",\"camera_password\":\"%s\",\"cloud_server\":\"%s\",\"cloud_port\":\"%d\",\"cloud_url\":\"rtsp://%s:%s@%s:%d/%s/%i\",\"status\":\"%s\"},\r\n", thread[i].thread_num+1,thread[i].Uri,thread[i].CloudID,thread[i].thread_num+1,thread[i].Login,thread[i].Password,thread[i].CloudServer, *thread[i].CloudPort,thread[i].Login,thread[i].Password,thread[i].CloudServer,*thread[i].CloudPort,thread[i].CloudID,thread[i].thread_num+1,thread[i].Status);
        } else if (i == count-1) {    
            sprintf(JsonTable,"%s{\"chanel\":\"%i\",\"real_chanel\":\"%s\",\"cloud_id\":\"%s/%i\",\"camera_login\":\"%s\",\"camera_password\":\"%s\",\"cloud_server\":\"%s\",\"cloud_port\":\"%d\",\"cloud_url\":\"rtsp://%s:%s@%s:%d/%s/%i\",\"status\":\"%s\"}\r\n]}", JsonTable, thread[i].thread_num+1,thread[i].Uri,thread[i].CloudID,thread[i].thread_num+1,thread[i].Login,thread[i].Password,thread[i].CloudServer, *thread[i].CloudPort, thread[i].Login,thread[i].Password,thread[i].CloudServer,*thread[i].CloudPort,thread[i].CloudID,thread[i].thread_num+1,thread[i].Status);
        } else {
            sprintf(JsonTable,"%s{\"chanel\":\"%i\",\"real_chanel\":\"%s\",\"cloud_id\":\"%s/%i\",\"camera_login\":\"%s\",\"camera_password\":\"%s\",\"cloud_server\":\"%s\",\"cloud_port\":\"%d\",\"cloud_url\":\"rtsp://%s:%s@%s:%d/%s/%i\",\"status\":\"%s\"},\r\n", JsonTable, thread[i].thread_num+1,thread[i].Uri,thread[i].CloudID,thread[i].thread_num+1,thread[i].Login,thread[i].Password,thread[i].CloudServer, *thread[i].CloudPort, thread[i].Login,thread[i].Password,thread[i].CloudServer,*thread[i].CloudPort,thread[i].CloudID,thread[i].thread_num+1,thread[i].Status);
        }
    }
    return JsonTable;
}
char * getHtmlString(void *arguments, char* Vendor, int HTTPCameraMode, char *HTTPLogoText, char *HTTPRegSite, char *HTTPLogoURL, char *HTTPaddURL) {
    ThreadData *thread  = (ThreadData*)arguments;
    char * String = getJsonString(arguments);
    if (HTTPCameraMode == 1) {
        char TEG[256];
        if (HTTPLogoURL != NULL && strlen(HTTPLogoURL) > 5 ) {
          snprintf(TEG, sizeof TEG,"<img src=\"%s\">", HTTPLogoURL);
        }else{
          snprintf(TEG, sizeof TEG,"Cloud IP Camera %s", HTTPLogoText);
        }
        snprintf(HtmlOut, sizeof HtmlOut,"<!DOCTYPE html>\r\n\
<html lang=\"en\">\r\n\
<head>\r\n\
  <meta charset=\"utf-8\">\r\n\
  <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\r\n\
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\r\n\
  <meta name=\"description\" content=\"\">\r\n\
  <meta name=\"author\" content=\"\">\r\n\
  <title>%s CLOUD IP CAMERA</title>\r\n\
  <script src=\"https://code.jquery.com/jquery-1.12.4.min.js\" integrity=\"sha256-ZosEbRLbNQzLpnKIkEdrPv7lOy9C27hHQ+Xp8a4MxAQ=\" crossorigin=\"anonymous\"></script>\r\n\
  <link href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u\" crossorigin=\"anonymous\">\r\n\
</head>\r\n\
<style>\r\n\
body {\
padding-top: 20px;\
padding-bottom: 20px;\
}\
.header,\
.marketing,\
.footer {\
padding-right: 15px;\
padding-left: 15px;\
}\
.header {\
padding-bottom: 20px;\
border-bottom: 1px solid #e5e5e5;\
}\
.header h3 {\
margin-top: 0;\
margin-bottom: 0;\
line-height: 40px;\
}\
.footer {\
padding-top: 19px;\
color: #777;\
border-top: 1px solid #e5e5e5;\
}\
@media (min-width: 768px) {\
.container {\
  max-width: 730px;\
}\
}\
.container-narrow > hr {\
margin: 30px 0;\
}\
.jumbotron {\
border-bottom: 1px solid #e5e5e5;\
}\
.jumbotron .btn {\
padding: 14px 24px;\
font-size: 21px;\
}\
.marketing {\
margin: 40px 0;\
}\
.marketing p + h4 {\
margin-top: 28px;\
}\
@media screen and (min-width: 768px) {\
.header,\
.marketing,\
.footer {\
  padding-right: 0;\
  padding-left: 0;\
}\
.header {\
  margin-bottom: 30px;\
}\
.jumbotron {\
  border-bottom: 0;\
}\
}\
</style>\
<script>\
function add_cloud() {\r\n\
    $(\"#result\").show();\r\n\
    $(\"#result\").empty();\r\n\
    $('#addbutton').attr(\"disabled\", 'true');\r\n\
    var jdata = %s;\r\n\
    var dataToSend = {\r\n\
        'action':'addcamera',\r\n\
        'login':$(\"#login\").val(),\r\n\
        'password':$(\"#password\").val(),\r\n\
        'login_camera':$(\"#clogin\").val(),\r\n\
        'password_camera':$(\"#cpassword\").val(),\r\n\
        'data': JSON.stringify(jdata)\r\n\
    };\
    $.ajax({\r\n\
      type: 'POST',\
      url: '%s',\r\n\
      data: dataToSend,\r\n\
      success: function(data) {\r\n\
        rjdata = JSON.parse(data)\r\n\
        if (rjdata.status == 1) {\r\n\
          $(\"#result\").html(\"<font color='#00cc66'><b>\"+rjdata.message+\"</b></font>\").hide(10000);\r\n\
        }else{\r\n\
          $(\"#result\").html(\"<font color='#ff0066'><b>Ошибка Добавления</b></font>:\" + rjdata.message).hide(5000);\r\n\
        }\r\n\
      },\r\n\
      error: function(data) {\r\n\
        $(\"#result\").html(\"<font color='ff0066'><b>Ошибка Добавления</b></font>:\" + data).hide(5000);\r\n\
      }\r\n\
  });\r\n\
  $(\"#addbutton\").attr(\"disabled\", false);\r\n\
}\r\n\
</script>\
<body>\
  <div class=\"container\">\
    <div class=\"header clearfix\">\
      <h3 class=\"text-muted\">%s</h3>\
    </div>\
    <div class=\"jumbotron\">\
       <h2>Добавлениe камеры в облако</h2>\
       <form>\
        <div class=\"form-group\">\
          <label for=\"login\">Логин от облака</label>\
          <input type=\"text\" class=\"form-control\" id=\"login\" aria-describedby=\"emailHelp\" placeholder=\"Логин от облака\">\
          <small id=\"loginHelp\" class=\"form-text text-muted\">Для регистрации в облаке перейдите по ссылке <a href=\"%s\">%s</a>.</small>\
        </div>\
        <div class=\"form-group\">\
          <label for=\"password\">Пароль от облака</label>\
          <input type=\"password\" class=\"form-control\" id=\"password\" placeholder=\"Пароль от облака\">\
        </div>\
        <div class=\"form-group\">\
          <label for=\"clogin\">Логин от камеры</label>\
          <input type=\"text\" class=\"form-control\" id=\"clogin\" aria-describedby=\"emailHelp\" placeholder=\"Логин от камеры\">\
        </div>\
        <div class=\"form-group\">\
          <label for=\"cpassword\">Пароль от камеры</label>\
          <input type=\"password\" class=\"form-control\" id=\"cpassword\" placeholder=\"Пароль от камеры\">\
        </div>\
        <p><button id=\"addbutton\" type=\"button\" class=\"btn btn-lg btn-success\" onclick=\"add_cloud();\">Добавить в Облако</button></p>\
      </form>\
    <div id=\"result\">\
      </div>\
    </div>\
    <footer class=\"footer\">\
      <p>&copy; <a href=\"%s\">%s</a> Company, Inc.</p>\
    </footer>\
  </div>\
</body>\
</html>",HTTPLogoText,String,HTTPaddURL,TEG,HTTPRegSite,HTTPRegSite,HTTPRegSite,HTTPLogoText);
//1 - тайтл
//2 - json
//3 - url добавления
//4 - картинка или text
//5 - веб сайт
//6 - компания
    } else {
        HtmlTable[0] = '\0';
        int i = 0;
        for (i = 0; i < count; i++) {
            char* status;
            if (strncmp(thread[i].Status, "1", 1) == 0) {
                status = "<td bgcolor=\"#00FFFF\"  class=\"text-center\">Wait</td>";
            } else if (strncmp(thread[i].Status, "2", 2) == 0) {
                status = "<td bgcolor=\"#00FA9A\"  class=\"text-center\">Online</td>";
            } else {
                status = "<td bgcolor=\"#FF4500\" class=\"text-center\">Offline</td>";
            }
            sprintf(HtmlTable,"%s<tr><td>%s/%d</td><td>rtsp://%s:%d/%s</td>%s<td class=\"text-center\"><div id=d%d><button id=b%d type=\"button\" class=\"btn btn-primary\" onclick=add(%d)>Добавить</button></div></td></tr>", HtmlTable, thread[i].CloudID,thread[i].thread_num+1, thread[i].CameraIP, thread[i].CameraPort, thread[i].Uri, status,thread[i].thread_num+1,thread[i].thread_num+1,thread[i].thread_num+1);
        }
        snprintf(HtmlOut, sizeof HtmlOut,"<html lang=\"en\">\r\n\
  <head>\r\n\
  <link href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u\" crossorigin=\"anonymous\">\r\n\
  <script src=\"https://code.jquery.com/jquery-2.2.4.min.js\" integrity=\"sha256-BbhdlvQf/xTY9gja0Dq3HiwQF8LaCRTXxZKRutelT44=\"crossorigin=\"anonymous\"></script>\r\n\
  <script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js\" integrity=\"sha384-Tc5IQib027qvyjSMfHjOMaLkfuWVxZxUPnCJA7l2mCWNIpG9mGCD8wGNIcPD7Txa\" crossorigin=\"anonymous\"></script>\r\n\
  <script src=\"https://cdn.rawgit.com/google/code-prettify/master/loader/run_prettify.js?skin=sunburst\"></script>\r\n\
  <meta charset=\"utf-8\">\r\n\
  <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\r\n\
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\r\n\
  <meta name=\"description\" content=\"\">\r\n\
  <meta name=\"author\" content=\"\">\r\n\
  <title>Добавление потоков в облако</title>\r\n\
  <script>\r\n\
  function add(actionid){\r\n\
    $(\"#b\"+actionid).attr(\"disabled\", true);\r\n\
    jdata = %s\r\n\
    var dataToSend = {\r\n\
        'action':'add',\r\n\
        'login':$(\"#login\").val(),\r\n\
        'password':$(\"#password\").val(),\r\n\
        'data': JSON.stringify(jdata.chanels[actionid-1])\r\n\
    };\r\n\
    $.ajax({\r\n\
      type: 'POST',\r\n\
      url: '%s',\r\n\
      data: dataToSend,\r\n\
      success: function(data) {\r\n\
        jdata = JSON.parse(data)\r\n\
        if (jdata.status == 1) {\r\n\
          $(\"#d\"+actionid).html(jdata.message)\r\n\
        }else{\r\n\
          $(\"#b\"+actionid).attr(\"disabled\", false);\r\n\
          alert(\"Ошибка Добавления \" + jdata.message)\r\n\
        }\r\n\
      },\r\n\
      error: function(data) {\r\n\
        $(\"#b\"+actionid).attr(\"disabled\", false);\r\n\
        alert(\"Ошибка Добавления \" + data)\r\n\
      }\r\n\
    });\r\n\
\r\n\
  }\r\n\
  </script>\r\n\
  <div align=center>\r\n\
  <h2>Добавление потоков в облако</h2>\r\n\
  <form class=\"form-inline\">\r\n\
    <div class=\"form-group\">\r\n\
      <label for=\"login\">Login</label>\r\n\
      <input id=login type=\"text\" class=\"form-control\" id=\"login\" placeholder=\"Login\">\r\n\
    </div>\r\n\
    <div class=\"form-group\">\r\n\
      <label for=\"password\">Password</label>\r\n\
      <input id=password type=\"password\" class=\"form-control\" id=\"password\" placeholder=\"Password\">\r\n\
    </div>\r\n\
  </form>\r\n\
  <table  class=\"table table-striped\" style=\"width: 1000\">\r\n\
  <tr><th>CloudID</th><th>Поток</th><th class=\"text-center\">Статус</th><th class=\"text-center\">Облако</th></tr>\r\n\
  %s</table></div>", String, "http://ipeye.ru/addcamera.php", HtmlTable);
    }
    return HtmlOut;
}
void *ChLoop(void *arguments) {
    ThreadData *readParams  = (ThreadData*)arguments;
    int sock, rs;
    char server_reply[256];
	memset(server_reply, 0, 256);

    while (1) {
        //if (EnableDubug == 1) {
        //    PRINT("connect SocketClient...\n");
        //}
        readParams->Status = "0";
        sock = SocketClient(readParams->CloudServer, *readParams->CloudPort);
        if (sock == -1)  {
            //if (EnableDubug == 1) {
            //    PRINT("SocketClient not create socket\n");
            //}
            sleep(2);
            continue;
        }
        if (EnableDubug == 1) {
            PRINT("uri = /%s ,connect SocketClient success \n", readParams->Uri);
        }
        struct timeval timeout;
        memset(&timeout, 0, sizeof(timeout));
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        if (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
            if (EnableDubug == 1) {
                PRINT("setsockopt SO_RCVTIMEO failed\n");
            }
            if(close(sock) != 0){
                if (EnableDubug == 1){
                    PRINT("close(sock) Err \n");
                }
            }
            sleep(1);
            continue;
        }
        if (setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(timeout)) < 0) {
            if (EnableDubug == 1) {
                PRINT("setsockopt SO_SNDTIMEO failed\n");
            }
            if(close(sock) != 0){
                if (EnableDubug == 1){
                    PRINT("close(sock) Err \n");
                }
            }
            sleep(1);
            continue;
        }
        char result[1024];
		memset(result, 0, 1024); 
        snprintf(result, sizeof result, "REGISTER={\"cloudid\":\"%s/%d\",\"login\":\"%s\",\"password\":\"%s\",\"uri\":\"/%s\",\"model\":\"%s\",\"vendor\":\"%s\",\"devkey\":\"%s\"}", readParams->CloudID,readParams->thread_num+1, readParams->Login, readParams->Password, readParams->Uri, readParams->Model, readParams->Vendor, readParams->DevKey);
        if (EnableDubug == 1) {
            PRINT("Send register ... \n");
			//PRINT("Send server string =  %s\n", result); // secure infomation include passwd
        }
        if (send(sock , result , strlen(result) , 0) < 0) {
            if (EnableDubug == 1) {
                PRINT("Send register failed\n");
            }
            if(close(sock) != 0){
				if (EnableDubug == 1){
                    PRINT("close(sock) Err \n");
				}
			}
            sleep(1);
            continue;
        }
        if (EnableDubug == 1) {
            PRINT("Send register success \n");
        }

        memset(&timeout, 0, sizeof(timeout));
        timeout.tv_sec = 120;
        timeout.tv_usec = 0;
        if (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
            if (EnableDubug == 1) {
                PRINT("setsockopt SO_RCVTIMEO failed\n");
            }
            if(close(sock) != 0){
                if (EnableDubug == 1){
                    PRINT("close(sock) Err \n");
                }
            }
            sleep(1);
            continue;
        }
        if (setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(timeout)) < 0) {
            if (EnableDubug == 1) {
                PRINT("setsockopt SO_SNDTIMEO failed\n");
            }
            if(close(sock) != 0){
                if (EnableDubug == 1){
                    PRINT("close(sock) Err \n");
                }
            }
            sleep(1);
            continue;
        }

        readParams->Status = "1";
        while (1) {
            if (EnableDubug == 1) {
                PRINT("uri = /%s ,wating recv data ... \n", readParams->Uri);
            }
            rs = recv(sock , server_reply , 256, 0);
            if (EnableDubug == 1) {
                PRINT("uri = /%s ,sock recv data success, len:%d \n", readParams->Uri, rs);
            }
            if(rs <= 0){
                if (EnableDubug == 1) {
                    PRINT("Read Task Error\n");
                }
                if(close(sock) != 0){
                    PRINT("close(sock) Err\n");
                }
                break;
            }

            //PRINT("\033[;31m=1234==server_reply:<%s>========================\033[0m\n",server_reply);

            if(strstr(server_reply, "RTSP")){
                readParams->Status = "2";
                if (EnableDubug == 1) {
                    PRINT("Recive start stream command from Server start Cloud <<<<<<<<<<<<<<<<<<<<<<<<\n");
                    PRINT("CloudStart...  uri = /%s\n", readParams->Uri);
                }
                CloudStart(&sock, server_reply, rs, readParams->CameraIP, readParams->CameraPort);
                break;
            } else if (strstr(server_reply, "\r\n\r\n")) {
                PRINT("Recive ACK Packet , uri = /%s\n", readParams->Uri);
                if( send(sock , "ok" , 2 , 0) < 0) {
                    if (EnableDubug == 1) {
                        PRINT("Send OK Error \n");
                    }
                    if(close(sock) != 0){
                        PRINT("close(sock) Err \n");
                    }
                    break;
                }
            } else {
                if (EnableDubug == 1) {
                    PRINT("Read Bad Hello \n");
                    puts(server_reply);
                }
                if(close(sock) != 0){
                    PRINT("close(sock) Err \n");
                }
                break;
            }
            sleep(1);
        }
    }

    if (EnableDubug == 1) {
        PRINT("ChLoop: exit! \n");
    }
    pthread_exit(NULL);
}

char* GetCloudID(char* ConfigDir, char* APIServer, int* APIPort, int EnableMAC, char *MACString ) {
    char* CloudID = "";
    if (EnableMAC == 1) {
        if (strlen(MACString) > 4) {
            CloudID = MACString;
			MACString = NULL;
        } else {
			char macaddrstr[18] = "";
            if (GetMac(macaddrstr)) {
				char *buff = malloc(18);
				memset(buff, 0, 18);
				memcpy(buff, macaddrstr, 18);
                CloudID = buff;
            } else {
                if (EnableDubug == 1) {
                    PRINT("Mac Address != 17 == %s\n", MACString);
                }
                if (EnableDubug == 1) {
                    PRINT("exit!\n");
                }
                exit(EXIT_FAILURE);
            }
        }
    } else {
        CloudID = GetFsCloudID(ConfigDir);
    }
    if (strlen(CloudID) < 8 || strlen(CloudID) > 50) {
        if (EnableDubug == 1) {
            if (strlen(CloudID) == 0) {
                PRINT("CloudID file not found create new UUID\n");
            }else{
                PRINT("CloudID bad < 8 || > 50 CloudID = %s\n", CloudID);
            }
        }
		free(CloudID);
        CloudID = GetBallancerCloudID(APIServer, APIPort);
        char result[256];
		memset(result, 0, 256);
		if(strcmp(ConfigDir, ""))
			snprintf(result, sizeof result, "%s/cloud.id",  ConfigDir);
		else
        	snprintf(result, sizeof result, "%scloud.id",  ConfigDir);
		FILE *fp;
        while (1) {
            fp = fopen(result, "w+");
            if (fp != NULL) {
                break;
            }
            if (EnableDubug == 1) {
                PRINT("Error write new UUID to file to %scloud.id check dir extis and writable -config_dir=/you/patch/\n", ConfigDir);
            }
            sleep(5);
        }
        fputs(CloudID, fp); //CloudID can't be NULL,segmentation fault
        fclose(fp);
		fp = NULL;
    }
    return CloudID;
}
char* GetFsCloudID(char* ConfigDir) {
    char result[256];
	memset(result, 0, 256);
	if(strcmp(ConfigDir, "")) //ConfigDir can't be NULL,Segmentation fault
		snprintf(result, sizeof(result), "%s/cloud.id", ConfigDir); //allow dir//file.c
	else
    	snprintf(result, sizeof(result), "%scloud.id", ConfigDir);
    FILE *fp = NULL;
    int i = 1;
    while (1) {
        fp = fopen(result, "r");
        if (fp != NULL) {
            break;
        }
        if (i >= 10) {
            return strndup("", 1);
        }
        if (EnableDubug == 1) {
            PRINT("Error opening %scloud.id file! wait mount 10 sec ,check you write access to this patch, IF it fist run wait %u\n", ConfigDir, i);
        }
        i++;
        sleep(1);
    }
	char buffer[64];
	memset(buffer, 0, 64);
    int readet = fread(buffer, 1, 64, fp);
	char *buff = strndup(buffer, readet-1);
	fclose(fp);
	if(NULL == buff)
		return strndup("", 1);
	if (EnableDubug == 1){
		PRINT("------GetFsCloudID--------readet:%d buff <%s>\n",readet,buff);
	}
    return buff;
}
char* GetBallancerCloudID(char* APIServer, int* APIPort) {
    int sock;
    struct sockaddr_in server;
    while (1) {
        sock = socket(AF_INET , SOCK_STREAM , 0);
        if (sock == -1)  {
            if (EnableDubug == 1) {
                PRINT("create socket error\n");
            }
            continue;
        }
        struct timeval timeout;
        
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        if (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
            if (EnableDubug == 1) {
                PRINT("setsockopt failed\n");
            }
        }
        if (setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(timeout)) < 0) {
            if (EnableDubug == 1) {
                PRINT("setsockopt failed\n");
            }
        }
        server.sin_addr.s_addr = inet_addr(APIServer);
        server.sin_family = AF_INET;
        server.sin_port = htons(*APIPort);
        if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
            if (EnableDubug == 1) {
                PRINT("Connection to API server fail. Check internet connection to cloud server test ping 8.8.8.8 on board.\n");
            }
            sleep(5);
            if(close(sock) != 0){
				if (EnableDubug == 1){
                    PRINT("close error\n");
				}
			}
            continue;
        }
        char result[512];
        snprintf(result, sizeof result, "GET /balancer/uuid HTTP/1.1\r\nHost: %s\r\nContent-Type: text/xml\r\nContent-Length: 0\r\n\r\n", APIServer);
        if( send(sock , result , strlen(result) , 0) < 0) {
            if (EnableDubug == 1) {
                PRINT("GetCloudServerAndPort Send register failed\n");
            }
            sleep(5);
            continue;
        }
		char server_reply[256];
		memset(server_reply, 0, 256);
        int rs = recv(sock , server_reply , 256, 0);
        if (EnableDubug == 1){
			//puts(server_reply);
        }
        if (rs > 0 && strstr(server_reply, "\"code\":200") != NULL) {
            char *token;
			char *one_ptr = NULL;
			char *two_ptr = NULL;
			char *three_ptr = NULL;
            token = strtok_r( server_reply, "\r\n\r\n", &one_ptr );
            while( token != NULL ) {
                if (strstr(token, "message") != NULL) {
                    char *tokenr;
                    tokenr = strtok_r( token, ",", &two_ptr );
                    while( tokenr != NULL ) {
                        if (strstr(tokenr, "message") != NULL) {
                            char *tokenrr;
                            tokenrr = strtok_r( tokenr, ":", &three_ptr );
                            while( tokenrr != NULL ) {
                                if (strstr(tokenrr, "message") == NULL) {
                                    removeChar(tokenrr, '"');
                                    char *str3 = malloc(strlen(tokenrr) + 1);
                                    strcpy (str3,tokenrr);
                                    return str3;
                                }
                                tokenrr = strtok_r( NULL, ",", &three_ptr );
                            }
                        }
                        tokenr = strtok_r( NULL, ",", &two_ptr );
                    }
                }
                token = strtok_r( NULL, "\r\n\r\n", &one_ptr );
            }
        }
        sleep(5);
    }
    return strndup("", 1);
}

int GetCloudServerAndPort(char* APIServer, int* APIPort,  char* CloudID,  char** CloudServer, int* CloudPort, char* Vendor, char* Model) {
    int sock = 0;
    struct sockaddr_in server;
    char server_reply[256];
	memset(server_reply, 0, 256);
    while (1) {
        sock = socket(AF_INET , SOCK_STREAM , 0);
        if (sock == -1)  {
            if (EnableDubug == 1) {
                PRINT("GetCloudServerAndPort not create socket\n");
            }
            sleep(5);
            continue;
        }
        struct timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        if (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
            if (EnableDubug == 1) {
                PRINT("setsockopt SO_RCVTIMEO failed\n");
            }
        }
        if (setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(timeout)) < 0) {
            if (EnableDubug == 1) {
                PRINT("setsockopt SO_SNDTIMEO failed\n");
            }
        }
        server.sin_addr.s_addr = inet_addr(APIServer);
        server.sin_family = AF_INET;
        server.sin_port = htons(*APIPort);
        if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
            if (EnableDubug == 1) {
                PRINT("create socket error\n");
            }
            if(close(sock) != 0){
				if (EnableDubug == 1){
                    PRINT("close error\n");
				}
			}
            sleep(10);
            continue;
        }
        char result[512];
		memset(result, 0, 512);
        snprintf(result, sizeof result, "GET /balancer/server/%s?vendor=%s&model=%s HTTP/1.1\r\nHost: %s\r\nContent-Type: text/xml\r\nContent-Length: 0\r\n\r\n", CloudID, Vendor, Model, APIServer);
        if( send(sock , result , strlen(result) , 0) < 0) {
            if (EnableDubug == 1) {
                PRINT("send register error\n");
            }
            sleep(5);
            continue;
        }
        int rs = recv(sock , server_reply , 256, 0);
        if (rs > 0 && strstr(server_reply, "\"code\":200") != NULL) {
            char *token = NULL;
			char *one_ptr = NULL;
			char *two_ptr = NULL;
			char *three_ptr = NULL;
			char *four_ptr = NULL;
            token = strtok_r(server_reply, "\r\n\r\n", &one_ptr );
            while( token != NULL ) {
                if (strstr(token, "message") != NULL) {
                    char *tokenr = NULL;
                    tokenr = strtok_r( token, ",", &two_ptr);
                    while( tokenr != NULL ) {
                        if (strstr(tokenr, "message") != NULL) {
                            char *tokenrr = NULL;
                            tokenrr = strtok_r( tokenr, ":", &three_ptr );
                            while( tokenrr != NULL ) {
                                if (strstr(tokenrr, "|") != NULL) {
                                    removeChar(tokenrr, '"');
                                    char *tokenJsonTable = NULL;
                                    tokenJsonTable = strtok_r( tokenrr, "|", &four_ptr );
                                    while( tokenJsonTable != NULL ) {
                                        if (strstr(tokenJsonTable, ".") != NULL) {
                                            char *str3 = malloc(strlen(tokenJsonTable) + 1);
                                            strcpy(str3, tokenJsonTable);
                                            *CloudServer = str3;
                                        } else {
                                            char *str3 = malloc(strlen(tokenJsonTable) + 1);
                                            strcpy (str3, tokenJsonTable);
                                            *CloudPort = atoi(str3);
											free(str3);
                                        }
                                        tokenJsonTable = strtok_r( NULL, "|", &four_ptr );
                                    }
                                    return 0;
                                }
                                tokenrr = strtok_r( NULL, ",", &three_ptr );
                            }
                        }
                        tokenr = strtok_r( NULL, ",", &two_ptr );
                    }
                }
                token = strtok_r( NULL, "\r\n\r\n", &one_ptr );
            }
        }
        sleep(5);
    }
    return 0;
}

int CloudStart(int* sock, char* server_reply, unsigned long server_reply_size, char* CameraIP, int CameraPort) {
    PRINT("CloudStart: Connect to camera...\n");
    int cam_sock = SocketClient(CameraIP, CameraPort);
    if (cam_sock == -1)  {
        if (EnableDubug == 1) {
            PRINT("Connect to Camera Error, please check you stream %s %d\n", CameraIP, CameraPort);
        }
        if(close(*sock) != 0){
            PRINT("close(sock) Err \n");
        }
        return 0;
    }
    if (EnableDubug == 1) {
        PRINT("Socket created\n");
    }

    if (send(cam_sock , server_reply , server_reply_size , 0) <= 0) {
        if (EnableDubug == 1) {
            PRINT("Camera hello write failed\n");
        }
        if(close(cam_sock) != 0){
            PRINT("close(cam_sock) Err \n");
        }
        if(close(*sock) != 0){
            PRINT("close(sock) Err \n");
        }
        return 1;
    }

    struct timeval timeout;
    memset(&timeout, 0, sizeof(timeout));
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt (cam_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        if (EnableDubug == 1) {
            PRINT("cam_sock setsockopt failed\n");
        }
        if(close(cam_sock) != 0){
            PRINT("close(cam_sock) Err \n");
        }
        if(close(*sock) != 0){
            PRINT("close(sock) Err \n");
        }
        return 0;
    }
    if (setsockopt (cam_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(timeout)) < 0) {
        if (EnableDubug == 1) {
            PRINT("cam_sock setsockopt failed\n");
        }
        if(close(cam_sock) != 0){
            PRINT("close(cam_sock) Err \n");
        }
        if(close(*sock) != 0){
            PRINT("close(sock) Err \n");
        }
        return 0;
    }

    memset(&timeout, 0, sizeof(timeout));
    timeout.tv_sec = 60;
    timeout.tv_usec = 0;
    if (setsockopt (*sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        if (EnableDubug == 1) {
            PRINT("sock setsockopt failed\n");
        }
        if(close(cam_sock) != 0){
            PRINT("close(cam_sock) Err \n");
        }
        if(close(*sock) != 0){
            PRINT("close(sock) Err \n");
        }
        return 0;
    }
    if (setsockopt (*sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(timeout)) < 0) {
        if (EnableDubug == 1) {
            PRINT("sock setsockopt failed\n");
        }
        if(close(cam_sock) != 0){
            PRINT("close(cam_sock) Err \n");
        }
        if(close(*sock) != 0){
            PRINT("close(sock) Err \n");
        }
        return 0;
    }

    PRINT("Start Working Chanel Online\n");

    int Camera = CAMERA;
    int Server = SERVER;
    struct arg_struct readParams;
    readParams.arg1 = sock;
    readParams.arg2 = &cam_sock;
    readParams.arg3 = &Camera;
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, CloudLoopCamera, &readParams)) {
        if (EnableDubug == 1) {
            PRINT("not create thread\n");
        }
        if(close(cam_sock) != 0){
            PRINT("close(cam_sock) Err \n");
        }
        if(close(*sock) != 0){
            PRINT("close(sock) Err \n");
        }
        return 1;
    }

    struct arg_struct readParams2;
    readParams2.arg1 = &cam_sock;
    readParams2.arg2 = sock;
    readParams2.arg3 = &Server;
    CloudLoopServer(&readParams2);
    if (EnableDubug == 1) {
        PRINT("waiting for Camera thread exit...\n");
    }
    pthread_join(thread_id, NULL);
    if (EnableDubug == 1) {
        PRINT("Stop Working Chanel Offile \n");
    }
    return 0;
}
/*
socket read and write loop
*/
void *CloudLoopCamera(void *arguments) {
    struct arg_struct *args = arguments;
    if (EnableDubug == 1) {
        PRINT("Camera thread start \n");
    }

    char server_reply[8096];
	memset(server_reply, 0, 8096);
    while (1) {
        int rs = recv(*args->arg1 , server_reply , 8096, 0);
        if ( rs <= 0)  {
            if (EnableDubug == 1) {
                PRINT("Close Read Loop\n");
            }
            break;
        }
        if ( send(*args->arg2 , server_reply , rs , 0) <= 0) {
            if (EnableDubug == 1) {
                PRINT("Close write Loop\n");
            }
            break;
        }
    }
	
    if(close(*args->arg2) != 0)
        PRINT("CAMERA close Camera socket Err\n");
    if(close(*args->arg1) != 0)
        PRINT("CAMERA close Server socket Err\n");
    if (EnableDubug == 1) {
        PRINT("CAMERA thread exit\n");
    }
 
    pthread_exit(NULL);
}

void CloudLoopServer(void *arguments) {
    struct arg_struct *args = arguments;
    if (EnableDubug == 1) {
        PRINT("Server thread start\n");
    }

    char server_reply[8096];
    memset(server_reply, 0, 8096);
    while (1) {
        int rs = recv(*args->arg1 , server_reply , 8096, 0);
        if ( rs <= 0)  {
            if (EnableDubug == 1) {
                PRINT("Close Read Loop\n");
            }
            break;
        }
        if ( send(*args->arg2 , server_reply , rs , 0) <= 0) {
            if (EnableDubug == 1) {
                PRINT("Close write Loop\n");
            }
            break;
        }
    }

    if(close(*args->arg2) != 0)
        PRINT("SERVER close Server socket Err\n");
    if(close(*args->arg1) != 0)
        PRINT("SERVER close Camera socket Err\n");
    if (EnableDubug == 1) {
        PRINT("SERVER thread exit\n");
    }
    return;
}

void removeChar(char *str, char garbage) {
    char *src, *dst;
	//three pointer point to same memory,Unpredictable results will occur when free.
    for (src = dst = str; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != garbage) dst++;
    }
    *dst = '\0';
	src = NULL;
	dst = NULL;
}

char* substring(char* str, size_t begin, size_t len) {
    if (str == NULL || strlen(str) == 0 || strlen(str) < begin || strlen(str) < (begin+len) || len < 0) {
        return NULL;
    }
    return strndup(str + begin, len);
}

int countSubstr(char string[], char substring[]) {
    int subcount = 0;
    size_t sub_len = strlen(substring);
    if (!sub_len) return 0;
	size_t i = 0;
    for (i = 0; string[i];) {
        size_t j = 0;
        size_t count = 0;
        while (string[i] && string[j] && string[i] == substring[j]) {
            count++;
            i++;
            j++;
        }
        if (count == sub_len) {
            subcount++;
            count = 0;
        } else {
            i = i - j + 1; /* no match, so reset to the next index in 'string' */
        }
    }
    return subcount;
}
