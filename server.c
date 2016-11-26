#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>

#define BACKLOG (19)
#define MAX_LEN (1024)

typedef struct EMAIL
{
	char from[MAX_LEN];
	char to[MAX_LEN];
	char subject[100],;
	char text[2000];
}email;

typedef struct USERDB
{
	char name[MAX_LEN];
	char password[MAX_LEN];
	email emails[32000];
}userDB;


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

	int socket = socket(PF_INET,SOCK_STREAM,0);
	if (socket <0)
	{
		printf("Error on opening socket: %s\n",strerror(errno));
		return -1;
	}

	struct sockaddr_in sockAddr ={0};
	sockAddr.sin_family = AF_INET;
	if (argc==3)
	{
		sockAddr.sin_port = argv[2];
	}
	else
	{
		sockAddr.sin_port = 6423;
	}

	inet_aton("127.0.0.1", &sockAddr.sin_addr);

	int bindCheck = bind(socket,&sockAddr,sizeof(sockAddr));
	if (bindCheck <0)
	{
		printf("Error on binding socket: %s\n",strerror(errno));
		return -1;
	}

	int listenCheck = listen(socket,BACKLOG);
	if (listenCheck <0)
	{
		printf("Error listening to socket: %s\n",strerror(errno));
		return -1;
	}

	struct sockaddr_in clientAddr ={0};
	const char delim[2] = " ";
	const char delimTab[2] = "\t";
	while(1)
	{
		int clientSocket = accept(socket,(struct sockadr*) &clientAddr, sizeof(struct sockaddr_in));
		if (clientSocket <0)
		{
		printf("Error accepting client socket: %s\n",strerror(errno));
		return -1;
		}
		int* lenSent;
		sendall(clientSocket,"Hey! I'm the coolest email server.",lenSent);

		int retries = 5;
		char* userCred[MAX_LEN];
		char* username;
		char* userPass;
		while (retries>0)
		{
			int recvLen = recv(clientSocket,&userCred,MAX_LEN,0);
			if (recvLen <0)
				{
					printf("Error receiving client message: %s\n",strerror(errno));
					return -1;
				}
			username = strtok(userCred, delimTab);
			userPass = strtok(userCred, delimTab);
			if (!validateUserCred(argv[1],userCred))
			{
				retries--;
				sendall(clientSocket,"Incorrect username\password. Please try again.\n",lenSent);
			}
		}

		if (retries <1)
		{
			sendall(clientSocket,"Ran out of login attempts. Bye bye\n",lenSent);
			break;
		}

		sendall(clientSocket,"Connected to server",lenSent);

		char* msg[MAX_LEN];
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
				printEmails(username,getUserIndexInDB(userdb,userCount,username),clientSocket);
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

	close(socket);
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
			close(file);
			return true;
		}
	}
	close(file);
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
		return -1;
	}

	char line[MAX_LEN];
	const char delim[2] = "\t";
	char *userCmd;
	while(fgets(line, MAX_LEN, file) != NULL)
	{
		userDB localDB = (userDB)malloc(sizeof(userDB));
		strcpy(localDB.name, strtok(line, delim));
		strcpy(localDB.password, strtok(line, delim));
	}
	close(file);
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
	close(file);
	return count;

}

char* printEmails(char* username, int userIndexInDB, socket clientSocket)
{


	for (int i=1 ; i<32001; i++)
	{

	}
	int* lenSent;
	sendall(clientSocket,"Hey! I'm the coolest email server.",lenSent);
}

int getUserIndexInDB(userDB* userdb,int userCnt,char* username)
{
	for (int i=0 ; i<userCnt ; i++)
	{
		if (!strcmp(userDB[i].name,username))
		{

		}
	}
}
