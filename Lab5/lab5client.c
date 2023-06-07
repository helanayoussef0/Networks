/* time_client.c - main */

#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>                                                                            
#include <netinet/in.h>
#include <arpa/inet.h>
#include<stdbool.h>                                                                                 
#include <netdb.h>

#define	BUFLEN 100

#define	MSG		"Any Message \n"


/*------------------------------------------------------------------------
 * main - UDP client for TIME service that prints the resulting time
 *------------------------------------------------------------------------
 */
int
main(int argc, char **argv)
{
	char	*host = "localhost";
	int	port = 3000;
	char	now[100];	   /* 32-bit integer to hold time	*/ 
	struct 	hostent	*phe;	   /* pointer to host information entry*/
	struct 	sockaddr_in sin;   /* an Internet endpoint address*/
	int	s, n, type;	   /* socket descriptor and socket type	*/
	char 	buff[BUFLEN];
	int	alen;		   /* from-address length	*/

	/* Create a file where data will be stored */
	FILE *fp;
	char fname[100];
	int bytesReceived = 0;
	bool eofFlag = false;
	int totalBytes=0;
	int packetNumber = 0;
	
	switch (argc) {
	case 1:
		break;
	case 2:
		host = argv[1];
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "usage: UDPtime [host [port]]\n");
		exit(1);
	}

	memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;                                                                
        sin.sin_port = htons(port);
                                                                                        
    /* Map host name to IP address, allowing for dotted decimal */
    if ( phe = gethostbyname(host) ){
            memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
    }
    else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
		fprintf(stderr, "Can't get host entry \n");
                                                                                
    /* Allocate a socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
		fprintf(stderr, "Can't create socket \n");
	
                                                                                
    /* Connect the socket */
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
		fprintf(stderr, "Can't connect to %s %s \n", host, "Time");
		exit(0);
	}
	while(1){
	strcpy(buff,"");
	bzero(buff,BUFLEN);
	bytesReceived = 0;
	packetNumber = 0;
	totalBytes = 0;
	eofFlag = false;
	/* Ask user for the file they want */
	printf("What file do you want to recieve from the server? (Q to quit) \n");
	n=read(0, buff, BUFLEN);					/* get user message */
	
	if(buff[strlen(buff)-1] = '\n')			//check for the "Enter" (\n) symbol from the recieved text
		buff[strlen(buff)-1] = '\0';    //Replace the "Enter" (\n) with a "terminator" (\0)
	if (buff[0] == 'Q' || buff[0] == 'q')
		return 0;
	/* send file name to server */
	(void) sendto(s, buff, strlen(buff), 0,(struct sockaddr *)&sin, sizeof(sin));
	/* infinit loop for recieving data */
	while (1){
		/* empty buffer */
		strcpy(buff,"");
		bzero(buff,BUFLEN);
		/* recieve data from server */
		bytesReceived = read(s, (char *)&buff, BUFLEN) - 1;
		//printf("recieved data is %s\n",buff);
		/* If data sent is an error */
		if (buff[0] == 'E'){
			printf("File not found, try again\n");
			break;
		}
		/* if data sent is the file name */
		else if (buff[0] == 'C'){
			memmove(buff, buff+1, strlen(buff));
			printf("File Name: %s\n",buff);
			fp = fopen(buff, "wb");
			if(fp == NULL){
				printf("Error opening file\n");
				perror("Error");
				return 1;
			}
		}
		else if ((buff[0] == 'D') || buff[0] == 'F'){
			if (buff[0] == 'F')
				eofFlag = true;
			memmove(buff, buff+1, strlen(buff));
			packetNumber++;
			//bytesReceived = sizeof(buff) - 1;
			totalBytes += bytesReceived;
			fwrite(buff, 1,bytesReceived,fp);
			printf("bytes recieved on packet %d are: %d\n",packetNumber, bytesReceived);
			if (eofFlag){
				printf("Total bytes recieved are: %d\n", totalBytes);
				break;
				fclose(fp);
			}
		}
	}
	}
}
