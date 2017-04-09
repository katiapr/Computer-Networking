//Author : Katia Prigon
//ID : 322088154
// EX3 - HTTP server
#include "threadpool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <netinet/in.h> // for INET_ANY
#include <sys/socket.h> //for socket
#include <errno.h> //for errors
#include <dirent.h> //for dir's function method
#include <sys/stat.h> //for stat
#include <fcntl.h>

/////////DEFINES/////////////
#define FAIL				-1
#define SUCCESS				0
#define MAX_FIRST_LINE_LEN 	4000
#define MAX_LINE_LEN 		500
#define MAX_BODY_DIR_LEN	4096

#define RES_200				"200 OK"
#define RES_302 			"302 Found"
#define RES_400 			"400 Bad Request"
#define RES_403 			"403 Forbidden"
#define RES_404 			"404 Not Found"
#define RES_500 			"500 Internal Server Error"
#define RES_501 			"501 Not supported"
#define INDEX				"index.html"
//-----------------------------//
#define HTTP				"HTTP/1.0 "
#define SERVER				"Server: webserver/1.0"
#define DATE				"Date: "
#define CONT_TYPE			"Content-Type: "
#define	CONT_LEN			"Content-Length: "
#define LAST_MOT			"Last-Motified: "
#define	CONNECTION			"Connection: close"

#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

///// STRUCTS /////
//TODO - remove no need enum
typedef enum { ERR302=1, ERR400, ERR403, ERR404, ERR500, ERR501,ERR200 ,ERR_DIR, ERR_FILE } err_t;

///// GLOBALS /////
char* method, *path, *version;
char dest_path[MAX_FIRST_LINE_LEN];

///// FUNCTIONS /////
//func check the port number - returns 0 on success and -1 otherwise
int valid_input(char* argv[], int argc);
//create the server by creating sockets  - returns 0 on success and -1 otherwise
int create_server(struct sockaddr_in *srv, int port);
//using the threadpool to redirect the request_handle function - returns 0 on succes and -1 otherwise
int manage_server(int server_sd, int poolsize, int max_num_req);
//this function sends to the client the response
int request_handler(void*);
//get the request and check its validation
int get_res_err(char*);
//check permisitions of directory and file
int permissions();
//----------------------//
//read the request from the client
char* get_request(int sd);
//build the html code (body)
char* build_body(int res_err);
//build the html table to show all content of the path
char* build_dir_body();
//build the response headers
char* build_headers(int res_err, char* body);
//check the content type
char* get_mime_type(char *name);

////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	int port, poolsize, max_num_req;

	// Input:
	if (valid_input(argv, argc) == FAIL)
	{
		fprintf(stderr,"Usage: server <port> <poolsize> <max-number-of-request>\n");
		exit(EXIT_FAILURE);
	}
	else
	{
		port = atoi(argv[1]);
		poolsize = atoi(argv[2]);
		max_num_req = atoi(argv[3]);
	}

	// Server:
	struct sockaddr_in srv;
	int server_sd;

	if ((server_sd = create_server(&srv, port)) == FAIL)
		exit(EXIT_FAILURE);

	if (manage_server(server_sd, poolsize, max_num_req) == FAIL)
		exit(EXIT_FAILURE);

	return SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////////////

int valid_input(char* argv[], int argc)
{
	if (argc != 4)
		return FAIL;

	int i,j;
	for (i=1; i<argc; i++)
	{
		for (j=0; j<strlen(argv[i]); j++)
		{
			if (isdigit(argv[i][j]) == FAIL)
				return FAIL;
		}
	}

	return SUCCESS;
}

int create_server(struct sockaddr_in *srv, int port)
{
	// Socket:
	int server_sd = socket(PF_INET, SOCK_STREAM, 0);
	if (server_sd < 0)
	{
		perror("socket");
		return FAIL;
	}

	srv->sin_family = AF_INET;
	srv->sin_port = htons(port);
	srv->sin_addr.s_addr = htonl(INADDR_ANY);

	// Bind:
	if (bind(server_sd, (struct sockaddr*)srv, sizeof(*srv)) < 0)
	{
		close(server_sd);
		perror("bind");
		return FAIL;
	}

	// Listen:
	if (listen(server_sd, 5) < 0)
	{
		close(server_sd);
		perror("listen");
		return FAIL;
	}

	return server_sd;
}

int manage_server(int server_sd, int poolsize, int max_num_req)
{
	// Threadpool:
	threadpool *pool = create_threadpool(poolsize);
	if (pool == NULL)
	{
		close(server_sd);
		return FAIL;
	}

	// Accept and dispatch request handle:
	int* client_sd, i;
	for (i=0; i<max_num_req; i++)
	{
		client_sd = (int*)malloc(sizeof(int));
		if (client_sd == NULL)
		{
			close(server_sd);
			destroy_threadpool(pool);
			perror("malloc");
			return FAIL;
		}

		*client_sd = accept(server_sd, NULL, NULL);
		if (*client_sd < 0)
		{
			close(server_sd);
			destroy_threadpool(pool);
			perror("accept");
			return FAIL;
		}

		dispatch(pool, request_handler, client_sd);
	}

	close(server_sd);
	destroy_threadpool(pool);

	return SUCCESS;
}

int request_handler(void* arg)
{
	int client_sd = *((int*)arg);
	memset(dest_path,'\0',sizeof(dest_path));

	char* request = get_request(client_sd);
	if (request == NULL)
	{
		close(client_sd);
		free(arg);
		return FAIL;
	}
	char cpy_request[MAX_FIRST_LINE_LEN];
	memset(cpy_request,'\0',sizeof(cpy_request));
	strcpy(cpy_request,request);
	// Set tokens:

	method = strtok(cpy_request, " ");
	path = strtok(NULL, " ");
	version = strtok(NULL, " ");

	// Build response:
	int res_err = get_res_err(request);//if for every thing
	char* body = build_body(res_err);//switch case
	if(body == NULL)
	{
		body = dest_path;
	}
	char* headers = build_headers(res_err, body);//+strlen body
	// Write to client:
	int ttl_bytes=0;
	int rcv_bytes=0;

	// Write the headers:
	rcv_bytes = write(client_sd, headers, strlen(headers));
	if (rcv_bytes < 0)
	{
		free(arg);
		if(res_err != ERR_FILE)
			free(body);
		free(headers);
		free(request);
		perror("write");
		return FAIL;
	}
	// Write the body:
	char buffer[MAX_FIRST_LINE_LEN];
	if (res_err == ERR_FILE)				// Write the file instead of the body.
	{
		int fd = open(dest_path, O_RDONLY);		// File Descriptor.
		if (fd < 0)
		{
			free(arg);
			// free body & headers if allocated
			if(res_err != ERR_FILE)
				free(body);
			free(headers);
			free(request);
			perror("open");
			return FAIL;
		}

		do {
			rcv_bytes = read(fd, buffer, sizeof(buffer));		// read from file
			if (rcv_bytes < 0)
			{
				free(arg);
				// free body & headers if allocated
				if(res_err != ERR_FILE)
					free(body);
				free(headers);
				free(request);
				perror("read");
				return FAIL;
			}

			if (write(client_sd, buffer, rcv_bytes) < 0)		// write it to client.
			{
				free(arg);
				// free body & headers if allocated
				if(res_err != ERR_FILE)
					free(body);
				free(headers);
				free(request);
				perror("write");
				return FAIL;
			}

			memset(buffer, 0, sizeof(buffer));
		} while (rcv_bytes > 0);

		close(fd);
	}
	else
	{
		int body_length = strlen(body);
		while (ttl_bytes != body_length)
		{
			rcv_bytes = write(client_sd, body + ttl_bytes, body_length - ttl_bytes);
			if (rcv_bytes < 0)
			{
				free(arg);
				free(request);
				free(headers);
				if(res_err != ERR_FILE)
					free(body);
				// free body & headers if allocated
				perror("write");
				return FAIL;
			}

			ttl_bytes += rcv_bytes;
		}
	}
	//frees

	free(request);
	free(headers);
	if(res_err != ERR_FILE)
		free(body);
	free(arg);
	close(client_sd);
	return SUCCESS;
}

char* get_request(int sd)
{
	int ttl_bytes=0;
	int rcv_bytes;

	char buffer[MAX_FIRST_LINE_LEN];
	memset(buffer, 0, sizeof(buffer));

	do {
		rcv_bytes = read(sd, buffer + ttl_bytes, sizeof(buffer) - ttl_bytes);
		if(rcv_bytes < 0)
		{
			perror("read");
			return NULL;
		}
		ttl_bytes += rcv_bytes;

		char* ptr = strstr(buffer, "\r\n");
		if (ptr != NULL)
		{
			ptr[0] = '\0';
			break;
		}

	} while(1);
	char* request = (char*)malloc(sizeof(char)*ttl_bytes +1);
	strcpy(request,buffer);
	//	request = buffer;
	return request;
}

int get_res_err(char* firstline)
{
	//TODO - remove
	// check the tokens and return the err response accordingly:
	// alloc dest_path for the current path!
	int		 	res_err = 0;
	int			rc = 0;

	struct stat file_stat;
	//------------//
	char str[MAX_FIRST_LINE_LEN];
	memset(str,'\0',sizeof(str));
	strcpy(str,firstline);

	char* 		ptr = strtok(firstline," ");

	while(ptr != NULL)
	{
		rc++;
		ptr = strtok(NULL," ");
	}
	if(rc != 3 || ((strcmp(version,"HTTP/1.0") != SUCCESS)
			&& (strcmp(version,"HTTP/1.1") != SUCCESS)) )
	{
		return res_err = ERR400;
	}


	if( strcmp(method,"GET") != SUCCESS )
		return res_err = ERR501;

	if(path[0] != '/')
		return ERR400;

	strcat(dest_path,".");
	strcat(dest_path, path);

	rc = stat(dest_path, &file_stat);

	if (rc == FAIL)
	{
		if(errno == EACCES)
			return ERR403;

		if(errno == ENOENT)
			return ERR404;

		return ERR500;
	}

	rc = permissions();
	if (rc == FAIL)
		return ERR403;

	//if path is directory
	if(S_ISDIR(file_stat.st_mode))
	{
		//if the directory ends with '/'
		if(dest_path[strlen(dest_path)-1] == '/')
		{
			strcat(dest_path,INDEX);

			rc = stat(dest_path,&file_stat);
			if (rc == FAIL)
			{
				if(errno == ENOENT)
				{
					memset(dest_path+strlen(path)+1, '\0', strlen(INDEX));
					return ERR_DIR;
				}

				if (errno == FAIL)
				{
					perror("stat");
					return ERR500;
				}
			}

			if ( !(file_stat.st_mode & S_IROTH) )
				return ERR403;
			else
				return ERR_FILE;
		}
		else
			return ERR302;
	}
	//if the path is file
	else
		return ERR_FILE;
}

char* build_body(int res_err)
{
	char* body;
	char* temp_body = (char*)malloc(sizeof(char)*MAX_LINE_LEN);
	if(temp_body == NULL)
		return NULL;

	memset(temp_body, '\0', MAX_LINE_LEN);
	strcat(temp_body,"<HTML><HEAD><TITLE>");
	if (res_err == ERR_DIR)
	{
		body = build_dir_body();//no need path its global
		if (body == NULL)
		{
			res_err = ERR500;
			body = temp_body;
		}
		else free(temp_body);
	}
	else if (res_err == ERR_FILE)
	{
		body = NULL;
		free(temp_body);
	}
	else{
		body = temp_body;
	}
	switch(res_err)
	{
	case ERR302:
		strcat(temp_body,RES_302);
		strcat(temp_body,"</TITLE></HEAD>\r\n<BODY><H4>");
		strcat(temp_body,RES_302);
		strcat(temp_body,"</H4>\r\nDirectories must end with a slash.\r\n</BODY></HTML>/\r\n\r\n");
		break;

	case ERR400:
		strcat(temp_body,RES_400);
		strcat(temp_body,"</TITLE></HEAD>\r\n<BODY><H4>");
		strcat(temp_body,RES_400);
		strcat(temp_body,"</H4>\r\nBad Request.</BODY></HTML>\r\n\r\n");
		break;

	case ERR403:
		strcat(temp_body,RES_403);
		strcat(temp_body,"</TITLE></HEAD>\r\n<BODY><H4>");
		strcat(temp_body,RES_403);
		strcat(temp_body,"</H4>\r\nAccess denied.\r\n</BODY></HTML>\r\n\r\n");
		break;

	case ERR404:
		strcat(temp_body,RES_404);
		strcat(temp_body,"</TITLE></HEAD>\r\n<BODY><H4>");
		strcat(temp_body,RES_404);
		strcat(temp_body,"</H4>\r\nFile not found.\r\n</BODY></HTML>\r\n\r\n");
		break;

	case ERR500:
		strcat(temp_body,RES_500);
		strcat(temp_body,"</TITLE></HEAD>\r\n<BODY><H4>");
		strcat(temp_body,RES_500);
		strcat(temp_body,"</H4>\r\nSome server side error.\r\n</BODY></HTML>\r\n\r\n");
		break;

	case ERR501:
		strcat(temp_body,RES_501);
		strcat(temp_body,"</TITLE></HEAD>\r\n<BODY><H4>");
		strcat(temp_body,RES_501);
		strcat(temp_body,"</H4>\r\nMethod is not supported.\r\n</BODY></HTML>\r\n\r\n");
		break;
	}
	return body;
}

char* build_dir_body()
{
	// build the html table:
	DIR* dir;
	char modified[128];
	char curr_dir[1024];
	char* dir_body = NULL;
	char entity[MAX_LINE_LEN] ;
	int flag_entity_file = FAIL;
	struct stat file_stat;

	memset(modified,'\0',sizeof(modified));
	memset(curr_dir,'\0',sizeof(curr_dir));

	if(stat(dest_path,&file_stat) == FAIL)
		return NULL;
	//now in modifiend :
	strftime(modified, sizeof(modified), RFC1123FMT, gmtime(&(file_stat.st_mtim.tv_sec)));
	if(strlen(dest_path) == 1)//there is only '/'
	{
		if( getcwd(curr_dir,sizeof(curr_dir)) == NULL )
		{
			return NULL;
		}
		strcat(dest_path,curr_dir);
		strcat(dest_path,"/");
		dir = opendir(dest_path);

	}
	else
	{
		dir = opendir(dest_path);
	}
	if( !dir )
	{
		return NULL;
	}

	struct dirent* dir_entity;

	char curr[MAX_LINE_LEN];

	dir_body = (char*)malloc(sizeof(char)*MAX_BODY_DIR_LEN +1);
	if(dir_body == NULL)
	{
		return NULL;
	}
	memset(dir_body,'\0',MAX_BODY_DIR_LEN+1);
	strcat(dir_body,"<HTML>");
	strcat(dir_body,"<HEAD><TITLE>Index of ");
	strcat(dir_body,dest_path);
	strcat(dir_body,"</TITLE></HEAD><BODY><H4>Index of ");
	strcat(dir_body,dest_path);
	strcat(dir_body,"</H4><table CELLSPACING=8>");
	strcat(dir_body,"<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>");
	for(; (dir_entity = readdir(dir)) != NULL;)
	{
		memset(curr,'\0',MAX_LINE_LEN);
		memset(entity,'\0',MAX_LINE_LEN);
		strcpy(curr,dest_path);
		strcat(curr,dir_entity->d_name);
		if(stat(curr,&file_stat) == FAIL)
		{
			closedir(dir);
			free(dir_body);
			return NULL;
		}


		strcpy(entity,dir_entity->d_name);


		//if entity is directory
		if(S_ISDIR(file_stat.st_mode) &&
				( (strcmp(dir_entity->d_name,".") == SUCCESS)
						|| (strcmp(dir_entity->d_name,"..") == SUCCESS)) )
			continue;


		strcat(dir_body,"<tr><td><A HREF=");
		strcat(dir_body,entity);
		strcat(dir_body,">");
		strcat(dir_body,entity);
		strcat(dir_body,"</A></td><td>");
		strcat(dir_body,modified);
		strcat(dir_body,"</td><td>");
		if(file_stat.st_mode & S_IFREG)
		{
			char f_size[32];
			sprintf(f_size,"%jd",(intmax_t)file_stat.st_size);
			strcat(dir_body,f_size);
		}

	}

	strcat(dir_body,"</td></tr>");
	strcat(dir_body,"</table><HR><ADDRESS>webserver/1.0</ADDRESS></BODY></HTML>\r\n\r\n");

	if(stat(curr,&file_stat) == FAIL)
	{
		free(dir_body);
		return NULL;
	}
	closedir(dir);
	return dir_body;
}

char* build_headers(int res_err, char* body)
{
	// build the headers, use strlen(body) for the content length.
	char* headers  = (char*)malloc(sizeof(char)* MAX_FIRST_LINE_LEN);
	time_t now;
	char timebuff[128];
	char last_mot[128];
	char* getmime;

	now = time(NULL);
	strftime(timebuff, sizeof(timebuff), RFC1123FMT, gmtime(&now));

	if(res_err == ERR_DIR || res_err == ERR_FILE)
	{
		struct stat file_stat;

		if(stat(dest_path,&file_stat) == FAIL)
		{
			perror("stat");
			return NULL;
		}

		strftime(last_mot, sizeof(last_mot), RFC1123FMT, gmtime(&(file_stat.st_mtim.tv_sec)));
	}
	switch(res_err)
	{
	case ERR302:sprintf(headers,"%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%d%s%s%s%s",
			HTTP,RES_302,"\r\n",SERVER,"\r\n",DATE,timebuff,"\r\n",
			"Location: ",dest_path,"\r\n",CONT_TYPE,"text/html","\r\n",
			CONT_LEN,(int)strlen(body),"\r\n",CONNECTION,"\r\n","\r\n");
	break;
	case ERR400:
		sprintf(headers,"%s%s%s%s%s%s%s%s%s%s%s%s%d%s%s%s%s",
				HTTP,RES_400,"\r\n",SERVER,"\r\n",DATE,timebuff,"\r\n",
				CONT_TYPE,"text/html","\r\n",CONT_LEN,(int)strlen(body),
				"\r\n",CONNECTION,"\r\n","\r\n");
		break;
	case ERR403:sprintf(headers,"%s%s%s%s%s%s%s%s%s%s%s%s%d%s%s%s%s",
			HTTP,RES_403,"\r\n",SERVER,"\r\n",DATE,timebuff,"\r\n",
			CONT_TYPE,"text/html","\r\n",CONT_LEN,(int)strlen(body),
			"\r\n",CONNECTION,"\r\n","\r\n");
	break;
	case ERR404:sprintf(headers,"%s%s%s%s%s%s%s%s%s%s%s%s%d%s%s%s%s",
			HTTP,RES_404,"\r\n",SERVER,"\r\n",DATE,timebuff,"\r\n",
			CONT_TYPE,"text/html","\r\n",CONT_LEN,(int)strlen(body),
			"\r\n",CONNECTION,"\r\n","\r\n");
	break;
	case ERR500:sprintf(headers,"%s%s%s%s%s%s%s%s%s%s%s%s%d%s%s%s%s",
			HTTP,RES_500,"\r\n",SERVER,"\r\n",DATE,timebuff,"\r\n",
			CONT_TYPE,"text/html","\r\n",CONT_LEN,(int)strlen(body),
			"\r\n",CONNECTION,"\r\n","\r\n");
	break;
	case ERR501:sprintf(headers,"%s%s%s%s%s%s%s%s%s%s%s%s%d%s%s%s%s",
			HTTP,RES_501,"\r\n",SERVER,"\r\n",DATE,timebuff,"\r\n",
			CONT_TYPE,"text/html","\r\n",CONT_LEN,(int)strlen(body),
			"\r\n",CONNECTION,"\r\n","\r\n");
	break;
	case ERR_DIR:
		sprintf(headers,"%s%s%s%s%s%s%s%s%s%s%s%s%d%s%s%s%s%s%s%s",
				HTTP,RES_200,"\r\n",SERVER,"\r\n",DATE,last_mot,"\r\n",
				CONT_TYPE,"text/html","\r\n",CONT_LEN,(int)strlen(body),
				"\r\n",LAST_MOT,last_mot,"\r\n",CONNECTION,"\r\n","\r\n");

		break;

	case ERR_FILE:
		getmime = get_mime_type(dest_path);
		if(getmime != NULL)
		{
			sprintf(headers,"%s%s%s%s%s%s%s%s%s%s%s%s%d%s%s%s%s%s%s%s",
					HTTP,RES_200,"\r\n",SERVER,"\r\n",DATE,timebuff,"\r\n",
					CONT_TYPE,getmime,"\r\n",CONT_LEN,(int)strlen(body),"\r\n",
					LAST_MOT,last_mot,"\r\n",CONNECTION,"\r\n","\r\n");
		}
		break;
	}
	return headers;
}

char *get_mime_type(char *name)
{
	char *ext = strrchr(name, '.');
	if (!ext) return NULL;
	if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
	if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
	if (strcmp(ext, ".gif") == 0) return "image/gif";
	if (strcmp(ext, ".png") == 0) return "image/png";
	if (strcmp(ext, ".css") == 0) return "text/css";
	if (strcmp(ext, ".au") == 0) return "audio/basic";
	if (strcmp(ext, ".wav") == 0) return "audio/wav";
	if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
	if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return "video/mpeg";
	if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
	return "";
}
int permissions()
{
	struct stat fs;
	stat(path,&fs);

	char* ptr;
	char tmp[MAX_FIRST_LINE_LEN];
	char cpy_dest_path[MAX_FIRST_LINE_LEN];//copy

	memset(cpy_dest_path,'\0', sizeof(cpy_dest_path));
	memset(tmp,'\0',sizeof(tmp));

	strcpy(cpy_dest_path,dest_path);
	strcpy(tmp,".");

	ptr = strtok(cpy_dest_path,"/");
//check the all path by cutting it
	while(ptr != NULL)
	{

		strcat(tmp,"/");
		strcat(tmp,cpy_dest_path);

		if(stat(tmp,&fs) == FAIL)
		{
			perror("stat");
			return FAIL;
		}
		//path is directory
		if(S_ISDIR(fs.st_mode)
				&& !(fs.st_mode & S_IXOTH))
			return FAIL;
		//path is a file
		if(S_ISREG(fs.st_mode)
				&& !(fs.st_mode & S_IROTH))
			return FAIL;
		ptr = strtok(NULL,"/");
	}

	return SUCCESS;
}
