//Based on UDP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>

#define BUF 1024
#define TRUE 1
#define FALSE 0
#define PATH "/project/member/member.txt"

#define logi(x) printf(#x": %d\n",x)
#define logs(x) printf(#x": %s\n",x)

void err_h(char *);
void *thread_stuff();
void DEBUG(char *);

typedef struct member{
	char MAC[20];
	char REG[256];
	char out[5];
}aa;

int open_end = TRUE;
int out;

int main(int argc, char *argv[])
{
	off_t loc;
	int serv_sock;
	char msg[BUF];
	char temp[BUF];
	char regid[BUF];
	char system_buf[BUF];
	socklen_t clnt_adr_sz;
	aa family;
	int status=0,str_len=0,cp_count=0,cp_temp=0,fd=0,i,debug=0,thread_id=0,ard_detect=0;
	int exist=FALSE;
	int seek_err=FALSE;
	pthread_t thread;
	
	
	struct sockaddr_in serv_adr, clnt_adr;
	if(argc!=2){
		printf("Usage: %s <port#>\n",argv[0]);
		exit(1);
	}

	serv_sock=socket(PF_INET, SOCK_DGRAM, 0);
	if(serv_sock == -1)
		err_h("UDP Socket creation error");

	memset(&serv_adr,0,sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		err_h("bind() error");

	logi(sizeof(family));

	thread_id=pthread_create(&thread,NULL,thread_stuff,NULL);
	if(thread_id<0)
		err_h("Fail to create thread");	
	while(1)
	{
		clnt_adr_sz=sizeof(clnt_adr);
		memset(msg,0,sizeof(msg));
		memset(temp,0,sizeof(temp));
		memset(regid,0,sizeof(regid));
		str_len=recvfrom(serv_sock,msg,BUF,0,(struct sockaddr*)&clnt_adr,&clnt_adr_sz);//receive message from client
		
		if(str_len>0){
			printf("\nConnected from %s:%d and he says: %s\n",inet_ntoa(clnt_adr.sin_addr),
			ntohs(clnt_adr.sin_port),msg);
		}

	///////////////////////////Arduino////////////////////////////
		if(strncmp(msg,"[ARD_H]",6)==0)	//Arduino hello
		{
			ard_detect=0;
			printf("Arduino hi msg\n");
			memset(msg,0x00,sizeof(msg));

			if(out)
				strcpy(msg,"[SVR]ON");
			else
				strcpy(msg,"[SVR]OFF");
			sendto(serv_sock,msg,strlen(msg),0,(struct sockaddr*)&clnt_adr,clnt_adr_sz);
		}
		else if( (strncmp(msg,"[ARD_D]",6)==0) && ard_detect<1)	//Arduino detect
		{
			printf("Arduino detect message");
			system("python /project/gcm.py");
			sendto(serv_sock,msg,strlen(msg),0,(struct sockaddr*)&clnt_adr,clnt_adr_sz);
			ard_detect++;
		}
		else if(strncmp(msg,"[ARD_N]",6)==0)
		{
			printf("Detect mode hello msg\n");
			ard_detect=0;
		}
	///////////////////////////Arduino////////////////////////////

	///////////////////////////Android////////////////////////////
		else if(strncmp(msg,"[AND_R]",6)==0)	//Android register ->message type  [AND_R]MAC ADDRESS_REGISTRATION ID
		{
			memset((void *)&family,0,sizeof(aa));
			printf("Android msg\n");
			for(cp_count=7,cp_temp=0;cp_count<=23;cp_count++,cp_temp++)
			{
				temp[cp_temp]=msg[cp_count];
				if(cp_count == 23)
					temp[cp_temp+1] = '\0';		
			}
			for(cp_count=25,cp_temp=0;msg[cp_count]!='\0';cp_count++,cp_temp++)
			{
				if(msg[cp_count]=='\0')
					regid[cp_temp]='\0';
				regid[cp_temp]=msg[cp_count];
			}
			fd=open(PATH,O_CREAT|O_RDWR|O_APPEND,S_IRUSR|S_IWUSR);//make txt file. PATH is define value
			open_end = FALSE;
			if(fd<0)
				err_h("open error");
			lseek(fd,0,SEEK_SET);//set cursor head of file
			while((debug=read(fd,(void *)&family,sizeof(family))) == sizeof(family))  //Issue point. search member
			{
				if(strncmp(temp,family.MAC,strlen(temp))==0)
				{
					memset(msg,0x00,sizeof(msg));
					strcpy(msg,"[SVR]EXIST");
					exist=TRUE;
					sendto(serv_sock,msg,strlen(msg),0,(struct sockaddr*)&clnt_adr,clnt_adr_sz);
					close(fd);
					open_end = TRUE;
					break;
				}
				else
				{
					exist=FALSE;
				}	
			}
			if(exist == FALSE)
			{
				int fd2=open("/project/member/regid.txt",O_CREAT|O_RDWR|O_APPEND,S_IRUSR|S_IWUSR);
				int len1;
				char Rtemp[256];

				logs("FALSE STUFF");
				lseek(fd,0,SEEK_END);
				strcpy(family.MAC,temp);
				strcpy(family.REG,regid);
				strcpy(family.out,"IN");
				write(fd,(void *)&family,sizeof(family));
				logs("register:write");

				strcpy(Rtemp,regid);
				len1=strlen(Rtemp);
				Rtemp[len1]='\n';
				Rtemp[len1+1]='\0';
				write(fd2,Rtemp,strlen(Rtemp));
				close(fd2);

				memset(msg,0x00,sizeof(msg));
				strcpy(msg,"[SVR]REGISTER");
				logs(msg);
				sendto(serv_sock,msg,strlen(msg),0,(struct sockaddr*)&clnt_adr,clnt_adr_sz);
				close(fd);
				open_end = TRUE;
			}
		}
		else if(strncmp(msg,"[AND_O]",6)==0)	//Android out alert ->message type  [AND_O]MAC ADDRESS
		{
			logs("android udp thread");
			memset(temp,0,BUF);
			for(cp_count=7,cp_temp=0;cp_count<=23;cp_count++,cp_temp++)
			{
				temp[cp_temp]=msg[cp_count];
				if(cp_count == 23)
					temp[cp_temp+1]='\0';
			}
			for(cp_count=25,cp_temp=0;msg[cp_count]!='\0';cp_count++,cp_temp++)
			{
				if(msg[cp_count]=='\0')
					regid[cp_temp]='\0';
				regid[cp_temp]=msg[cp_count];
			}
			logs(temp);
			fd = open(PATH,O_RDWR,S_IRUSR|S_IWUSR);	//no O_APPEND here
			if(fd<0)
				err_h("file open err");
			open_end = FALSE;
			lseek(fd,0,SEEK_SET);
			while(read(fd,(void *)&family,sizeof(family)) == sizeof(family))
			{
				if(strncmp(temp,family.MAC,strlen(temp))==0)
				{
					int temp_len;
					seek_err=FALSE;
					temp_len = sizeof(family)*-1;	 
					loc=lseek(fd,0,SEEK_CUR);
					loc=lseek(fd,temp_len,SEEK_CUR);
					strcpy(family.MAC,temp);
					strcpy(family.REG,regid);
					strcpy(family.out,"OUT");
					write(fd,(void *)&family,sizeof(family));
					memset(msg,0,sizeof(msg));
					strcpy(msg,"[SVR]OK");
					sendto(serv_sock,msg,strlen(msg),0,(struct sockaddr*)&clnt_adr,clnt_adr_sz);
					close(fd);
					open_end = TRUE;
					break;
				}
				else
				{
					seek_err=TRUE;
					memset(msg,0,sizeof(msg));
					strcpy(msg,"[SVR]ERR");
				}
			}
			if(seek_err)
			{
				sendto(serv_sock,msg,strlen(msg),0,(struct sockaddr*)&clnt_adr,clnt_adr_sz);
				close(fd);
				open_end = TRUE;
			}
		}
		else if(strncmp(msg,"[AND_I]",6)==0)	//Android in
		{
			logs("android udp thread");
			memset(temp,0,BUF);
			for(cp_count=7,cp_temp=0;cp_count<=23;cp_count++,cp_temp++)
			{
				temp[cp_temp]=msg[cp_count];
				if(cp_count == 23)
					temp[cp_temp+1]='\0';
			}
			for(cp_count=25,cp_temp=0;msg[cp_count]!='\0';cp_count++,cp_temp++)
			{
				if(msg[cp_count]=='\0')
					regid[cp_temp]='\0';
				regid[cp_temp]=msg[cp_count];
			}
			logs(temp);
			fd = open(PATH,O_RDWR,S_IRUSR|S_IWUSR);	//no O_APPEND here
			if(fd<0)
				err_h("file open err");
			open_end = FALSE;
			lseek(fd,0,SEEK_SET);
			while(read(fd,(void *)&family,sizeof(family)) == sizeof(family))
			{
				if(strncmp(temp,family.MAC,strlen(temp))==0)
				{
					int temp_len;
					seek_err=FALSE;
					temp_len = sizeof(family)*-1;	 
					loc=lseek(fd,0,SEEK_CUR);
					loc=lseek(fd,temp_len,SEEK_CUR);
					strcpy(family.MAC,temp);
					strcpy(family.REG,regid);
					strcpy(family.out,"IN");
					write(fd,(void *)&family,sizeof(family));
					memset(msg,0,sizeof(msg));
					strcpy(msg,"[SVR]OK");
					sendto(serv_sock,msg,strlen(msg),0,(struct sockaddr*)&clnt_adr,clnt_adr_sz);
					close(fd);
					open_end = TRUE;
					break;
				}
				else
				{
					seek_err=TRUE;
					memset(msg,0,sizeof(msg));
					strcpy(msg,"[SVR]ERR");
				}
			}
			if(seek_err)
			{
				sendto(serv_sock,msg,strlen(msg),0,(struct sockaddr*)&clnt_adr,clnt_adr_sz);
				close(fd);
				open_end = TRUE;
			}
		}
	///////////////////////////Android////////////////////////////
		else// not yet
			sendto(serv_sock,msg,strlen(msg),0,(struct sockaddr*)&clnt_adr,clnt_adr_sz);
	}
	close(serv_sock);
	return 0;
}
void err_h(char *msg)
{
	fputs(msg,stderr);
	fputc('\n',stderr);
	exit(1);
}
void DEBUG(char *msg)
{
	fputs("DEBUG: ",stdout);
	fputs(msg,stdout);
	fputc('\n',stdout);
}
void *thread_stuff()
{
	int fd1,fd2;
	char buff[10];
	aa family;

	out=FALSE;

	while(1)
	{
	if(open_end){
		printf("=*=*=*=*=*Thread start=*=*=*=*=*\n");
		memset(buff,0x00,sizeof(buff));
		fd1=open("/project/member/member.txt",O_RDONLY,S_IRUSR);
		//fd2=open("/project/member/out.txt",O_CREAT|O_RDWR|O_TRUNC,S_IRUSR);
		if(fd1 < 0)
  			printf("member.txt open fail\n");
		//if(fd2 < 0)
			//printf("out.txt open fail\n");
		if( lseek(fd1,0,SEEK_END)==0 )
		{
			close(fd1);
   			strcpy(buff,"IN");
  			//write(fd2,buff,strlen(buff));
  			//close(fd2);
			DEBUG("SEEK_END == 0");
			out=FALSE;
  		}
  		else
		{
			lseek(fd1,0,SEEK_SET);
    			while( read(fd1,(void *)&family,sizeof(family)) == sizeof(family) )
    			{
     				if ( strncmp("IN",family.out,2) == 0 )
     				{
       					close(fd1);
       					//strcpy(buff,"IN");
       					//write(fd2,buff,strlen(buff));
       					//close(fd2);
					DEBUG("Someone in the house");
					out=FALSE;
     				}
     				else
     				{
       					close(fd1);
       					//strcpy(buff,"OUT");
       					//write(fd2,buff,strlen(buff));
       					//close(fd2);
					DEBUG("Everybody out");
					out=TRUE;
     				}
    			}
  		}
  		sleep(3);
	}
		else
			sleep(3.3);

	printf("=*=*=*=*=*Thread end=*=*=*=*=*\n\n");
	}//end of while
}

