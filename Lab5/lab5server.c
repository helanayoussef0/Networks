/* time_server.c - main */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#define BUFLEN		100	/* buffer length */


/*------------------------------------------------------------------------
 * main - Iterative UDP server for TIME service
 *------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
	struct  sockaddr_in fsin;	/* the from address of a client	*/
	char	buf[100];		/* "input" buffer; any size > 0	*/
	char    *pts;
	int	sock;			/* server socket		*/
	time_t	now;			/* current time			*/
	int	alen;			/* from-address length		*/
	struct  sockaddr_in sin; /* an Internet endpoint address         */
    	int     s, type;        /* socket descriptor and socket type    */
	int 	port=3000;
    	char	fname[100];
	int 	n;
	FILE *fp;
	char buff[BUFLEN];
	char data[BUFLEN-1];
	int totalbytes;
	

	switch(argc){
		case 1:
			break;
		case 2:
			port = atoi(argv[1]);
			break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}

        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = htons(port);
                                                                                                 
    /* Allocate a socket */
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0)
		fprintf(stderr, "can't creat socket\n");
                                                                                
    /* Bind the socket */
        if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "can't bind to %d port\n",port);
        listen(s, 5);	
	alen = sizeof(fsin);
	while (1) {
		strcpy(buff,"");
		strcpy(fname,"");
		strcpy(data,"");
		bzero(buff,BUFLEN);
		bzero(fname,100);
		bzero(data,BUFLEN-1);
		if (recvfrom(s, fname, sizeof(buf), 0,(struct sockaddr *)&fsin, &alen) < 0){
			fprintf(stderr, "recvfrom error\n");
		}
		else{
			printf("%s\n",fname);
			fp = fopen(fname,"rb");
			if(fp == NULL){
				perror("Error");
				buff[0] = 'E';
				strcat(buff,"File does not exist");
				(void) sendto(s, buff, strlen(buff), 0,(struct sockaddr *)&fsin, sizeof(fsin));
			}
			else{
				printf("file is not null\n");
				buff[0] = 'C';
				strcat(buff,fname);
				(void) sendto(s, buff, strlen(buff), 0,(struct sockaddr *)&fsin, sizeof(fsin));
				totalbytes=0;
				while(1){
					/* First read file in chunks of 100 bytes */
					bzero(buff,BUFLEN);
					bzero(data,BUFLEN-1);
					buff[0] = 'D';
					int nread = fread(data,1,BUFLEN-1,fp);
					strcat(buff,data);
					printf("Bytes read %d \n", nread);
					totalbytes = totalbytes + nread;        
					/* If read was success, send data. */
					if(nread > 0){
						if (nread < (BUFLEN-1)){
							if (feof(fp)){
								printf("End of file, total bytes sent is %d\n",totalbytes);
								buff[0] = 'F';
								(void) sendto(s, buff, strlen(buff), 0,(struct sockaddr *)&fsin, sizeof(fsin));
								break;
							}
							if (ferror(fp))
								printf("Error reading\n");
								break;
					}
					printf("Sending \n");
					(void) sendto(s, buff, strlen(buff), 0,(struct sockaddr *)&fsin, sizeof(fsin));
					}
				}
			}
		}
	}
}
