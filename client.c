#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>


#define MAX_LEN (1024)
#define bzero(b,len) (memset((b), '\0', (len)), (void) 0) //from http://pubs.opengroup.org/onlinepubs/009695399/functions/bzero.html
#define bcopy(b1,b2,len) (memmove((b2), (b1), (len)), (void) 0) //from http://pubs.opengroup.org/onlinepubs/009695399/functions/bcopy.html
#define DEFAULT_PORT 6423
#define TOTAL_TO 20
#define MAXMAILS 32000
#define MAX_USERNAME 50
#define MAX_PASSWORD 50
#define MAX_SUBJECT 100
#define MAX_CONTENT 100
#define NUM_OF_CLIENTS 20

int sendFullMsg(int sock, char* buf);
int recvFullMsg(int sock, char* buf);
int sendLenOfMsg(int sock, int len);
int recvLenOfMsg(int sock, int *len);
int recvall(int s, char *buf, int len);
int sendall(int s, char *buf, int len);




//taken from http://stackoverflow.com/questions/791982/determine-if-a-string-is-a-valid-ip-address-in-c
//written by user named Bill The Lizard

// int isValidIpAddress(char *ipAddress)
//{
//    struct sockaddr_in sa;
//    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
//    return (result != 0);
//}


char *trimwhitespace(char *str) //from stack overflow
{
    char *end;

    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end+1) = 0;

    return str;
}

int onlyNumbers(char* s)
{
    int i=0;
    int len = strlen(s);
    char temp;
    for(i=0; i<len; i++)
    {
        temp = *(s+i);
        if(!isdigit(temp))
        {
            return 0;
        }
    }
    return 1;
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

void closeHandlers(int sock, char* all, char* all2, char* all3){
    if(close(sock)<0)
    {
        printf("failed closing socket");
    }
    free(all);
    free(all2);
    free(all3);
}


int handleEvents(int sock)
{
    char* all = (char*)malloc(sizeof(char)*MAX_LEN);
    char* all2 = (char*)malloc(sizeof(char)*MAX_LEN);
    char* all3 = (char*)malloc(sizeof(char)*MAX_LEN);
    fgets(all, MAX_LEN, stdin);//get input from user
    all[strlen(all)-1]='\0';


    while(strcmp(all, "QUIT"))
    {
        if((!strcmp(all, "SHOW_INBOX")))
        {
            int numOfEmails = 0;
            if(sendFullMsg(sock, all) == -1)
            {
                printf("Error sending message to server");
                closeHandlers(sock, all, all2, all3);
                return 0;
            }

            if(recvFullMsg(sock,all) == -1){ //number of emails
                printf("Error accepting message from server");
                closeHandlers(sock, all, all2, all3);
                return 0;
            }

            numOfEmails = atoi(all);

            if (numOfEmails){
                for(int i=0; i< numOfEmails; i++)
                {
                    if(recvFullMsg(sock, all) == -1)
                    {
                        printf("Error showing inbox");
                        closeHandlers(sock, all, all2, all3);
                        return 0;
                    }
                    printf("%s", all);
                }
            }
        }

        else if(strstr(all, "GET_MAIL") != NULL)
        {
          int serial = atoi(all+strlen("GET_MAIL "));
            if((serial != 0) && (serial > 0) && (serial < MAXMAILS)){ //vaild serial - send it to server
                if(sendFullMsg(sock, all) == -1)
                {
                    printf("Error sending message to server");
                    closeHandlers(sock, all, all2, all3);
                    return 0;
                }
                if(recvFullMsg(sock, all) == -1){ //checking if server completed operation
                    printf("Error getting message from server");
                    closeHandlers(sock, all, all2, all3);
                    return 0;
                }

                if(strcmp(all, "FAIL")){
                        if(recvFullMsg(sock, all) == -1) //receive full message
                        {
                            printf("Error getting mail");
                            closeHandlers(sock, all, all2, all3);
                            return 0;
                        }
                        printf("%s", all); //print the mail itself
                }
            }
            else{
                printf("couldn't get specified mail. make sure you entered correct serial\n");
            }
        }

        else if(strstr(all, "DELETE_MAIL") != NULL)
        {
            int serial = atoi(all+strlen("DELETE_MAIL "));
            if((serial != 0) && (serial > 0) && (serial < MAXMAILS)){ //vaild serial - send it to server
                if(sendFullMsg(sock, all) == -1)
                {
                    printf("Error sending message to server\n");
                    closeHandlers(sock, all, all2, all3);
                    return 0;
                }
                if(recvFullMsg(sock, all) == -1){ //checking if received ok
                    printf("Error accepting message from server\n");
                    closeHandlers(sock, all, all2, all3);
                    return 0;
                }
                if(!strcmp(all, "FAIL")){
                    printf("couldn't delete specified mail. make sure you entered correct serial\n");
                    printf("please enter a new operation\n");
                }

            }

            else{ //invalid serial
                printf("specified ID of mail is invalid. please try again performing an operation\n");
            }
        }


        else if(!strcmp(all, "COMPOSE"))
        {

            if(sendFullMsg(sock, all) == -1) //send receive
            {
                printf("Error sending message to server");
                closeHandlers(sock, all, all2, all3);
                return 0;
            }
            fgets(all, MAX_LEN, stdin);
            strtok(all,": ");
            all = strtok(NULL,": ");
            all[strlen(all)-1]='\0';

            if(sendFullMsg(sock, all) == -1) //send To:
            {
                printf("Error in COMPOSE - couldn't send data to server");
                closeHandlers(sock, all, all2, all3);
                return 0;
            }

            fgets(all2, MAX_LEN, stdin);
            strtok(all2,":");
            all2 = strtok(NULL,":");
            all2[strlen(all2)-1]='\0';

            if(sendFullMsg(sock, all2+1) == -1) //send Subject:
            {
                printf("Error in COMPOSE - couldn't send data to server");
                closeHandlers(sock, all, all2, all3);
                return 0;
            }

            fgets(all3, MAX_LEN, stdin);
            all2 = strtok(all3,":");
            all3 = strtok(NULL,":");
            all3[strlen(all3)-1]='\0';

            if(sendFullMsg(sock, all3+1) == -1) //send Text:
            {
                printf("Error in COMPOSE - couldn't send data to server\n");
                closeHandlers(sock, all, all2, all3);
                return 0;
            }
            if(recvFullMsg(sock,all3) == -1){
                printf("Error in receiving data from server\n");
                closeHandlers(sock, all, all2, all3);
                return 0;
               }
            printf("%s",all3);
        }

        else
        {
            printf("please insert a valid command: SHOW_INBOX, GET_MAIL, DELETE_MAIL or COMPOSE\n");
        }

        fgets(all, MAX_LEN, stdin);
        all[strlen(all)-1]='\0';
    }

    if(sendFullMsg(sock,"QUIT") == -1){
        printf("Error in sending data to server\n");
    }

    closeHandlers(sock, all, all2, all3);
    return 0;
}


void freeAllMain(char* all,char* userName,char* password,char* all2){
    free(all);
    free(userName);
    free(password);
    free(all2);
}

int main(int argc, char *argv[])   //probably needs to get port number and host name
{
//we used https://www.youtube.com/watch?v=V6CohFrRNTo for help in writing
//the wrapping of the methods (defining the sockets and structs, nothing related to
//the business logic))
    int port = DEFAULT_PORT;
    char hostname[MAX_LEN] = "";
    char* all = (char*)malloc(sizeof(char)*MAX_LEN);
    char* userName = (char*)malloc(sizeof(char)*MAX_USERNAME);

    //char password[MAX_PASSWORD] = { '\0' };
    char* password = (char*)malloc(sizeof(char)*MAX_PASSWORD);
    char* all2 = (char*)malloc(sizeof(char)*MAX_LEN);

    int sock = socket(AF_INET, SOCK_STREAM, 0); //was PF_INET
    if(sock<0)
    {
        printf("ERROR opening socket");
        freeAllMain(all, userName, password, all2);
        return 0;
    }
    struct sockaddr_in serv_addr;
    struct hostent *server;
    if(argc > 3)   //
    {
        printf("too much arguments. exiting\n");
        close(sock);
        freeAllMain(all, userName, password, all2);
        return 0;
    }

    //update hostname
    if((argc == 2) || (argc == 3))
    {
        sprintf(hostname, "%s", *(argv+1));
    }
    else  //no arguments given
    {
        sprintf(hostname, "%s%s", hostname, "localhost");
    }

    //update port
    if(argc == 3)
    {
        if(onlyNumbers(argv[2])){
                port = atoi(argv[2]); //get port number.
                if((port > 65535) || (port < 0)){
                    printf("port is not in correct range. exiting\n");
                    close(sock);
                    freeAllMain(all, userName, password, all2);
                    return 0;
                }
        }
        else
        {
            printf("port is not a number. exiting\n");
            close(sock);
            freeAllMain(all, userName, password, all2);
            return 0;
        }
    }

    server=gethostbyname(hostname);
    if(server == NULL)
    {
        printf("problem with name of host. exiting\n");
        close(sock);
        freeAllMain(all, userName, password, all2);
        return 0;
    }

    //assigning parameters to socket
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    bcopy((char*) server->h_addr,(char*) &serv_addr.sin_addr.s_addr,server->h_length);

    if(connect(sock, (struct sockaddr*) &serv_addr,
               sizeof(serv_addr)) < 0)
    {
        printf("ERROR connecting to server\n");
        freeAllMain(all, userName, password, all2);
        return 0;
    }


    //parse user name and password from given text
    char* Rusername;
    char* Rpassword;

    if(recvFullMsg(sock, all) == -1)  //this is for accepting the server's welcome message
    {
        printf("ERROR receiving message from server\n");
        close(sock);
        freeAllMain(all, userName, password, all2);
        return 0;
    }
    printf("%s\n", all);

    fgets(userName, MAX_USERNAME, stdin); //get user name from user

    char delim[2] = " ";
    strtok(userName,delim);
    Rusername = strtok(NULL,delim); //actual username

    int de = strlen(Rusername);
    Rusername[de-1]='\0';

    fgets(password, MAX_PASSWORD, stdin); //get password from user

    strtok(password,delim);
    Rpassword = strtok(NULL,delim);
    Rpassword[strlen(Rpassword)-1]='\0';

    sprintf(all, "%s\t%s", Rusername, Rpassword);
    sendFullMsg(sock, all);
    recvFullMsg(sock, all);

    if(!strcmp(all, "FAIL")){
        printf("wrong username and password combination. program will now exit\n");
        close(sock);
        freeAllMain(all, userName, password, all2);
        return -1;
    }

    if(recvFullMsg(sock, all2) == -1){ //should be connected to server message
        printf("ERROR receiving message from server\n");
        freeAllMain(all, userName, password, all2);
        close(sock);

        return 0;
    }
    printf("%s\n",all2);
    handleEvents(sock);
    freeAllMain(all, userName, password, all2);
    return 1;
}



