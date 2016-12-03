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
    //mail id is the place in the array of the mail
    char from[MAX_CONTENT];
    char to[MAX_CONTENT];
    char subject[MAX_CONTENT];
    char content[MAX_CONTENT];
    bool deleted;
} email;

typedef struct USERDB
{
    char name[MAX_USERNAME];
    char password[MAX_PASSWORD];
    int numOfEmails;
    email emails[MAXMAILS];
} userDB;

int getUserIndexInDB(userDB* userdb,int userCnt,char* username);
bool onlyNumbers(char* s);
int sendall (int s, char *buf, int len);
int recvall(int s, char *buf, int len);
bool validateUserCred(char* filePath,char* userCred);
userDB* parseUserDB(char** filePath, int* userCnt);
int countUsers(char** filePath);
void printEmails(char* username, int userIndexInDB, int clientSocket, userDB* userdb);
void printEmailById(int emailId, int userIndexInDB, int clientSocket, userDB* userdb);
void deleteEmailById(int emailId, int userIndexInDB, userDB* userdb);
int insertEmailToDB(char* username, int userCount, int clientSocket, userDB* userdb);
int recvFullMsg(int sock, char* buf);
int sendFullMsg(int sock, char* buf);
int recvLenOfMsg(int sock, int *len);
int sendLenOfMsg(int sock, int len);



int main(int argc, char** argv)
{

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

    int userCount; // allocating inside function parseUserDB

    userDB* userdb = parseUserDB(&argv[1],&userCount);

    int serverSocket = socket(AF_INET,SOCK_STREAM,0);
    if (serverSocket <0)
    {
        printf("Error on opening socket: %s\n",strerror(errno));
        return -1;
    }

    int bindCheck,listenCheck;
    struct sockaddr_in sockAddr;
    sockAddr.sin_family = AF_INET; //Was AF_INET
    if (argc==3)
    {
        sockAddr.sin_port = htons(atoi(argv[2]));
    }
    else
    {
        sockAddr.sin_port = htons(DEFAULT_PORT);
    }

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
    struct sockaddr_in clientAddr = {0};
    const char delim[2] = " ";
    const char delimTab[2] = "\t";

    socklen_t clientLen = sizeof(sockAddr);

    while(1) //for each client
    {
        int clientSocket = accept(serverSocket,(struct sockaddr*) &clientAddr, &clientLen);

        if (clientSocket <0)
        {
            printf("Error accepting client socket: %s\n",strerror(errno));
            continue;
        }

        char* userCred = (char*)malloc(sizeof(char)*(MAX_USERNAME+MAX_PASSWORD));
        char* username = (char*)malloc(sizeof(char)*MAX_USERNAME);
        char* userPass = (char*)malloc(sizeof(char)*MAX_PASSWORD);

        int check = sendFullMsg(clientSocket,"Welcome! I am simple-mail-server.");
        if (check <0)
        {
            printf("Error sending client message: %s\nDisconnecting Client.\n",strerror(errno));
            close(clientSocket);
            free(userCred); free(username);free(userPass);
            continue;
        }
        check = recvFullMsg(clientSocket,userCred);
        if (check <0)
        {
            printf("Error receiving client message: %s\nDisconnecting Client.\n",strerror(errno));
            close(clientSocket);
            free(userCred); free(username);free(userPass);
            continue;
        }

        if (!validateUserCred(argv[1],userCred)) //validate tht user name and password is correct
        {
            sendFullMsg(clientSocket,"FAIL");
            printf("wrong username & password! Disconnecting client\n");
            close(clientSocket);
            free(userCred); free(username);free(userPass);
            continue;
        }
        else{
            sendFullMsg(clientSocket,"SUCCESS");
            strcpy(username,strtok(userCred, delimTab));
            strcpy(userPass,strtok(NULL, delimTab));
        }

        check = sendFullMsg(clientSocket,"Connected to server");

        if (check <0)
        {
            printf("Error sending client message: %s\nDisconnecting Client.\n",strerror(errno));
            close(clientSocket);
            free(userCred); free(username);free(userPass);
            continue;
        }

        char* msg = (char*)malloc(sizeof(char)*MAX_LEN);

        check = recvFullMsg(clientSocket,msg);
        if (check <0)
        {
            printf("Error receiving client message: %s\nDisconnecting Client.\n",strerror(errno));
            close(clientSocket);
            free(userCred); free(username);free(userPass); free(msg);
            continue;
        }

        char* userCmd = strtok(msg, delim);

        int emailId;
        int userIndex = getUserIndexInDB(userdb,userCount,username);

        while(strcmp(userCmd,"QUIT"))
        {
            if(!strcmp(userCmd,"SHOW_INBOX"))
            {
                printEmails(username,userIndex,clientSocket,userdb);
            }
            else if(!strcmp(userCmd,"GET_MAIL"))
            {
                emailId = atoi(strtok(NULL, delim)); // turn into number the next word in string
                if(userdb[userIndex].emails[emailId-1].deleted)
                {
                    sendFullMsg(clientSocket,"FAIL");
                }
                else
                {
                    sendFullMsg(clientSocket,"SUCCESS");
                    printEmailById(emailId,userIndex,clientSocket,userdb);
                }
            }
            else if(!strcmp(userCmd,"DELETE_MAIL"))
            {
                emailId = atoi(strtok(NULL, delim)); // turn into number the next word in string
                if(userdb[userIndex].emails[emailId-1].deleted)
                {
                    sendFullMsg(clientSocket,"FAIL");
                }
                else
                {
                    sendFullMsg(clientSocket,"SUCCESS");
                    deleteEmailById(emailId,userIndex,userdb);
                }
            }
            else if(!strcmp(userCmd,"COMPOSE"))
            {
                if(insertEmailToDB(username, userCount, clientSocket, userdb) == -1)
                {
                    printf("Error inserting email into DB. Disconnecting client.\n");
                    close(clientSocket);
                    free(userCred); free(username);free(userPass); free(msg);
                    continue;
                }
            }
            check = recvFullMsg(clientSocket,msg);
            userCmd = strtok(msg, delim);
        }

        close(clientSocket);

    }
    free(userdb);
    close(serverSocket);
    return 0;
}

int sendFullMsg(int sock, char* buf)
{
    int lenSent = strlen(buf)+1;
    int sendCheck = sendLenOfMsg(sock,lenSent);
    if (sendCheck <0)
        {
            printf("Error sending client message size: %s\n",strerror(errno));
            return -1;
        }

    sendCheck = sendall(sock,buf,lenSent);
    if (sendCheck <0)
        {
            printf("Error sending client message: %s\n",strerror(errno));
            return -1;
        }
    return 0;
}

int recvFullMsg(int sock, char* buf)
{
    int recvLen = 0;
	if (recvLenOfMsg(sock, &recvLen) != 0) return -1;
	if (recvall(sock, buf, recvLen) != 0) return -1;

	return 0;
}

int sendLenOfMsg(int sock, int len)
{
	if (send(sock, &len, sizeof( len ),0) != 4)
	{
		return -1;
	}
	return 0;
}

int recvLenOfMsg(int sock, int *len)
{
    if (recv(sock, len, sizeof(*len), 0) != 4)
    {
     	return -1;
    }
    return 0;
}

int recvall(int s, char *buf, int len)
{
	//total = how many bytes we've sent, bytesleft =  how many we have left to send
	int total = 0, n, bytesleft = len;

	while(total < len)
	{
		n = recv(s, buf+total, bytesleft, 0);
		if ((n == -1) || (n == 0))
		{
			return -1;
		}
		total += n;
		bytesleft -= n;
	}
	//-1 on failure, 0 on success
	return 0;
}

int sendall(int s, char *buf, int len)
{
	/* total = how many bytes we've sent, bytesleft =  how many we have left to send */
	int total = 0, n, bytesleft = len;

	while(total < len)
	{
		n = send(s, buf+total, bytesleft, MSG_NOSIGNAL);
		if (n == -1)
		{
			break;
		}
		total += n;
		bytesleft -= n;
	}
	/*-1 on failure, 0 on success */
	return n == 1 ? -1:0;
}

bool onlyNumbers(char* s){
    int i=0;
    int len = strlen(s);
    char temp;
    for(i=0; i<len; i++)
    {
        temp = *(s+i);
        if(!isdigit(temp))
        {
            return false;
        }
    }
    return true;
}

bool validateUserCred(char* filePath,char* userCred){
    FILE* file = fopen(filePath,"r");
    if (file == NULL)
    {
        printf("Error on opening username file: %s\n",strerror(errno));
        return false;
    }

    char line[MAX_LEN];
    while(fgets(line, MAX_LEN, file) != NULL)
    {
        //line[strlen(line)-1]='\0';
        sprintf(line, "%s", strtok(line, "\n"));
        if(!strcmp(line,userCred))
        {
            fclose(file);
            return true;
        }
    }
    fclose(file);
    return false;
}

userDB* parseUserDB(char** filePath, int* userCnt){
    int userCount = countUsers(filePath);
    *userCnt = userCount;
    userDB* userdb = (userDB*)malloc(sizeof(userDB)*userCount);
    if(userdb == NULL)
    {
        printf("error on malloc for userDb array: %s\n",strerror(errno));
        return NULL;
    }
    FILE* file = fopen(*filePath,"r");
    if (file == NULL)
    {
        printf("Error on opening username file: %s\n",strerror(errno));
        return NULL;
    }
    char line[MAX_LEN];
    const char delim[2] = "\t";

    int i=0;
    while(fgets(line, MAX_LEN, file) != NULL) //fill in userDb array
    {
        if (line[strlen(line)-1]=='\n')
        {
            line[strlen(line)-1]='\0';
        }

        strcpy(userdb[i].name, strtok(line, delim));
        strcpy(userdb[i].password, strtok(NULL, delim));

        for (int j=0 ; j<MAXMAILS ; j++)
        {
            userdb[i].emails[j].deleted = true;
        }

        userdb[i].numOfEmails=0;
        i++;
    }
    fclose(file);
    return userdb;
}

int countUsers(char** filePath){
    FILE* file = fopen(*filePath,"r");
    if (file == NULL)
    {
        printf("Error on opening username file: %s\n",strerror(errno));
        return -1;
    }

    int count=0;
    char line[MAX_LEN];
    while(fgets(line, MAX_LEN, file) != NULL)
    {
        count++;
    }
    fclose(file);
    return count;
}

void printEmails(char* username, int userIndexInDB, int clientSocket, userDB* userdb){
    char emailLine[MAX_SUBJECT+MAX_USERNAME+10];
    char emailNum[10];
    char emailUsername[MAX_USERNAME];
    char emailSubject[MAX_SUBJECT];
    email* userEmails = userdb[userIndexInDB].emails;

    char numOfEmails[5];
    sprintf(numOfEmails,"%d",userdb[userIndexInDB].numOfEmails);
    numOfEmails[5] = '\0';

    int check = sendFullMsg(clientSocket,numOfEmails);
    if (check <0)
    {
        printf("Error sending client message: %s\nDisconnecting Client.\n",strerror(errno));
        close(clientSocket);
        return;
    }

    if (userdb[userIndexInDB].numOfEmails)
    {
      for (int i=0 ; i<MAXMAILS; i++)
        {
            if(!userEmails[i].deleted)
            {
            sprintf(emailNum,"%d",i+1);
            strcpy(emailUsername,userEmails[i].from);
            strcpy(emailSubject,userEmails[i].subject);
            sprintf(emailLine,"%s\t%s\t'%s'\n",emailNum, emailUsername, emailSubject);
            check = sendFullMsg(clientSocket,emailLine);
                if (check <0)
                {
                printf("Error sending client message: %s\nDisconnecting Client.\n",strerror(errno));
                close(clientSocket);
                return;
                }
            }
        }
    }
}

void printEmailById(int emailId, int userIndexInDB, int clientSocket, userDB* userdb){

    char emailLine[MAX_CONTENT+MAX_USERNAME*TOTAL_TO+MAX_SUBJECT+MAX_USERNAME];

    char emailUsername[MAX_USERNAME];
    char emailSubject[MAX_SUBJECT];
    char emailTo[MAX_USERNAME*TOTAL_TO];
    char emailContent[MAX_CONTENT];

    email* userEmails = userdb[userIndexInDB].emails;

    //The emailId is minus 1 because we use the place in the array as the email ID
    sprintf(emailUsername,"From: %s\n",userEmails[emailId-1].from);
    sprintf(emailTo,"To: %s\n",userEmails[emailId-1].to);
    sprintf(emailSubject,"Subject: %s\n",userEmails[emailId-1].subject);
    sprintf(emailContent,"Content: %s\n",userEmails[emailId-1].content);

    sprintf(emailLine,"%s%s%s%s",emailUsername,emailTo,emailSubject,emailContent);

    int check = sendFullMsg(clientSocket,emailLine);
    if (check <0)
    {
        printf("Error sending client message: %s\nDisconnecting Client.\n",strerror(errno));
        close(clientSocket);
        return;
    }
}

void deleteEmailById(int emailId, int userIndexInDB, userDB* userdb){
    //deleting by setting the email "deleted" value to TRUE
    if(!(userdb[userIndexInDB].emails[emailId-1].deleted)){
        userdb[userIndexInDB].emails[emailId-1].deleted = true;
        userdb[userIndexInDB].numOfEmails--;
    }

}

int insertEmailToDB(char* username, int userCount, int clientSocket, userDB* userdb){

    char* emailSubject = (char*) malloc(MAX_SUBJECT*sizeof(char));
    char* emailTo = (char*) malloc((MAX_USERNAME*TOTAL_TO+20)*sizeof(char));
    char* emailContent = (char*) malloc(MAX_CONTENT*sizeof(char));


    char** usersEmailIsSentTo = (char**)malloc(sizeof(char*)*TOTAL_TO);
    int i;
    for(i=0;i<TOTAL_TO;i++){
        usersEmailIsSentTo[i] = (char*)malloc(sizeof(char)*MAX_USERNAME);
    }

    email* incomingEmail = (email*)malloc(sizeof(email));
    incomingEmail->deleted = false;

    int recvLen = recvFullMsg(clientSocket, emailTo);
    if (recvLen <0)
    {
        printf("Error receiving client message: %s\n",strerror(errno));
        for(i=0;i<TOTAL_TO;i++){
            free(usersEmailIsSentTo[i]);
        }
        free(usersEmailIsSentTo);free(incomingEmail); free(emailContent); free(emailSubject); free(emailTo);
        return -1;
    }

    strcpy(incomingEmail->to,emailTo);
    strcpy(incomingEmail->from,username);

//    strtok(emailTo," "); //remove "To:" from to: itai

    const char delim[2] = ",";
    char* recipient;
    int count=0;
    if(strstr(emailTo, ",") != NULL)
    {
        //getting here means there is more than one recipient
        recipient = strtok(emailTo, delim);
        //Getting all the users the email was sent to, and inserting them into an array

        while(recipient != NULL)
        {
        strcpy(usersEmailIsSentTo[count],recipient);
        count++;
        recipient = strtok(NULL, delim);
        }
    }
    else
    {
        //getting here means there is only one recipient
        recipient = emailTo;
        strcpy(usersEmailIsSentTo[count],recipient);
        count=1;
    }

    recvLen = recvFullMsg(clientSocket, emailSubject); //get the subject
    if (recvLen <0)
    {
        printf("Error receiving client message: %s\n",strerror(errno));
        for(i=0;i<TOTAL_TO;i++)
        {
            free(usersEmailIsSentTo[i]);
        }
        free(usersEmailIsSentTo);free(incomingEmail); free(emailContent); free(emailSubject); free(emailTo);
        return -1;
    }

    strcpy(incomingEmail->subject,emailSubject);

    recvLen = recvFullMsg(clientSocket, emailContent);

    if (recvLen <0)
    {
        printf("Error receiving client message: %s\n",strerror(errno));
        for(i=0;i<TOTAL_TO;i++)
        {
            free(usersEmailIsSentTo[i]);
        }
        free(usersEmailIsSentTo);free(incomingEmail); free(emailContent); free(emailSubject); free(emailTo);
        return -1;
    }

    strcpy(incomingEmail->content,emailContent);

    //Go over all the users in the db
    for (int i=0 ; i<userCount; i++)
    {
        //Go over all the users the email was sent to
        for (int j=0; j<count ; j++)
        {
            //Find a match between the arrays
            if (!strcmp(userdb[i].name,usersEmailIsSentTo[j]))
            {
                //Insert the email to the emails array of all the users it was sent to
                strcpy(userdb[i].emails[userdb[i].numOfEmails].to,incomingEmail->to);
                strcpy(userdb[i].emails[userdb[i].numOfEmails].subject,incomingEmail->subject);
                strcpy(userdb[i].emails[userdb[i].numOfEmails].from,incomingEmail->from);
                strcpy(userdb[i].emails[userdb[i].numOfEmails].content,incomingEmail->content);
                userdb[i].emails[userdb[i].numOfEmails].deleted = false;
                userdb[i].numOfEmails = (userdb[i].numOfEmails)+1;
                break;
            }
        }
    }

    //free resources
    for(i=0;i<TOTAL_TO;i++){
        free(usersEmailIsSentTo[i]);
    }
    free(usersEmailIsSentTo);free(incomingEmail); free(emailContent); free(emailSubject); free(emailTo);
    sendFullMsg(clientSocket,"Mail sent\n");
    return 0;
}

int getUserIndexInDB(userDB* userdb,int userCnt,char* username){
    for (int i=0 ; i<userCnt ; i++)
    {
        if (!strcmp(userdb[i].name,username))
        {
            return i;
        }
    }
    return -1; //if wasnt found returns 1
}


