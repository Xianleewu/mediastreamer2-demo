#define LOG_TAG "cmd.cgi"
#include <stdio.h>    
#include <log/log.h>
#include <binder/IServiceManager.h>

#include "RemoteBuffer.h"
using namespace rockchip;
#define COMMAND_STRING "command="

void decode(char *src, char *last, char *dest)
{
   for(; src != last; src++, dest++) {
       if(*src == '+')
           *dest = ' ';
       else if(*src == '%') {
           int code;
           if(sscanf(src+1, "%2x", &code) != 1)
               code = ' ';
               *dest = code;
               src +=2;
       }
       else
           *dest = *src;
    }
}

void urldecode(char *p)  
{  
    int i=0;  
    while(*(p+i))  
    {  
       if ((*p=*(p+i)) == '%')  
       {  
        *p=*(p+i+1) >= 'A' ? ((*(p+i+1) & 0XDF) - 'A') + 10 : (*(p+i+1) - '0');  
        *p=(*p) * 16;  
        *p+=*(p+i+2) >= 'A' ? ((*(p+i+2) & 0XDF) - 'A') + 10 : (*(p+i+2) - '0');  
        i+=2;  
       }  
       else if (*(p+i)=='+')  
       {  
        *p=' ';  
       }  
       p++;  
    }  
    *p='\0';  
}  

void process(char *data)
{
    RemoteBufferWrapper service;

    ALOGD("strlen=%d, cmd=%s", strlen(data),data);

    char *buf = new char[strlen(data)];
    memset(buf, 0, strlen(data));
    decode(data, data+strlen(data), buf);
    ALOGD("after decode, strlen=%d, cmd=%s",strlen(buf),buf);

    //urldecode(data);
    //ALOGD("after urldecode, stlen=%d, QUERY_STRING=%s", strlen(data), data);

    //find "command=";
    char *cmd = strstr(buf, COMMAND_STRING);
    if(cmd == NULL)
        return;
    
    String8 result = service.callSync((unsigned char *)cmd+strlen(COMMAND_STRING), strlen(cmd+strlen(COMMAND_STRING)));

    ALOGD("result=%s",result.string());
    printf("%s", result.string());
    delete[] buf;
}

int   main()    
{    
	ALOGD("cmd.cgi enter");
    char   *data;  
    char   *pRequestMethod;

    printf("Content-type: application/json\n\n");

    pRequestMethod = getenv("REQUEST_METHOD");
    if(pRequestMethod==NULL) {
         printf("<p>request = null</p>");
         return 0;
    }
    ALOGD("Request method: %s",pRequestMethod);

    if (strcasecmp(pRequestMethod,"POST")==0) {
        int ContentLength = 0;
        char *p = getenv("CONTENT_LENGTH");
        if (p != NULL)
            ContentLength = atoi(p);

        if(ContentLength <= 0) {
            printf("<p>ContentLength = 0</p>");
            return 0;
        }
        data = new char[ContentLength + 1];

        int i = 0;
        char x;
        while (i < ContentLength) {
            x  = fgetc(stdin);
            if (x == EOF)
                break;
            data[i++] = x;
        }
        data[i] = '\0';
		ALOGD("get data:%s",data);
        process(data);
        delete[] data;
    } else if (strcasecmp(pRequestMethod,"GET")==0) {
        data = getenv("QUERY_STRING");
        if(data != NULL) {
	    ALOGD("get data:%s",data);
            process(data);
        }
	} else {
        ALOGD("Invalid request method: %s",pRequestMethod);
    }
	ALOGD("cmd.cgi exit");
	return 0;
}

 
