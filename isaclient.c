/*

Projekt: HTTP nastenka
Autor: Šimon Matyaš (xmatya11)
*/

/* Generic */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
/* Network */
#include <netdb.h>
#include <sys/socket.h>

#define BUF_SIZE 5000
#define HOST_NAME_MAX 255
#define COMMAND_LEN_MAX 1024
#define REQUEST_LEN_MAX 8096
//Return 0 if string starts with specified prefix
//Parameter prefix - prefix 
//Parameter string - string to be compared
int prefix(char* prefix,char* string){

	return strncmp(prefix,string,strlen(prefix));
}

// Get host information (used to establishConnection)
//Source: https://github.com/pradyuman/socket-c
struct addrinfo *getHostInfo(char* host, char* port) {
	int r;
	struct addrinfo hints, *getaddrinfo_res;
	// Setup hints
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if ((r = getaddrinfo(host, port, &hints, &getaddrinfo_res))) {
		fprintf(stderr, "[getHostInfo:21:getaddrinfo] %s\n", gai_strerror(r));
		return NULL;
	}

	return getaddrinfo_res;
}

// Establish connection with host
//Source: https://github.com/pradyuman/socket-c
int establishConnection(struct addrinfo *info) {
	if (info == NULL) return -1;

	int clientfd;
	for (;info != NULL; info = info->ai_next) {
		if ((clientfd = socket(info->ai_family,
													 info->ai_socktype,
													 info->ai_protocol)) < 0) {
			perror("[establishConnection:35:socket]");
			continue;
		}

		if (connect(clientfd, info->ai_addr, info->ai_addrlen) < 0) {
			close(clientfd);
			perror("[establishConnection:42:connect]");
			continue;
		}

		freeaddrinfo(info);
		return clientfd;
	}

	freeaddrinfo(info);
	return -1;
}
int PrintOn;

//Send request to server
void RequestSend(int clientfd,char* command,char* host,int argc,char** argv){
	char* zprava=malloc(strlen(argv[argc-1]));
	zprava=argv[argc-1];
	for(int i=0;strlen(zprava)>i;i++){
		if(zprava[i]=='\n'){
			zprava[i]=1;
		}
	}
	//process command to HTTP method 
	char HttpRequest[REQUEST_LEN_MAX]={0};
		if(strcmp("boards",command)==0){
			sprintf(HttpRequest,"GET /boards HTTP/1.1\r\nHost: %s\r\n\r\n",host);
			PrintOn=1;
		}else if(prefix("board add ",command)==0){
			sprintf(HttpRequest,"POST /boards/%s HTTP/1.1\r\nHost: %s\r\n\r\n",argv[argc-1],host);
		}else if(prefix("board delete ",command)==0){
			sprintf(HttpRequest,"DELETE /boards/%s HTTP/1.1\r\nHost: %s\r\n\r\n",argv[argc-1],host);
		}else if(prefix("board list ",command)==0){
			sprintf(HttpRequest,"GET /board/%s HTTP/1.1\r\nHost: %s\r\n\r\n",argv[argc-1],host);
			PrintOn=1;
		}else if(prefix("item add ",command)==0){
			sprintf(HttpRequest,"POST /board/%s HTTP/1.1\r\nHost: %s\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s",argv[argc-2],host,strlen(argv[argc-1]),zprava);
		}else if(prefix("item delete ",command)==0){
			sprintf(HttpRequest,"DELETE /board/%s/%s HTTP/1.1\r\nHost: %s\r\n\r\n",argv[argc-2],argv[argc-1],host);
		}else if(prefix("item update ",command)==0){
			sprintf(HttpRequest,"PUT /board/%s/%s HTTP/1.1\r\nHost: %s\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s",argv[argc-3],argv[argc-2],host,strlen(argv[argc-1]),zprava);
		}else{
			fprintf(stderr,"neznámy command\n");
			exit(-1);
		}
	send(clientfd, HttpRequest, strlen(HttpRequest), 0);
}

int main(int argc, char **argv) {
	int clientfd;
	char buf[BUF_SIZE]={0};

	int c;
	PrintOn=0;
	char port[100];
	char host[HOST_NAME_MAX];
	char command[COMMAND_LEN_MAX]={0};
	int hostOn=0;
	int portOn=0;
	while ((c = getopt (argc, argv, "p:hH:")) != -1)//reading arguments
		switch (c)
			{
			case 'p':
				strcpy(port,optarg);
				portOn=1;
				break;
			case 'H':
				strcpy(host,optarg);
				hostOn=1;
				break;
			case 'h':printf("Napoveda pro spusteni klienta:\n"
						    "Parametry:\n\t-h - vypis teto napovedy\n"
						    "\t-p - tento parametr urcuje na kterem portu se klient pripoji k serveru\n"
						    "\t-H - tento parametr urcuje host\n"
						    "Spusob spusteni klienta:\n"
						    "./isaclient -H <host> -p <port> <command>\n");
							"dostupne commandy:\n"
							"\tboards - vraci seznam dostupnych nastenek\n"
							"\tboard add <name> - vytvori novou prazdnou nastenku s nazvem name\n"
							"\tboard delete <name> - smaze nastenku s nazvem name\n"
							"\tboard list <name> - zobrazi obsah nastenky name\n"
							"\titem add <name> <content> - vlozi novy prispevek na posledni misto nastenky name\n"
							"\titem delete <name> <id> - odstrani prispevek cislo id z nastenky name\n"
							"\titem update <name> <id> <content> - upravi prispevek cislo id z nastenky name\n";

				break;
			default:
				break;
			}
	if(hostOn==0 || portOn==0){
		exit(-1);
	}
	for(; optind < argc; optind++){     

			if(strlen(command)==1){
				strcpy(command,argv[optind]);
			}else{
				strcat(command, argv[optind]);
			}

			strcat(command, " ");
	}
	command[strlen(command)-1]='\0';

	// Establish connection with <hostname>:<port>
	clientfd = establishConnection(getHostInfo(host, port));
	if (clientfd == -1) {
		fprintf(stderr,
			"[main:73] Failed to connect\n");
		return 3;
	}

	char* token;
	char* ResponseHeader;
	char* ResponseMessage;
	RequestSend(clientfd,command,host,argc,argv);
	//sprintf(command,"");
	while (recv(clientfd, buf, BUF_SIZE, 0) > 0) {
		token=strtok(buf,"\r\n\r\n");//procces response
		ResponseHeader=token;
		token=strtok(NULL,"");
		ResponseMessage=token;
		fputs(ResponseHeader, stderr);
		ResponseMessage+=2;
		if(PrintOn==1){
			if(ResponseMessage==NULL){
				printf("");
			}else{
				printf("%s",ResponseMessage);
			}
		}
		memset(buf, 0, BUF_SIZE);
	}

	close(clientfd);
	return 0;
}