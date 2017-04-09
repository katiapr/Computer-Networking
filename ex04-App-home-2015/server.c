//Author: Katia Prigon
//ID: 322088154
//ex4: UDP sockets with Select
#include "slist.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>//use for signals
//#include <sys/time.h>
//#include <sys/types.h>
//#include <sys/select.h>

#define MAX_PORT_NUM 		65535
#define SUCCESS				0
#define FAIL				-1
#define MAX_MESSAGE_SIZE	4096

//----functions-----//
//this func using for catch CTRL+C to stop the program
void signalHandler(int signal);
//this func change all chars to upper
void upper_case(char* str);
//this func check the port validation
//returns SUCCESS - for succes and FAIL - if the port is invalid
int valid_port(char*);
//this func insert each read message to the queue using functionality of slist(ex1)
//returns SUCCESS - on success and FAIL - if anything went wrong
int insert_message(char* arg,slist_t*);
//implementation of this func

int serv_sd = 0;
slist_t* queue = NULL;

void signalHandler(int sign){
	close(serv_sd);
	slist_destroy(queue,SLIST_FREE_DATA);
	exit(EXIT_SUCCESS);
}

int main(int argc,char *argv[])
{
	int		rc;
	int		port;
//	int		serv_sd;
	char 	buff[MAX_MESSAGE_SIZE];

	struct 	sockaddr_in serv_addr;
	struct	sockaddr_in cli_addr;

	int		bytes;
	fd_set	rdfd,wrfd;
	//-----------------------------------//
	if(argc != 2)
	{
		fprintf(stderr,"Usage: server <port>\n");
		exit(EXIT_FAILURE);
	}
	//--------------PORT-------------//
	rc = valid_port(argv[1]);
	if(rc == FAIL)
	{
		fprintf(stderr,"invalid port input\n");
		exit(EXIT_FAILURE);
	}
	port = atoi(argv[1]);
	if(port < 1 || port > MAX_PORT_NUM)
	{
		fprintf(stderr,"invalid port number\n");
		exit(EXIT_FAILURE);
	}

	//---------------SOCKET---------//
	serv_sd = socket(PF_INET,SOCK_DGRAM,0);
	if(serv_sd < SUCCESS)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}
	////socket creation
	memset(&serv_addr,'\0',sizeof(cli_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);


	rc = bind(serv_sd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
	if(rc < SUCCESS)
	{
		perror("bind");
		exit(EXIT_FAILURE);
	}



	memset(&cli_addr,'\0',sizeof(cli_addr));
	socklen_t cli_len = sizeof(cli_addr);
	memset(buff,'\0',sizeof(buff));


	//inits - SLIIST ----///

	queue = (slist_t*)malloc(sizeof(slist_t));
	if(queue == NULL)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	slist_init(queue);



	while(1)
	{
		signal(SIGINT,signalHandler);
		FD_ZERO(&rdfd);
		FD_SET(serv_sd,&rdfd);
		FD_SET(serv_sd,&wrfd);
		//-------reading-----//
		//		if(select(serv_sd+1, &rdfd, 0, 0, 0) < 0)
		//		{
		//			perror("select");
		//			slist_destroy(queue,SLIST_FREE_DATA);
		//			exit(EXIT_FAILURE);
		//		}
		if(select(serv_sd+1,&rdfd,&wrfd,0,0) < 0)
		{
			perror("select");
			slist_destroy(queue,SLIST_FREE_DATA);
			exit(EXIT_FAILURE);
		}

		if(FD_ISSET(serv_sd, &rdfd))
		{
			printf("server is ready to read\n");
			//
			bytes = recvfrom(serv_sd,buff,sizeof(buff),0,(struct sockaddr*)&cli_addr,&cli_len);
			if(bytes < SUCCESS)
			{
				slist_destroy(queue,SLIST_FREE_DATA);
				perror("recvfrom");
				exit(EXIT_FAILURE);
			}
			if(bytes == SUCCESS)
			{
				memset(buff,'\0',sizeof(buff));
				continue;
			}
			upper_case(buff);
			//insertion message to queue
			if(insert_message(buff,queue) == FAIL)
			{
				slist_destroy(queue,SLIST_FREE_DATA);
				exit(EXIT_FAILURE);
			}

		}

		//-------writting------//

		//send:
		if(slist_size(queue))
		{
			if(FD_ISSET(serv_sd, &wrfd))
			{

				printf("server is ready to write\n");
				char* message = slist_pop_first(queue);

				bytes = sendto(serv_sd,message,strlen(message),0,(struct sockaddr*)&cli_addr,cli_len);
				if(bytes < 0)
				{
					perror("sendto");
					slist_destroy(queue,SLIST_FREE_DATA);
					exit(EXIT_FAILURE);
				}
			}
		}
		FD_ZERO(&wrfd);

	}
	exit(EXIT_SUCCESS);
}

int valid_port(char* str)
{
	int i;
	for(i = 0; i< strlen(str);i++)
	{
		if(isdigit(str[i]) == FAIL)
			return FAIL;
	}
	return SUCCESS;
}

void upper_case(char* str)
{
	int i = 0;

	if(str == NULL)
	{
		fprintf(stderr,"buffer is NULL\n");
		return;
	}
	char temp;
	for(i = 0; i< strlen(str); i++)
	{
		temp = str[i];
		str[i] = toupper(temp);
	}
}

int insert_message(char* arg, slist_t* q)
{
	if(arg == NULL || q == NULL)
	{
		fprintf(stderr,"one of arguments is NULL\n");
		free(arg);
		return FAIL;
	}
	if(slist_append(q,arg) == FAIL)
	{
		fprintf(stderr,"cannot append list\n");
		free(arg);
		return FAIL;
	}
	return SUCCESS;
}
