/*
 Contains Different Utility Function
*/
#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include<stdlib.h>
#include<stdarg.h>

#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "utility.h"
#include "str.h"


int compute_hash(char *data)
{

	const unsigned int p = 16777619;
        unsigned int hash = 2166136261;
	unsigned int mask=0x00FFFFFF;
 	int i, len;
	len=strlen(data);
	for(i=0; i<len;i++)
		hash = (hash ^ data[i]) * p;
        hash += hash << 13;
        hash ^= hash >> 7;
        hash += hash << 3;
        hash ^= hash >> 17;
        hash += hash << 5;
        
	return hash & mask;
}

char * align_data(char *data)
{
	int i;	
	int len;
	if (strlen(data)%4 == 0)
		return data;
	else
	{
		len=strlen(data);
		for(i=len; i< ((len/4)*4+4);i++)
			data[i]='|'; 
	}
	data[i]='\0';

	return data;
}

char * strToLower(char *str)
	{
		int i;
		for(i=0; str[i]; i++)
			str[i]=tolower(str[i]);

		return str;
	}

char * strrev(const char * string) {
  int length = strlen(string);
  char * result = malloc(length+1);
  if( result != NULL ) {
    int i,j;                                         
    result[length] = '\0';
    for (i=length-1,j=0; i >= 0; i--,j++ )  
         result[j] = string[i];
  }
  return result;
}

unsigned int number_width(unsigned int number)
	{

	unsigned int width=0;
	while(number!=0)
		{
			width++;
			number/=10;
		}
	return width;
	}


char* substring(const char* str, size_t begin, size_t len) 
{ 
  if (str == 0 || strlen(str) == 0 || strlen(str) < begin || strlen(str) < (begin+len)) 
    return 0; 

  return strndup(str + begin, len); 
}

char * lpad( unsigned int num, unsigned int padNumber)
{

 	char *result=malloc(padNumber+1);
        int len=padNumber;	
        while(padNumber>0)
	{
		result[padNumber-1]=(char)(num%10)+48;		
		num=num/10;
		padNumber--;
	}	
        result[len]='\0';	
	return result;
}


char * getLocalTimeStamp(void)
{
	char *timestamp = (char *)malloc(sizeof(char) * 16);
	time_t ltime;
	ltime=time(NULL);
	struct tm *tm;
	tm=localtime(&ltime);
  
	sprintf(timestamp, "%04d%02d%02d%02d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, 
		tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	return timestamp;
}

char * startLogging(char *loggingDir)
{
  struct passwd pd;
  struct passwd* pwdptr=&pd;
  struct passwd* tempPwdPtr;
  char pwdbuffer[200];
  int  pwdlinelen = sizeof(pwdbuffer);
  char logDir[200];
  char logFileName[222];
  char *ret;
  int status;
  struct stat st;
  int isLogDirExists=0;
  char *time=getLocalTimeStamp();

  //printf(" S L : %s \n",loggingDir);

 if(loggingDir!=NULL)
 {
  if( stat( loggingDir, &st)==0)
    {
        if ( st.st_mode & S_IFDIR )
	{
		if( st.st_mode & S_IWUSR)
                  {
			isLogDirExists=1;
			strcpy(logDir,loggingDir);
                  }
                  else printf("User do not have write permission to %s \n",loggingDir);
	}
	else printf("Provided path for %s is not a directory!!\n",loggingDir);
    }
  else printf("Log directory: %s does not exists\n",loggingDir);
  } 
  
  if(isLogDirExists == 0)
  	{
	  if ((getpwuid_r(getuid(),pwdptr,pwdbuffer,pwdlinelen,&tempPwdPtr))!=0)
     		perror("getpwuid_r() error.");
  	  else
  		{
		strcpy(logDir,pd.pw_dir);
		strcat(logDir,"/ospfnLog");
 		//struct stat st;
		// if log directory does not exists create directory with read and write permission in user home directory
		if(stat(logDir,&st) != 0)
			status = mkdir(logDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);	
        	}
	}	
 	//printf(" %s \n",logDir);	
	strcpy(logFileName,logDir);
 	//printf(" lf: %s \n",logFileName);	
  if( logDir[strlen(logDir)-1]!='/')
	      strcat(logFileName,"/");
	//printf(" lf: %s \n",logFileName);
	strcat(logFileName,time);
	//printf(" lf: %s \n",logFileName);
	strcat(logFileName,".log");
	//printf(" lf: %s \n",logFileName);
        ret=(char *)malloc(strlen(logFileName));
        ret=strndup(logFileName,strlen(logFileName));	
       //printf("%s\n",ret); 
        return ret;	
   			


}

void writeLogg(const char  *file, const char *format, ...)
{
    if (file != NULL)
    {
        //printf("%s\n",file); 
        FILE *fp = fopen(file, "a");

        if (fp != NULL)
        {
            
            char *time=getLocalTimeStamp();
	    fprintf(fp,"%s:",time);        
            va_list args;
            va_start(args, format);

            vfprintf(fp, format, args);
            fclose(fp);

            va_end(args);
        }
    }
}

