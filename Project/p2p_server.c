#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>
#define BUFLEN		100	/* buffer length */
#include <stddef.h> // null
#include <stdbool.h> // bool
#include <time.h> // rand
/*------------------------------------------------------------------------
 * main - P2P server
 *------------------------------------------------------------------------
 */
 typedef struct Content{
    char fname[20]; 	//file name
    char ipAddress[20]; //ip address
    char port[10];		//port number
    char peer_name[10];	//user name
}Content;

struct pdu {
   char type;
   char data[100];
};

void append(Content * add){
    FILE *fp;
    fp = fopen("content.txt","a");
    fwrite(add,sizeof(Content),1,fp);
    fclose(fp);
}

Content * search(Content * searchFor) {
    Content * index = (Content*)malloc(sizeof(Content));
    FILE *fp;
    fp = fopen("content.txt","r");

    if(fp == NULL) {
        return NULL;
    }

    while(fread(index,sizeof(Content),1,fp)){
        if (strncmp(index->fname, searchFor->fname, strlen(index->fname)) == 0
            && strncmp(index->peer_name, searchFor->peer_name, strlen(index->peer_name))==0) {
            fclose(fp);
			return index;
        }
    }

    free(index);
    fclose(fp);
    return NULL;
}

Content * searchServer(char * fileName) {
    Content *index = (Content*)malloc(sizeof(Content));
    Content * matches[5];

    int currentMatch = 0;

    FILE *fp;
    fp = fopen("content.txt","r");

    if(fp == NULL) {
        return NULL;
    }

    while(fread(index,sizeof(Content),1,fp)){
        if (strcmp(index->fname, fileName) == 0) {
            if(currentMatch == 5) {
                printf("Too many servers for file %s\n", fileName);
            }
            matches[currentMatch] = index;
            currentMatch += 1;
        }
    }

    if(currentMatch == 0) {
        return NULL;
    }

    free(index);
    fclose(fp);

    int randomMatch = rand() % currentMatch;

    return matches[randomMatch];
}

 void update(char* file,char* addr, char* p){
    int found=0;
    Content s1;
    FILE *fp, *fp1;
    char searchFile[20];
    strcpy(searchFile,file);
    fp = fopen("content.txt","r");
    fp1 = fopen("temp.txt","w");
    while(fread(&s1,sizeof(Content),1,fp)){
        if(strcmp(s1.fname,searchFile)==0){
            found = 1;
            fflush(stdin);
            strcpy(s1.ipAddress,addr);
			strcpy(s1.port, p);
        }
        fwrite(&s1,sizeof(Content),1,fp1);
    }
    fclose(fp);
    fclose(fp1);


    if(found){
            fp = fopen("content.txt","w");
            fp1 = fopen("temp.txt","r");

            while(fread(&s1,sizeof(Content),1,fp1)){
                fwrite(&s1,sizeof(Content),1,fp);
            }
            fclose(fp);
            fclose(fp1);
    }
    else
        printf("\nNot Found.....\n");
}

bool delete_rec(Content *to_delete){
    int found=0;
    Content *s1;
    FILE *fp, *fp1;
    fp = fopen("content.txt","r");
    fp1 = fopen("temp.txt","w");

    while(fread(s1,sizeof(Content),1,fp)){
        strcat(s1->fname, "\0");
        strcat(s1->peer_name, "\0");

        if(strncmp(s1->fname,to_delete->fname, strlen(s1->fname))==0
            && strncmp(s1->peer_name,to_delete->peer_name, strlen(s1->peer_name)) == 0) {
            found = 1;
        } else {
            fwrite(s1,sizeof(Content),1,fp1);
        }
    }

    fclose(fp);
    fclose(fp1);

    if(found == 1){
        fp = fopen("content.txt","w");	
        fp1 = fopen("temp.txt","r");

        while(fread(s1,sizeof(Content),1,fp1)){
            fwrite(s1,sizeof(Content),1,fp);
        }
        fclose(fp);
        fclose(fp1);
        return true;
    }
    else {
        return false;
    }
}

void count(){
    Content s1;
    FILE *fp;
    fp = fopen("content.txt","r");
    fseek(fp,0,SEEK_END);
    int n = ftell(fp)/sizeof(Content);
    printf("\nNo of Records = %d\n",n);
    fclose(fp);
}

char files[5][20];

char * display() {
    Content *s1;
    FILE *fp;

    fp = fopen("content.txt","r");

    int i = 0;

    while(fread(s1,sizeof(Content),1,fp)) {
        if (i == 5) {
            printf("Too many files.\n");
            i++;
        } else {
            bool duplicate = false;
            for(int j=0; j<i; j++) {
                if(strcmp(files[j], s1->fname) == 0) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                strcpy(files[i], s1->fname);
                printf("%s\n", s1->fname);
                i++;
            }
        }
    }

    char * display = malloc(BUFLEN);

    for(int j=0; j<i; j++) {
        strcat(display, files[j]);
        strcat(display, "\n");
    }

    fclose(fp);
    return display;
}

char * copy_buf(char * buffer, int length) {
    char * answer;

    for(int i = 0; i < length; i++) {
        if(buffer[i] == ' ' || buffer[i] == '\0') {
            answer[i] = '\0';
        } else {
            answer[i] = buffer[i];
        }
    }

    return answer;
}

void send_tcp(char type, char * message, int udp_socket, struct sockaddr_in * fsin) {
    struct pdu * bufpdu = (struct pdu *)malloc(sizeof(struct pdu));
    bufpdu->type = type;
    strcpy(bufpdu->data, message);
    sendto(udp_socket, bufpdu, BUFLEN+1, 0, (struct sockaddr *) fsin, sizeof(*fsin));
    free(bufpdu);
}

int main(int argc, char *argv[]) {

	struct  sockaddr_in fsin;	/* the from address of a client	*/
	char	buf[100];		/* "input" buffer; any size > 0	*/
	int		sock;			/* server socket		*/
	int 	alen;			/* from-address length		*/
	struct  sockaddr_in sin,server; /* an Internet endpoint address         */
    int     udpsocket, type;        /* socket descriptor and socket type    */
	int 	port=3000;
    char	fname[100];
	char	disp[100];
	int 	n;
	FILE 	*fp;
	char 	buff[BUFLEN];
	char 	data[BUFLEN-1];
	int 	totalbytes;
	char 	clientAddress[INET_ADDRSTRLEN];
	uint16_t clientPort;
	struct pdu * bufpdu = (struct pdu*)malloc(sizeof(struct pdu));

	Content * content_buffer = (Content*)malloc(sizeof(Content));

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

	memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

	/* Create a stream socket	*/
	if ((udpsocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	/* Bind an address to the socket	*/
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY); 
	if (bind(udpsocket, (struct sockaddr *)&server, sizeof(server)) == -1){
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}

	alen = sizeof(fsin);

	fclose(fopen("content.txt", "w"));

	while (1) {
		/* empty buffer */
		strcpy(buff,"");
		bzero(buff,BUFLEN);
		bzero(disp,100);
		bzero(clientAddress,sizeof(clientAddress));
		clientPort = 0;

		if (recvfrom(udpsocket, (struct pdu *) bufpdu, BUFLEN+1, 0,(struct sockaddr *)&fsin, &alen) < 0) {
			fprintf(stderr, "recvfrom error\n");
		} else {
			if(bufpdu->type == 'R') {
                strcpy(content_buffer->peer_name, copy_buf(bufpdu->data, 10));
                strcpy(content_buffer->fname, copy_buf(bufpdu->data + 10, 20));
                strcpy(content_buffer->port, copy_buf(bufpdu->data + 30, 10));
                strcpy(content_buffer->ipAddress, inet_ntoa(fsin.sin_addr));

				if (search(content_buffer) != NULL) {
                    send_tcp('E', "File of same name was already registered by peer of same name.", udpsocket, &fsin);
				} else {
					append(content_buffer);
					send_tcp('A', "Acknowledgement: appended.", udpsocket, &fsin);
				}
			} else if (bufpdu->type == 'T') {
                bzero(content_buffer->fname, 20);
                bzero(content_buffer->peer_name, 10);
                strcpy(content_buffer->peer_name, copy_buf(bufpdu->data, 10));
                strcpy(content_buffer->fname, copy_buf(bufpdu->data + 10, 20));

				if (delete_rec(content_buffer)) {
                    send_tcp('A', "Acknowledgement: deregistration successful.", udpsocket, &fsin);
				} else {
                    bzero(buff, BUFLEN);
                    strcat(buff, content_buffer->fname);
                    strcat(buff, " is not registered by you.");
					send_tcp('E', buff, udpsocket, &fsin);
				}
			} else if(bufpdu->type == 'O') {
                bufpdu->type = 'O';
                bzero(bufpdu->data, BUFLEN);
				strcpy(bufpdu->data, display());
				(void) sendto(udpsocket, bufpdu, BUFLEN+1, 0,(struct sockaddr *)&fsin, sizeof(fsin));
			} else if(bufpdu->type == 'S') {
                strcpy(content_buffer->peer_name, copy_buf(bufpdu->data, 10));
                strcpy(content_buffer->fname, copy_buf(bufpdu->data + 10, 20));
                bzero(content_buffer->ipAddress, 20);
                bzero(content_buffer->port, 10);

                if(search(content_buffer)) {
                    send_tcp('E', "File of same name was already registered by peer of same name.", udpsocket, &fsin);
                } else {
                    Content * server_info = searchServer(content_buffer->fname);

                    if(server_info != NULL) {
                        bufpdu->type = 'S';
                        bzero(bufpdu->data, BUFLEN);

                        strcat(bufpdu->data, server_info->ipAddress);
                        memset(bufpdu->data + strlen(bufpdu->data), ' ', 20 - strlen(bufpdu->data));
                        strcat(bufpdu->data, server_info->port);
                        memset(bufpdu->data + strlen(bufpdu->data), ' ', 30 - strlen(bufpdu->data));

                        sendto(udpsocket, bufpdu, BUFLEN+1, 0,(struct sockaddr *)&fsin, sizeof(fsin));

                        recvfrom(udpsocket, (struct pdu *) bufpdu, BUFLEN+1, 0,(struct sockaddr *)&fsin, &alen);
                        strcpy(content_buffer->port, bufpdu->data);
                        strcpy(content_buffer->ipAddress, inet_ntoa(fsin.sin_addr));
                        append(content_buffer);
                        send_tcp('A', "Acknowledgement: appended.", udpsocket, &fsin);
                    } else {
                        send_tcp('E', "File not found.", udpsocket, &fsin);
                    }
                }

			} else {
                printf("Invalid UDP type");
			}
		}
	}
}
