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


 //taken from http://stackoverflow.com/questions/791982/determine-if-a-string-is-a-valid-ip-address-in-c
 //written by user named Bill The Lizard

// int isValidIpAddress(char *ipAddress)
//{
//    struct sockaddr_in sa;
//    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
//    return (result != 0);
//}


int onlyNumbers(char* s){
	int i=0;
	int len = strlen(s);
	char temp;
	for(i=0;i<len;i++){
		temp = *(s+i);
		if(!isdigit(temp)){
			return 0;
		}
	}
	return 1;
}

//Code considering partial sending - taken from recitation 2
int sendall(int s, char *buf, int *len) {
	int total = 0; /* how many bytes we've sent */
	int bytesleft = *len; /* how many we have left to send */
	int n;
	while(total < *len) {
	n = send(s, buf+total, bytesleft, 0);
	if (n == -1) { break; }
	total += n;
	bytesleft -= n;
	}
	*len = total; /* return number actually sent here */
	return n == -1 ? -1:0; /*-1 on failure, 0 on success */
}

 int recvall(int s, char *buf, int *len) {
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

int main(int argc, char *argv[]) { //probably needs to get port number and host name
//we used https://www.youtube.com/watch?v=V6CohFrRNTo for help in writing
//the wrapping of the methods (defining the sockets and structs, nothing related to
//the business logic))
	int port = DEFAULT_PORT;
	char hostname[MAX_LEN] = "";
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock<0){
		printf("ERROR opening socket");
		return 0;
	}
	struct sockaddr_in serv_addr;
	struct hostent *server;
	if(argc > 3) { //
		printf("too much arguments");
		close(sock);
		return 0;
	}

	//update hostname
	if((argc == 2) || (argc == 3)){
		sprintf(hostname, "%s%s", hostname, *(argv+1));
	}
	else{ //no arguments given
        printf("there were no arguments\n");
		sprintf(hostname, "%s%s", hostname, "localhost");
	}

	//update port
	if(argc == 3){
		if(onlyNumbers(argv[2])) port = atoi(argv[2]); //get port number
	}

	server=gethostbyname(hostname);
	if(server == NULL){
		printf("problem with name of host");
		close(sock);
		return 0;
	}

	//assigning parameters to socket
	//bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
//	serv_addr.sin_addr = htonl(hostname);
	bcopy((char*) server->h_addr,(char*) &serv_addr.sin_addr.s_addr,server->h_length);

	if(connect(sock, (struct sockaddr*) &serv_addr,
	sizeof(serv_addr)) < 0){
		printf("ERROR connecting to server");
		return 0;
	}

	//parse user name and password from given text
	char all[MAX_LEN] = { '\0' };
	int howMuchWritten = MAX_LEN;
	char userName[MAX_USERNAME] = { '\0' };
	char temp[MAX_LEN] = { '\0' };
	char password[MAX_PASSWORD] = { '\0' };

	recvall(sock, all, &howMuchWritten);
	printf("%s\n", all);
//
//	fgets(userName, MAX_USERNAME, stdin); //get user name from user
////	char *pos;
////	if ((pos=strchr(userName, '\n')) != NULL) //move past "User: "
////		*pos = '\0';
//	memcpy(all, &userName[6], strlen(userName)-7);
//	all[strlen(userName)-6] = '\0';
//	printf("username is:%s\n", all);
//	fgets(password, MAX_LEN, stdin); //get password from user
////	if ((pos=strchr(userName, '\n')) != NULL)
////		*pos = '\0';
//	memcpy(temp, &password[10], strlen(password)-10); //get rid of "password: "
//	sprintf(all, "username+password is: %s\t%s\n", all, temp);
//
//	printf("connected to server");

    size_t len;
    while(1){
        fgets(all, MAX_LEN, stdin);
        if((!strcmp(all, "SHOW_INBOX"))){
            sendall(sock, all, strlen(all));

        }

        recvall(sock, )
    }
 	int closeVal = close(sock);
	if(closeVal<0){
		printf("failed closing socket");
		return 0;
	}

	return 0;
}



