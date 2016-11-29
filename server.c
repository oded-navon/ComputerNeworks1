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
	//int ID;
	char from[MAX_CONTENT];
	char to[MAX_CONTENT];
	char subject[MAX_CONTENT];
	char content[MAX_CONTENT];
}email;

typedef struct USERDB
{
	char name[MAX_USERNAME];
	char password[MAX_PASSWORD];
	int numOfEmails;
	email emails[MAXMAILS];
}userDB;

int getUserIndexInDB(userDB* userdb,int userCnt,char* username);
bool onlyNumbers(char* s);
int sendall (int s, char *buf , int *len);
int recvall(int s, char *buf, int *len);
bool validateUserCred(char* filePath,char* userCred);
userDB* parseUserDB(char** filePath, int* userCnt);
int countUsers(char** filePath);
void printEmails(char* username, int userIndexInDB, int clientSocket, userDB* userdb);
void printEmailById(int emailId, int userIndexInDB, int clientSocket, userDB* userdb);
void deleteEmailById(int emailId, int userIndexInDB, userDB* userdb);
void insertEmailToDB(char* username, int userCount, int clientSocket, userDB* userdb);

int main(int argc, char** argv) {

	if (argc != 2 && argc != 3)
	{
		printf("Incorrect parameters, try again.\n");
		return -1;
	}
	printf("Print 0\n");
	if (argc==3 && !onlyNumbers(argv[2]))
	{
		printf("Incorrect port number, try again.\n");
		return -1;
	}
printf("Print 1\n");
	int* userCount = (int*)malloc(sizeof(int));
	userDB* userdb = parseUserDB(&argv[1],userCount);
	printf("Print 1.5\n");
	int serverSocket = socket(PF_INET,SOCK_STREAM,0);
	if (serverSocket <0)
	{
		printf("Error on opening socket: %s\n",strerror(errno));
		return -1;
	}
	int recvLen,bindCheck,listenCheck;
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
	printf("Print 2\n");
	//sockAddr.sin_addr = htonl(0x8443FC64);

	inet_aton("127.0.0.1", &sockAddr.sin_addr);

	bindCheck = bind(serverSocket,(struct sockaddr*) &sockAddr,sizeof(sockAddr));
	if (bindCheck <0)
	{
		printf("Error on binding socket: %s\n",strerror(errno));
		return -1;
	}

	listenCheck = listen(serverSocket,NUM_OF_CLIENTS);
	if (listenCheck <0)
	{
		printf("Error listening to socket: %s\n",strerror(errno));
		return -1;
	}
printf("Print 3\n");
	struct sockaddr_in clientAddr ={0};
	const char delim[2] = " ";
	const char delimTab[2] = "\t";


	int clientLen = sizeof(sockAddr);
	int* lenSent;
printf("Print 4\n");
	while(1)
	{
int clientSocket = accept(serverSocket,(struct sockaddr*) &clientAddr, &clientLen);
printf("Print 4.5\n");
		if (clientSocket <0)
		{
		printf("Error accepting client socket: %s\n",strerror(errno));
		return -1;
		}

		sendall(clientSocket,"Hey! I'm the coolest email server.",lenSent);
printf("Print 5\n");
		int retries = 5;
		char userCred[MAX_USERNAME+MAX_PASSWORD];
		char username[MAX_USERNAME];
		char userPass[MAX_PASSWORD];

		//Let the client try and connect for 5 times before disconnecting
		while (retries>0)
		{
			recvLen = recvall(clientSocket,&userCred,lenSent);
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
		recvLen = recvall(clientSocket,&msg,lenSent);
		if (recvLen <0)
		{
			printf("Error receiving client message: %s\n",strerror(errno));
			return -1;
		}


   		char *userCmd = strtok(msg, delim);
   		int emailId = atoi(strtok(msg, delim));

		while(strcmp(userCmd,"QUIT"))
		{
			if(strcmp(userCmd,"SHOW_INBOX"))
			{
				printEmails(username,getUserIndexInDB(userdb,*userCount,username),clientSocket,userdb);
			}
			else if(strcmp(userCmd,"GET_MAIL"))
			{
				printEmailById(emailId,getUserIndexInDB(userdb,*userCount,username),clientSocket,userdb);
			}
			else if(strcmp(userCmd,"DELETE_MAIL"))
			{
				deleteEmailById(emailId,getUserIndexInDB(userdb,*userCount,username),userdb);
			}
			else if(strcmp(userCmd,"COMPOSE"))
			{
				insertEmailToDB(username, userCount, clientSocket, userdb);
			}
		}

		close(clientSocket);

	}
	free(userCount);
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

//CREDIT: function taken from recitation
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

int recvall (int s, char *buf, int *len)
{
	int total = 0; /* how many bytes we've received */
	int bytesleft = *len; /* how many we have left to receive */
	int n;
	while(total < *len) {
	n = recv(s, buf+total, bytesleft, 0);
	if (n == -1) { break; }
	total += n;
	bytesleft -= n;
	}
	*len = total; /* return number actually sent here */
	return n == -1 ? -1:0; /*-1 on failure, 0 on success */
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

userDB* parseUserDB(char** filePath, int* userCnt)
{
	int userCount = countUsers(filePath);
	printf("Print 1.1\n");
	*userCnt = userCount;
	printf("Print 1.15\n");
	userDB* userdb = (userDB*)malloc(sizeof(userDB)*userCount);
	printf("Print 1.2\n");
	FILE* file = fopen(*filePath,"r");
	if (file == NULL)
	{
		printf("Error on opening username file: %s\n",strerror(errno));
		return NULL;
	}
	printf("Print 1.25\n");
	char line[MAX_LEN];
	const char delim[2] = "\t";
	char *userCmd;
	printf("Print 1.3\n");
	while(fgets(line, MAX_LEN, file) != NULL)
	{
		printf("Print 1.3\n");		
		userDB* localDB = (userDB*) malloc(sizeof(userDB));
		strcpy(localDB->name, strtok(line, delim));
		strcpy(localDB->password, strtok(line, delim));
	}
	fclose(file);
	return userdb;
}

int countUsers(char** filePath)
{
	FILE* file = fopen(*filePath,"r");
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

void printEmails(char* username, int userIndexInDB, int clientSocket, userDB* userdb)
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

void printEmailById(int emailId, int userIndexInDB, int clientSocket, userDB* userdb)
{
	char emailLine[MAX_CONTENT+MAX_USERNAME*TOTAL_TO+MAX_SUBJECT+MAX_USERNAME+10];
	
	char emailUsername[MAX_USERNAME];
	char emailSubject[MAX_SUBJECT];
	char emailTo[MAX_USERNAME*TOTAL_TO];
	char emailContent[MAX_CONTENT];
	
	email* userEmails = userdb[userIndexInDB].emails;
	int* lenSent;
	
	sprintf(emailUsername,"From: %s\n",userEmails[emailId].from);
	sprintf(emailTo,"To: %s\n",userEmails[emailId].to);
	sprintf(emailSubject,"Subject: %s\n",userEmails[emailId].subject);
	sprintf(emailContent,"Content: %s\n",userEmails[emailIdi].content);

	sprintf(emailLine,"%s%s%s%s",emailUsername,emailTo,emailSubject,emailContent);

	sendall(clientSocket,emailLine,lenSent);

	/*
	for (int i ; i<MAXMAILS; i++)
	{
		if (userEmails[i].ID == emailId)
		{
		
		sprintf(emailUsername,"From: %s\n",userEmails[i].from);
		sprintf(emailTo,"To: %s\n",userEmails[i].to);
		sprintf(emailSubject,"Subject: %s\n",userEmails[i].subject);
		sprintf(emailContent,"Content: %s\n",userEmails[i].content);
		
		sprintf(emailLine,"%s%s%s%s",emailUsername,emailTo,emailSubject,emailContent);
		
		sendall(clientSocket,emailLine,lenSent);
		
		break;	
		}
	}
	*/
}

void deleteEmailById(int emailId, int userIndexInDB, userDB* userdb)
{
	free(userdb[userIndexInDB].emails[emailId]);
	/*
	email* userEmails = userdb[userIndexInDB].emails;
	for (int i ; i<MAXMAILS; i++)
	{
		if (userEmails[i].ID == emailId)
		{
			free(userdb[userIndexInDB].emails[i]);
			break;
		}
	}
	*/
}

void insertEmailToDB(char* username, int userCount, int clientSocket, userDB* userdb)
{
	char emailSubject[MAX_SUBJECT];//= (char*) malloc(MAX_SUBJECT*sizeof(char));
	char emailTo[MAX_USERNAME*TOTAL_TO+20];
	char emailContent[MAX_CONTENT];
	char usersEmailIsSentTo[TOTAL_TO][MAX_USERNAME];

	email incomingEmail = (email) malloc(sizeof(email));
	int* lenSent;

	int recvLen = recvall(clientSocket, &emailTo, lenSent);
	if (recvLen <0)
	{
		printf("Error receiving client message: %s\n",strerror(errno));
		return -1;
	}
	strcpy(incomingEmail.to,emailTo);
	strcpy(incomingEmail.from,username);

	const char delim[2] = ",";
	char* recipient = strtok(emailTo, delim);

	int count;

	//Getting all the users the email was sent to, and inserting them into an array
	while(recipient != NULL)
	{
		usersEmailIsSentTo[count] = recipient;
		recipient = strtok(emailTo, delim);
		count++;
	}

	recvLen = recvall(clientSocket, &emailSubject, lenSent);
	if (recvLen <0)
	{
		printf("Error receiving client message: %s\n",strerror(errno));
		return -1;
	}

	strcpy(incomingEmail.subject,emailSubject);

	recvLen = recvall(clientSocket, &emailContent, lenSent);
	if (recvLen <0)
	{
		printf("Error receiving client message: %s\n",strerror(errno));
		return -1;
	}

	strcpy(incomingEmail.content,emailContent);

	//Go over all the users in the db
	for (int i ; i<userCount; i++)
	{
		//Go over all the users the email was sent to
		for (int j; j<count ; j++)
		{
			//Find a match between the arrays
			if (!strcmp(userdb[i].name,usersEmailIsSentTo[j]))
			{
				userdb[i].emails[userdb->numOfEmails] = incomingEmail;
				(userdb->numOfEmails)++;
				break;
			}
		}
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

