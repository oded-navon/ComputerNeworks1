#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_LEN (1024)
#define DEFAULT_PORT (6423)
#define TOTAL_TO (20)
#define MAXMAILS (32000)
#define MAX_USERNAME (50)
#define MAX_PASSWORD (50)
#define MAX_SUBJECT (100)
#define MAX_CONTENT (2000)
#define NUM_OF_CLIENTS (20)

typedef struct EMAIL
{
	int index;	
	char from[MAX_CONTENT];
	char to[MAX_CONTENT];
	char subject[MAX_CONTENT];
	char text[MAX_CONTENT];
}email;

typedef struct USERDB
{
	char name[MAX_USERNAME];
	char password[MAX_PASSWORD];
	email emails[MAXMAILS];
}userDB;

int getUserIndexInDB(userDB* userdb,int userCnt,char* username);
bool onlyNumbers(char* s);
int sendall (int s, char *buf , int *len);
bool validateUserCred(char* filePath,char* userCred);
userDB* parseUserDB(char* filePath, int* userCnt);
int countUsers(char* filePath);
char* printEmails(char* username, int userIndexInDB, int clientSocket, userDB* userdb);



int main(int argc, char** argv) {

	if (argc != 2 && argc != 3)
	{
		printf("Incorrect parameters, try again.\n");
		return -1;
	}

	if (argc==3 && !onlyNumbers(argv[2]))
	{
		printf("Incorrect port number, try again.\n");
		return -1;
	}
	int* userCount;
	userDB* userdb = parseUserDB(argv[1],userCount);

	int serverSocket = socket(PF_INET,SOCK_STREAM,0);
	if (serverSocket <0)
	{
		printf("Error on opening socket: %s\n",strerror(errno));
		return -1;
	}

	struct sockaddr_in sockAddr;
	sockAddr.sin_family = AF_INET;
	if (argc==3)
	{
		sockAddr.sin_port = htons(atoi(argv[2]));
	}
	else
	{
		sockAddr.sin_port = htons(DEFAULT_PORT);
	}

	//sockAddr.sin_addr = htonl(0x8443FC64);

	inet_aton("127.0.0.1", &sockAddr.sin_addr);

	int bindCheck = bind(serverSocket,(struct sockaddr*) &sockAddr,sizeof(sockAddr));
	if (bindCheck <0)
	{
		printf("Error on binding socket: %s\n",strerror(errno));
		return -1;
	}

	int listenCheck = listen(serverSocket,NUM_OF_CLIENTS);
	if (listenCheck <0)
	{
		printf("Error listening to socket: %s\n",strerror(errno));
		return -1;
	}

	struct sockaddr_in clientAddr ={0};
	const char delim[2] = " ";
	const char delimTab[2] = "\t";


	int clientLen = sizeof(sockAddr);	

	while(1)
	{
int clientSocket = accept(serverSocket,(struct sockaddr*) &clientAddr, &clientLen);
		if (clientSocket <0)
		{
		printf("Error accepting client socket: %s\n",strerror(errno));
		return -1;
		}
		int* lenSent;
		sendall(clientSocket,"Hey! I'm the coolest email server.",lenSent);

		int retries = 5;
		char userCred[MAX_USERNAME+MAX_PASSWORD];
		char username[MAX_USERNAME];
		char userPass[MAX_PASSWORD];

		while (retries>0)
		{
			int recvLen = recv(clientSocket,&userCred,MAX_USERNAME+MAX_PASSWORD,0);
			if (recvLen <0)
				{
					printf("Error receiving client message: %s\n",strerror(errno));
					return -1;
				}
			strcpy(username,strtok(userCred, delimTab));
			strcpy(userPass,strtok(userCred, delimTab));

			if (!validateUserCred(argv[1],userCred))
			{
				retries--;
				sendall(clientSocket,"Incorrect username or password. Please try again.\n",lenSent);
			}
		}

		if (retries <1)
		{
			sendall(clientSocket,"Ran out of login attempts. Bye bye\n",lenSent);
			break;
		}

		sendall(clientSocket,"Connected to server",lenSent);

		char msg[MAX_LEN];
		int recvLen = recv(clientSocket,&msg,MAX_LEN,0);
			if (recvLen <0)
				{
					printf("Error receiving client message: %s\n",strerror(errno));
					return -1;
				}


   		char *userCmd = strtok(msg, delim);

		while(strcmp(userCmd,"QUIT"))
		{
			if(strcmp(userCmd,"SHOW_INBOX"))
			{
				printEmails(username,getUserIndexInDB(userdb,*userCount,username),clientSocket,userdb);
			}
			else if(strcmp(userCmd,"GET_MAIL"))
			{

			}
			else if(strcmp(userCmd,"DELETE_MAIL"))
			{

			}
			else if(strcmp(userCmd,"COMPOSE"))
			{

			}

			int recvLen = recv(clientSocket,&msg,MAX_LEN,0);
			if (recvLen <0)
				{
					printf("Error receiving client message: %s\n",strerror(errno));
					return -1;
				}
		}

		close(clientSocket);

	}

	close(serverSocket);
	return 0;
}

bool onlyNumbers(char* s){
	int i=0;
	int len = strlen(s);
	char temp;
	for(i=0;i<len;i++){
		temp = *(s+i);
		if(!isdigit(temp)){
			return false;
		}
	}
	return true;
}

//CREDIT: taken from recitation
//returns -1 on failure, 0 on success, and len is number of bytes sent
int sendall (int s, char *buf , int *len)
{
	int total = 0;
	int bytesleft=*len ;
	int n;

	while(total < * len )
	{
		n = send(s, buf+total, bytesleft, 0);
		if (n == -1) { break; }
		total += n;
		bytesleft -= n;
	}
	*len = total;
	return n == -1 ? -1:0;
}

bool validateUserCred(char* filePath,char* userCred)
{
	FILE* file = fopen(filePath,"r");
	if (file == NULL)
	{
		printf("Error on opening username file: %s\n",strerror(errno));
		return -1;
	}

	char line[MAX_LEN];
	while(fgets(line, MAX_LEN, file) != NULL)
	{
		if(!strcmp(line,userCred))
		{
			fclose(file);
			return true;
		}
	}
	fclose(file);
	return false;
}

userDB* parseUserDB(char* filePath, int* userCnt)
{
	int userCount = countUsers(filePath);
	*userCnt = userCount;
	userDB* userdb = (userDB*)malloc(sizeof(userDB)*userCount);

	FILE* file = fopen(filePath,"r");
	if (file == NULL)
	{
		printf("Error on opening username file: %s\n",strerror(errno));
		return NULL;
	}

	char line[MAX_LEN];
	const char delim[2] = "\t";
	char *userCmd;
	while(fgets(line, MAX_LEN, file) != NULL)
	{
		userDB* localDB = (userDB*) malloc(sizeof(userDB));
		strcpy(localDB->name, strtok(line, delim));
		strcpy(localDB->password, strtok(line, delim));
	}
	fclose(file);
	return userdb;
}

int countUsers(char* filePath)
{
	FILE* file = fopen(filePath,"r");
	if (file == NULL)
	{
		printf("Error on opening username file: %s\n",strerror(errno));
		return -1;
	}

	int count;
	char line[MAX_LEN];
	while(fgets(line, MAX_LEN, file) != NULL)
	{
		count++;
	}
	fclose(file);
	return count;

}

char* printEmails(char* username, int userIndexInDB, int clientSocket, userDB* userdb)
{
	char emailLine[MAX_SUBJECT+MAX_USERNAME+10];
	char emailNum[10];
	char emailUsername[MAX_USERNAME];
	char emailSubject[MAX_SUBJECT];
	email* userEmails = userdb[userIndexInDB].emails;
	int* lenSent;

	for (int i=1 ; i<MAXMAILS+1; i++)
	{
		sprintf(emailNum,"%d",i);
		strcpy(emailUsername,userEmails[i].from);
		strcpy(emailSubject,userEmails[i].subject);
		sprintf(emailLine,"%s\t%s \"%s\"",emailNum, emailUsername, emailSubject);
		sendall(clientSocket,emailLine,lenSent);
	}


}

int getUserIndexInDB(userDB* userdb,int userCnt,char* username)
{
	for (int i=0 ; i<userCnt ; i++)
	{
		if (!strcmp(userdb[i].name,username))
		{
			return i;
		}
	}
}

