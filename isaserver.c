/*

Projekt: HTTP nastenka
Autor: Šimon Matyaš (xmatya11)
*/

/* Generic */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Network */
#include <netdb.h>
#include <sys/socket.h>

#define BUF_SIZE 8000
#define MESSAGE_MAX_LEN 1000
//Struct for records
typedef struct zaznam{
    struct zaznam* next;
    int id;
    char* text;
}zaznam;
//Struct for boards
typedef struct nastenka{
    char* jmeno;
    int zaznamCount;
    struct nastenka *next;
    zaznam *zaznam;
}nastenka;

void initMainBoard();
int NewBoard(char* Name);
int DeleteBoard(char* Name);
char* PrintAllBoards();
char* PrintBoard();
int AddZaznam(char* NastenkaName,char* Content);
int DeleteZaznam(char* NastenkaName,int id);
int UpdateZaznam(char* NastenkaName,int id,char* Content);
//Return 0 if string starts with specified prefix
//Parameter prefix - prefix 
//Parameter string - string to be compared
int prefix(char* prefix,char* string){

  return strncmp(prefix,string,strlen(prefix));
}

// Get addr information (used to bindListener)
//Source: https://github.com/pradyuman/socket-c
struct addrinfo *getAddrInfo(char* port) {
  int r;
  struct addrinfo hints, *getaddrinfo_res;
  // Setup hints
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_socktype = SOCK_STREAM;
  if ((r = getaddrinfo(NULL, port, &hints, &getaddrinfo_res))) {
    fprintf(stderr, "[getAddrInfo:21:getaddrinfo] %s\n", gai_strerror(r));
    return NULL;
  }

  return getaddrinfo_res;
}

// Bind Listener
//Source: https://github.com/pradyuman/socket-c
int bindListener(struct addrinfo *info) {
  if (info == NULL) return -1;

  int serverfd;
  for (;info != NULL; info = info->ai_next) {
    if ((serverfd = socket(info->ai_family,
                           info->ai_socktype,
                           info->ai_protocol)) < 0) {
      perror("[bindListener:35:socket]");
      continue;
    }

    int opt = 1;
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(int)) < 0) {
      perror("[bindListener:43:setsockopt]");
      return -1;
    }

    if (bind(serverfd, info->ai_addr, info->ai_addrlen) < 0) {
      close(serverfd);
      perror("[bindListener:49:bind]");
      continue;
    }

    freeaddrinfo(info);
    return serverfd;
  }

  freeaddrinfo(info);
  return -1;
}

void resolve(int handler) {
  int size;
  char buf[BUF_SIZE]={0};
  char *HeaderPart[255];
  char *token=malloc(1000);

  recv(handler, buf, BUF_SIZE, 0);
  //Splitting recieved message
  token = strtok(buf, "\r\n");
  HeaderPart[0]=token;
  for(int i=1;token != NULL;i++){
      token = strtok(NULL, "\r\n"); 
      HeaderPart[i]=token;
    
  }

  char* Command;
  char* Path;
  char* HTTPVersion;
  char Zprava[MESSAGE_MAX_LEN]={0};
  char* ContenLen;
  char* ContenType;
  char ResponseMessage[2000]={0};
  int ResponseCode=404;
  char* ResponseCodeMessage;
  char Response[2000];
  int HeaderOk=1;
  int ContentID=0;
  for(int i=1;;i++){//Checking if HTTP header is correct
    if(HeaderPart[i]==NULL){
      ContentID=i-1;
      break;
    }
    if(prefix("Content-Type:",HeaderPart[i])==0){
      ContenType=HeaderPart[i];
      if(strcmp(ContenType,"Content-Type: text/plain")!=0){
        HeaderOk=0;
      }
    }else if(prefix("Content-Length:",HeaderPart[i])==0){
      ContenLen=HeaderPart[i];
    }
  }
  //Reading content of recieved message
  if(HeaderPart[ContentID]!=NULL){
    //Zprava=HeaderPart[ContentID];
    strcpy(Zprava,HeaderPart[ContentID]);
    for(int i=0;strlen(Zprava)>i;i++){
      if(Zprava[i]==1){
        Zprava[i]='\n';
      }
    }
  }
  Command = strtok(HeaderPart[0]," ");
  Path = strtok(NULL," ");
  HTTPVersion = strtok(NULL," ");

  char* PathPart[4];

  token=strtok(Path,"/");
  PathPart[0]=token;
  for(int i=1;token != NULL;i++){//Splitting recieved part
    token=strtok(NULL,"/");
    PathPart[i]=token;
  }
  //Pathpart[0] - boards
  //Pathpart[1] - nazev
  //Pathpart[2] - zaznam
  if(HeaderOk==1){//If header is OK, processing command

  if(strcmp(Command,"GET")==0){
    if(PathPart[1]!=NULL){
      //Print specified board
      sprintf(ResponseMessage,"%s",PrintBoard(PathPart[1]));
      if(strcmp(ResponseMessage,"\\")!=0){//Check if board to be printed exists
        ResponseCode=200;
      }else{
        ResponseCode=404;
        sprintf(ResponseMessage,"");
      }
    }else{
      //Print all boards
      sprintf(ResponseMessage,"%s",PrintAllBoards());
      if(ResponseMessage!=NULL){
        ResponseCode=200;
      }else{
        ResponseCode=404;
      }
    }
  }else if(strcmp(Command,"POST")==0){
    if(strcmp(PathPart[0],"board")==0){
      //Adding new record
      if(strcmp(ContenLen,"Content-Length: 0")==0){//Checking if Content-Length is correct
        ResponseCode=400;
      }else{
        ResponseCode=AddZaznam(PathPart[1],Zprava);
      }

    }else if(strcmp(PathPart[0],"boards")==0){
      //Adding new board
      ResponseCode=NewBoard(PathPart[1]);
    }
  }else if(strcmp(Command,"DELETE")==0){
    if(strcmp(PathPart[0],"board")==0){
      //Removing record
      ResponseCode=DeleteZaznam(PathPart[1],atoi(PathPart[2]));
    }else if(strcmp(PathPart[0],"boards")==0){
      //Removing board
      ResponseCode=DeleteBoard(PathPart[1]);
    }
  }else if(strcmp(Command,"PUT")==0){
    //Updating record
    if(strcmp(ContenLen,"Content-Length: 0")==0){//Checking if Content-Length is correct
      ResponseCode=400;
    }else{
      ResponseCode=UpdateZaznam(PathPart[1],atoi(PathPart[2]),Zprava);
      //sprintf(ResponseMessage,"");
    }
  }else{
    ResponseCode=404;
  }

  switch(ResponseCode){
    case 200:
      ResponseCodeMessage="OK";
      break;
    case 201:
      ResponseCodeMessage="Created";
      break;
    case 409:
      ResponseCodeMessage="Conflict";
      break;
    case 400:
      ResponseCodeMessage="Bad Request";
      break;
    case 404:
      ResponseCodeMessage="Not Found";
      break;
  }
  //Putting together Response message
  //printf("MessageLen: %s\n",strlen(ResponseMessage) );
  //printf("CodeLen: %s\n",strlen(ResponseMessage) );
  //printf("MessageLen: %s\n",strlen(ResponseMessage) );
  sprintf(Response,"HTTP/1.1 %d %s\r\n\r\n%s",ResponseCode,ResponseCodeMessage,ResponseMessage);
  send(handler, Response, strlen(Response), 0);//Sending response message
  sprintf(Response,"");
  sprintf(ResponseMessage,"");
  sprintf(Zprava,"");
  }
  
}
nastenka* MainBoard;
int main(int argc, char **argv) {
  char* port;
  int portOn=0;
  int c;
  while ((c = getopt (argc, argv, "p:h")) != -1)//reading arguments
  switch (c)
    {
    case 'p':
      strcpy(port,optarg);
      portOn=1;
      break;
    case 'h':
      printf("Napoveda pro spusteni serveru:\n"
      "Parametry:\n\t-h - vypis teto napovedy\n"
      "\t-p - tento parametr urcuje na kterem portu bude server ocekavat spojeni\n"
      "Spusob spusteni serveru:\n"
      "./isaserver -p 1234\n\n");
      break;
    default:
      break;
    }

  if(portOn){

  // bind a listener
  //Source: https://github.com/pradyuman/socket-c
  int server = bindListener(getAddrInfo(port));
  if (server < 0) {
    fprintf(stderr, "[main:72:bindListener] Failed to bind at port %s\n", port);
    return 2;
  }

  if (listen(server, 10) < 0) {
    perror("[main:82:listen]");
    return 3;
  }

  // accept incoming requests asynchronously
  int handler;
  socklen_t size;
  struct sockaddr_storage client;
  //initialization of MainBoard
  MainBoard=(nastenka*) malloc(sizeof(nastenka));
  initMainBoard();

  //Source: https://github.com/pradyuman/socket-c
  while (1) {
    size = sizeof(client);
    handler = accept(server, (struct sockaddr *)&client, &size);
    if (handler < 0) {
      perror("[main:82:accept]");
      continue;
    }
    resolve(handler);
    close(handler);
  }

  close(server);
  }
  return 0;
}
//Initialization of Mainboard
void initMainBoard(){
  MainBoard->jmeno=NULL;
  MainBoard->zaznamCount=0;
  MainBoard->next=NULL;
  MainBoard->zaznam=NULL;
}

//Creating New board
//parameter Name - name of new board
int NewBoard(char* Name){
  Name=strdup(Name);
  nastenka* LastBoard=MainBoard;
  while(LastBoard->next!=NULL){//search for last board
      LastBoard=LastBoard->next;
    if(strcmp(LastBoard->jmeno,Name)==0){//return code if board already exists
      return 409;
    } 
    
  }
  //Initialization of new board
  nastenka* NewBoard=(nastenka*) malloc(sizeof(nastenka));
  NewBoard->jmeno=malloc(strlen(Name));
  NewBoard->jmeno=Name;
  NewBoard->zaznamCount=0;
  NewBoard->next=NULL;
  NewBoard->zaznam=NULL;
  LastBoard->next=NewBoard;//adding new board to list of boards
  return 201;

}
//Removing existing board
//parameter Name - name of board to be removed
int DeleteBoard(char* Name){
  nastenka* LastActiveBoard=MainBoard;
  nastenka* ActiveBoard=MainBoard->next;    
  while(ActiveBoard!=NULL){
    if(strcmp(ActiveBoard->jmeno,Name)==0){//Finding specified board
      LastActiveBoard->next=ActiveBoard->next;//Removing board from list
      free(ActiveBoard->jmeno);
      free(ActiveBoard);      
      return 200;
    }else{
      LastActiveBoard=ActiveBoard;
      ActiveBoard=ActiveBoard->next;
    }   
  }
  return 404;
}
//Adding new record to end of specified board
//parameter NastenkaName - board where new record will be added
//parameter Content - content of new record
int AddZaznam(char* NastenkaName,char* Content){
  Content=strdup(Content); 
  nastenka* ActiveBoard=MainBoard->next; 
  while(ActiveBoard!=NULL){
    if(strcmp(ActiveBoard->jmeno,NastenkaName)==0){//Finding specified board
      zaznam* LastZaznam=ActiveBoard->zaznam;
      if(ActiveBoard->zaznamCount>0){
        while(LastZaznam->next!=NULL){//Finding last record on board
          LastZaznam=LastZaznam->next;
        }
      }
      ActiveBoard->zaznamCount++;//Initializing new record
      zaznam* NewZaznam=malloc(sizeof(zaznam));
      NewZaznam->next=NULL;
      NewZaznam->id=ActiveBoard->zaznamCount;
      NewZaznam->text=(char*) malloc(strlen(Content));
      NewZaznam->text=Content;
      if(ActiveBoard->zaznamCount==1){//Adding record to record list on board
        ActiveBoard->zaznam=NewZaznam;
      }else{
        LastZaznam->next=NewZaznam;
      }
      return 201;
    }else{
      ActiveBoard=ActiveBoard->next;
    }
  }
  return 404;
}

//Removing record from specified board
//parameter NastenkaName - board where record will be removed
//parameter id - id record to be removed
int DeleteZaznam(char* NastenkaName,int id){
  nastenka* ActiveBoard=MainBoard->next; 
  while(ActiveBoard!=NULL){
    if(strcmp(ActiveBoard->jmeno,NastenkaName)==0){//Finding specified board
      zaznam* LastActiveZaznam=ActiveBoard->zaznam;
      zaznam* ActiveZaznam=ActiveBoard->zaznam;
      while(ActiveZaznam!=NULL){
        if(ActiveZaznam->id==id){//Finding specified record
          if(id==1){//Removing record from list
            ActiveBoard->zaznam=ActiveZaznam->next;
          }else{
            LastActiveZaznam->next=ActiveZaznam->next;
          }
          ActiveZaznam=LastActiveZaznam->next;
          while(ActiveZaznam!=NULL){//Reducing IDs of all next records on board
            ActiveZaznam->id--;
            ActiveZaznam=ActiveZaznam->next;
          }
          ActiveBoard->zaznamCount--;
          return 200;
        }else{
          LastActiveZaznam=ActiveZaznam;        
          ActiveZaznam=ActiveZaznam->next;

        }
      }
      return 404;
    }else{
      ActiveBoard=ActiveBoard->next;
    }
  }
  return 404;
}

//Changing content of record
//parameter NastenkaName - board where record will be changed
//parameter id - id of record to be changed
//parameter Content - content of updatet record
int UpdateZaznam(char* NastenkaName,int id,char* Content){
  Content=strdup(Content);
  nastenka* ActiveBoard=MainBoard->next; 
  while(ActiveBoard!=NULL){
    if(strcmp(ActiveBoard->jmeno,NastenkaName)==0){//Finding board
      zaznam* ActiveZaznam=ActiveBoard->zaznam;
      while(ActiveZaznam!=NULL){
        if(ActiveZaznam->id==id){//Finding record
          ActiveZaznam->text=Content;//Updating content
          return 200;
        }else{      
          ActiveZaznam=ActiveZaznam->next;
        }
      }      return 404;
    }else{
      ActiveBoard=ActiveBoard->next;
    }
  }
  return 404;
}

//Returns all records and their IDs from specified board
//parameter Name - board to be printed
char* PrintBoard(char* Name){
  char* result=malloc(1);
  nastenka* ActiveBoard=MainBoard->next;
  if(MainBoard->next==NULL){
    return "\\";
  }
  while(ActiveBoard!=NULL){
    if(strcmp(ActiveBoard->jmeno,Name)==0){//Finding specified board
      zaznam* ActiveZaznam=ActiveBoard->zaznam;
      while(ActiveZaznam!=NULL){//Reading all records with IDs
        result=realloc(result,strlen(result)+strlen(ActiveZaznam->text)*2+sizeof(int));
        sprintf(result,"%s%d:%s\n",result,ActiveZaznam->id,ActiveZaznam->text);
        ActiveZaznam=ActiveZaznam->next;
      }
      return result;
    }else{
      ActiveBoard=ActiveBoard->next;
    }
  }
  return "\\";

}
//Print names of all existing boards
char* PrintAllBoards(){
  char* result=malloc(1);
  nastenka* ActiveBoard=MainBoard->next;
  while(ActiveBoard!=NULL){//Reading all names of all existing boards
    result=realloc(result,strlen(result)+strlen(ActiveBoard->jmeno)*2);
    sprintf(result,"%s%s\n",result,ActiveBoard->jmeno);
    ActiveBoard=ActiveBoard->next;
  }
  return result;
}
