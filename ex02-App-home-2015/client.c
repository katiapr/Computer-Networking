/*
 * Author:Katia Prigon
 * ID:322088154
 * create on 1/12/2015
 */
//------------------------------//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>//for strings
#include <unistd.h>//using system call functions
#include <time.h>///for time
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h> // for data hostent struct


//------------------------------//
#define SUCCESS 0
#define FAIL -1
#define TRUE 1
#define FALSE 0
#define SHIFTHTTP 7
#define  DEFAULTPORT 80
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
//------------------------------//

//#define TEST_HTTP "GET / HTTP/1.1\r\nhost:www.w3.org\r\n"
//global variables:
time_t now;
char timebuf[128];
int _flagh,_flagd,
_flagURL,_flagWrng,_flagport;
int port;
int url_index;


//----------FUNCTIONS-----------//
/*
 * this func gets the argv[URL]
 * and returns the host name only
 */
char* gethostName(char*);
/*
 * this functions gets the all input from the terminal and checks if the input
 * is 'wrong input' or 'usage' wrong or valid by checking every argument
 * and updates flags
 * -if the input is valid this function build the request for the server
 * -if not it returns NULL - something was wrong
 */
char* buildRequest(int ,char**);
/*
 * this function gets a char* that points to place with time
 * and checks if it in right format D:H:M (D-day;H-hour;M-min)
 * returns
 * -SUCCESS - if it in right format
 * -FAIL - if not
 */
int checkTimeInterval(char*);
/*
 * this function gets integer and check its length
 * return value is num of digits
 */
int intlen(int);
/*
 * this function gets sd - socket descriptor and buffer - the request
 * and trying to write to the server.
 * returns
 * -SUCCESS - if this function succeed
 * -FAIL - if something went wrong on trying to write to the server
 */
int writeToServ(int,char*);

//------------------------------//


int main(int argc,char* argv[])
{
	int sd;//socket descriptor
	int resp_bytes;//size of response in bytes

	struct sockaddr_in sock_addr;
	struct hostent* servName;

	char *request, *hostname;
	char temp[20000];
	int rc = 0,total_bytes=0;

	memset(temp,'\0',sizeof(temp));

	if(argc< 2 || argc > 5)
	{
		fprintf(stderr,"Usage: client [-h] [-d <time-interval>]<URL>\n");
		exit(1);
	}
	request = buildRequest(argc,argv);
	if(request == NULL)
	{
		if(_flagport)
		{
			fprintf(stderr,"wrong input\n");
			exit(1);
		}
		else if(_flagURL == FALSE ||_flagd == FALSE
				|| _flagWrng == FALSE
				|| (_flagd == TRUE && _flagURL == -1)
				|| _flagURL == -1)
		{
			fprintf(stderr,"Usage: client [-h] [-d <time-interval>]<URL>\n");
			exit(1);
		}
		else
		{
			fprintf(stderr,"wrong input\n");
			exit(1);
		}

	}

	//assuming that all checks at the build request
	//and if the program got here without exit(1)
	//the input/usage valid.
	hostname = gethostName(argv[url_index]);
	if(hostname == NULL)
	{
		perror("allocation");
		exit(1);
	}

	printf("HTTP request =\n%s\nLEN = %d\n", request, (int)strlen(request));


	//socket creation
	if( (sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 )
	{
		free(request);
		free(hostname);
		perror("socket");
		exit(1);
	}


	//get IP address by server name
	if( (servName = gethostbyname(hostname)) == NULL )
	{
		free(request);
		free(hostname);
		herror("gethostbyname");
		exit(1);
	}


	//sock address update
	sock_addr.sin_addr = *((struct in_addr*) servName->h_addr );
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port);

	memset(sock_addr.sin_zero,'\0',sizeof(sock_addr.sin_zero));

	//connection to server
	if( (rc = connect(sd,(struct sockaddr*)&sock_addr,sizeof(sock_addr))) == -1 )
	{
		free(request);
		free(hostname);
		perror("connect");
		exit(1);
	}
	//read and write to the server.
	if(writeToServ(sd,request) == FAIL)
	{
		free(request);
		free(hostname);
		exit(1);
	}
	resp_bytes = 0;


	do{
		memset(temp,'\0',sizeof(temp));
		resp_bytes = read(sd,temp,sizeof(temp));
		if(resp_bytes == -1)
		{
			free(request);
			free(hostname);
			perror("read - ERROR");
			exit(1);

		}
		printf("%s",temp);
		total_bytes+=resp_bytes;

	}while(resp_bytes);
	printf("\n Total received response bytes: %d\n",total_bytes);
	//frees:
	close(sd);
	free(request);
	free(hostname);
	return 0;
}

char* gethostName(char* str)
{
	int i = 0,j = 0;
	char tmp[2048];

	memset(tmp,'\0',sizeof(tmp));


	for(i = SHIFTHTTP; i < strlen(str); i++)
	{
		if(str[i]!=':' && str[i] != '/')
		{
			tmp[j] = str[i];
			j++;
		}
		else break;
	}
	char* hostname = (char*)malloc(sizeof(char)*sizeof(tmp)+1);
	if(hostname == NULL)
		return NULL;

	//	memset(hostname,'\0',sizeof(hostname));

	strcpy(hostname,tmp);
	return hostname;
}
char* buildRequest(int argc,char* argv[])
{
	char* req = NULL;

	char host[2048],
	filepath[2048],
	temp_port[6];
	char* url;

	url_index = -1;

	memset(temp_port,'\0',sizeof(temp_port));
	memset(host,'\0',sizeof(host));
	memset(filepath,'\0',sizeof(filepath));

	int i,j=0;
	//--init flags
	_flagh = -1;
	_flagd = -1;
	_flagURL = -1;
	_flagWrng = -1;
	_flagport = -1;
	//---//
	for(i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')//command
		{
			if(argv[i][1] == 'h'
					&& (strlen(argv[i]) == 2))
			{
				if(_flagh == TRUE)
				{
					_flagh = FALSE;
					return NULL;
				}
				else _flagh = TRUE;
			}
			else if(argv[i][1] == 'd'
					&& (strlen(argv[i]) == 2))
			{
				//assuming that must be element after '-d'
				if(_flagd == TRUE)
				{
					_flagd = FALSE;
					return NULL;
				}
				_flagd = TRUE;//there is '-d'
				if(checkTimeInterval(argv[i + 1]) == FAIL)
					return NULL;
				i++;
			}
			else
			{
				_flagWrng = FALSE;
				return NULL;
			}
		}
		if(strstr(argv[i],"http://"))
		{
			if(_flagURL == TRUE)
			{
				_flagURL = FALSE;
				return NULL;
			}
			_flagURL = TRUE;
			url_index = i;
			url = argv[i];

		}
		else
		{
			if(_flagURL == TRUE)
				continue;
			else _flagURL = FALSE;
		}


	}
	if(_flagURL == FALSE)
		return NULL;

	//copy data:
	strcat(host,"http://");
	j = SHIFTHTTP;
	//copy host + port
	i = 0;
	int p = 0;


	while( (url[i+SHIFTHTTP]!='/')
			&& ( (i+SHIFTHTTP)< strlen(url)) )
	{
		if(url[i+SHIFTHTTP] == ':')
		{
			_flagport = TRUE;
			if( (i+SHIFTHTTP) == strlen(url)-1 )
				return NULL;
			while ((url[i+SHIFTHTTP+1]!='/')
					&& (i+SHIFTHTTP+1) < strlen(url)
					&& p < 5)
			{
				temp_port[p] = url[i+SHIFTHTTP+1];
				p++;
				i++;
			}
			break;
		}
		else
		{
			host[j] = url[i+SHIFTHTTP];
			i++;
			j++;
		}

	}
	if(host[SHIFTHTTP] == '\0')
	{
		_flagURL = FALSE;
		return NULL;
	}
	//copy filepath:
	j = 0;
	if(url[i+SHIFTHTTP] != '/')
	{
		while(url[i+SHIFTHTTP] != '/')
			i++;
	}

	while( (url[i+SHIFTHTTP+1] != '\0')
			&& ( (i+SHIFTHTTP+1) < strlen(url)) )
	{
		filepath[j] = url[i+SHIFTHTTP+1];
		i++;
		j++;
	}

	//check port validation:
	if(_flagport == TRUE)
	{
		int tmp = atoi(temp_port);
		//to check the port - using ASCII values
		if(tmp == 0
				&& (temp_port[0] < 48 || temp_port[0] > 57))
		{
			_flagport = FALSE;
			return NULL;
		}
		if( (int)strlen(temp_port) != intlen(tmp) )
		{
			_flagport = FALSE;
			return NULL;
		}
		if(tmp < 0 || tmp > 65525)
		{
			_flagport = FALSE;
			return NULL;
		}
		port = tmp;
	}
	else port = DEFAULTPORT;
	//	if((strcmp(filepath,"/") == 0))
	//		return NULL;

	int length = sizeof(host)+sizeof(filepath);

	//build request:
	req = (char*)malloc(sizeof(char)*length + 1);
	if(req == NULL)
		return NULL;
	memset(req,'\0',sizeof(req));
	//#if 1
	if(_flagh == TRUE)
		strcat(req,"HEAD ");
	else strcat(req,"GET ");

	strcat(req,host);
	strcat(req,"/");
	strcat(req,filepath);
	strcat(req," HTTP/1.0");
	//#else
	//	memcpy(req,TEST_HTTP,strlen(TEST_HTTP));
	//#endif
	if(_flagd == TRUE)
	{
		strcat(req,"\r\n");
		strcat(req,"If-Modified-Since: ");
		strcat(req,timebuf);

	}
	strcat(req,"\r\n\r\n");
	return req;

}
//------------------------------//

int checkTimeInterval(char* str)
{

	if (str == NULL)
		return FAIL;
	if(str[0] == '-')
		return FAIL;

	int day = 0,
			min = 0,
			hour = 0;
	now = time(NULL);

	//--------------//
	char* ptr;
	char temp[128];
	int count = 0;
	strcpy(temp, str);
	ptr = strtok(temp, ":");

	int i = 0, length = 0;
	//check if amount of ':' is orderly
	for (i = 0; i < strlen(str); i++)
		if (str[i] == ':')
			count++;
	if (count != 2)
		return FAIL;
	count = 0;
	//check if there are enough arguments
	while (ptr != NULL)
	{
		//to check the time - using ASCII values
		if(atoi(ptr) == 0
				&& (ptr[0]<48 || ptr[0]>57) )
			return FAIL;
		if(strlen(ptr) != intlen(atoi(ptr)))
			return FAIL;
		switch (count)
		{
		case 0:
			day = atoi(ptr);
			break;
		case 1:
			hour = atoi(ptr);
			break;
		case 2:
			min = atoi(ptr);
			break;
		}
		count++;
		ptr = strtok(NULL, ":");
	}
	if (count != 3)
		return FAIL;
	//------------//
	now = now-(day*24*3600+hour*3600+min*60);
	strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
	return SUCCESS;
}
//------------------------------//

int intlen(int num)
{
	int l = 1;
	while(num > 9)
	{
		l++;
		num/=10;
	}
	return l;
}
//------------------------------//

int writeToServ(int sd,char* buff)
{
	int ret_val = 0;

	ret_val = write(sd,buff,strlen(buff));

	if(ret_val == 0)
	{
		perror("write - connection lost");
		return FAIL;
		//		exit(1);
	}
	if( ret_val == -1)
	{
		perror("write - error");
		return FAIL;
		//		exit(1);
	}
	return SUCCESS;
}
//------------------------------//
