/* time_client.c - main */
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <signal.h>
#include <stdbool.h> // bool
#include <stddef.h> // null
#include <sys/wait.h> // wait3 in reaper()
#include <errno.h>

#define BUFLEN		100

struct pdu {
   char type;
   char data[100];
};

// TCP
void send_file(int, char*);
void reaper(int);
int createTCP(struct sockaddr_in *);
void runTCP(char*,int);
bool TCPDownload(char*, char*, char*);
char fileError[20] = "File not found.";
char * copy_buf(char *, int);

// UDP
bool isError(struct pdu *);
void deregisterFile(char*,char*,int);
struct pdu * UDPSendReceive(char, char*, int);

// Linked list - storage
struct LinkedListNode {
    char data[50];
    struct LinkedListNode* next;
};
void push(struct LinkedListNode**, char*);
void printLL(struct LinkedListNode**);
bool contains(struct LinkedListNode**, char*);
bool deleteLL(struct LinkedListNode**, char*);

/*------------------------------------------------------------------------
 * main - UDP server and TCP client
 *------------------------------------------------------------------------
 */
int main(int argc, char **argv) {
	// UDP
	char	*host = "localhost";
	char    *port = "3000";
	char	buf[101];
	struct hostent	*phe;	/* pointer to host information entry	*/
	struct sockaddr_in sin;	/* an Internet endpoint address		*/
	int	s, n, type;	/* socket descriptor and socket type	*/
	char	*gui_message = "Enter file name: ";

	// TCP SERVER
	int s_tcp;
    struct sockaddr_in reg_addr;
    char dyn_port[5];

    // TCP CLIENT
    char tcp_server_port[5];
    char tcp_server_ip[20];

	char username[10];
	char command[50];
    char downloadFile[20];
    char uploadFile[20];
    char downloadPeer[10];
    struct LinkedListNode * head = NULL;
    struct pdu * bufpdu = (struct pdu*)malloc(sizeof(struct pdu));
    char sendData[BUFLEN];

	char *options = "R: Content registration\tD: Content Download Request\nT: Content De-Registration\tL: List Local Content\nO: List of Online Registered Content\tQ: Quit\n";

	switch (argc) {
	case 1:
		break;
	case 2:
		host = argv[1];
	case 3:
		host = argv[1];
		port = argv[2];
		break;
	default:
		fprintf(stderr, "usage: peer host [port]\n");
		exit(1);
	}

	// UDP CLIENT
	memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(atoi(port));

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
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    fprintf(stderr, "Can't connect to %s %s \n", host, "Time");

    printf("Enter username (max 10 characters): ");
    scanf("%s", username);

	while(1) {
		printf("Command (enter ? for options): ");
		scanf(" %s", command);
		char charCommand = command[0];

		switch(charCommand) {
			case '?':
				printf("%s", options);
				break;
            case 'R':
                printf("File to register: ");
                uploadFile[0] = '\0';
                scanf("%s", uploadFile);
                if(contains(&head, uploadFile)) {
                    printf("This file has already been registered by you.\n");
                } else {
                    FILE * file = fopen(uploadFile, "r");

                    if(file == NULL) {
                        printf("Error: The file was not found. Please double-check the location.\n");
                    } else {
                        if((s_tcp = createTCP(&reg_addr)) == -1) {
                            printf("Error in creating TCP server with dynamic port. Cannot register file.\n");
                        } else {
                            sprintf(dyn_port, "%d", ntohs(reg_addr.sin_port));

                            bzero(sendData, BUFLEN);

                            strcat(sendData, username);
                            memset(sendData + strlen(sendData), ' ', 10 - strlen(sendData));
                            strcat(sendData, uploadFile);
                            memset(sendData + strlen(sendData), ' ', 30 - strlen(sendData));
                            strcat(sendData, dyn_port);
                            memset(sendData + strlen(sendData), ' ', 40 - strlen(sendData));

                            bufpdu = UDPSendReceive('R', sendData, s);

                            if(!isError(bufpdu) && bufpdu->type == 'A') {
                                printf("Registration of file %s was successful (port %s).\n", uploadFile, dyn_port);
                                push(&head, uploadFile);
                                runTCP(uploadFile, s_tcp); // createTCP was done above
                            } else {
                                printf("Registration of file %s was unsuccessful.\n", uploadFile);
                                printf("Enter new username (max 10 characters): ");
                                scanf("%s", username);
                            }

                        }

                        fclose(file);
                    }
                }

                break;
            case 'D':
                printf("File: ");
                scanf("%s", downloadFile);

                bzero(sendData, BUFLEN);
                strcat(sendData, username);
                memset(sendData + strlen(sendData), ' ', 10 - strlen(sendData));
                strcat(sendData, downloadFile);
                memset(sendData + strlen(sendData), ' ', 30 - strlen(sendData));
                bufpdu = UDPSendReceive('S', sendData, s);

                if(!isError(bufpdu) && bufpdu->type == 'S') {
                    printf("%s\n", bufpdu->data);
                    strcpy(tcp_server_ip, copy_buf(bufpdu->data, 20));
                    strcpy(tcp_server_port, copy_buf(bufpdu->data + 20, 6));

                    if(TCPDownload(downloadFile, tcp_server_port, tcp_server_ip)) {
                        s_tcp = createTCP(&reg_addr);
                        if(s_tcp != -1) {
                            runTCP(downloadFile, s_tcp);
                        }

                        bzero(sendData, BUFLEN);
                        sprintf(dyn_port, "%d", ntohs(reg_addr.sin_port));
                        strcat(sendData, dyn_port);
                        bufpdu = UDPSendReceive('R', sendData, s);

                        if(bufpdu->type == 'A') {
                            printf("Success: file downloaded and registered at port %s.\n", dyn_port);
                        } else {
                            printf("Failure.\n");
                        }

                    }
                } else {
                    if(strcmp(bufpdu->data, "File of same name was already registered by peer of same name.") == 0) {
                        printf("Enter new username (max 10 characters): ");
                        scanf("%s", username);
                    }
                }

                break;
            case 'T':
                printf("File to deregister: ");
                scanf(" %s", uploadFile);

                if(deleteLL(&head, uploadFile)) {
                    deregisterFile(username, uploadFile, s);
                } else {
                    printf("You have not registered this file.\n");
                }

                break;
            case 'L':
                if(head == NULL) {
                    printf("No local files have been registered yet.\n");
                }

                printLL(&head);

                break;
            case 'O':
                bufpdu = UDPSendReceive('O', "", s);
                if(!isError(bufpdu) && bufpdu->type == 'O') {
                    printf("%s", bufpdu->data);
                }
                break;
            case 'Q': ;
                struct LinkedListNode * iterator = head;
                while(iterator != NULL) {
                    deregisterFile(username, iterator->data, s);
                    iterator = iterator->next;
                }
                exit(0);
            default:
                printf("Invalid option.\n");
                break;
		}
	}

	exit(0);
}

/*	reaper		*/
void	reaper(int sig)
{
	int	status;
	while(wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}

bool isError(struct pdu * bufpdu) {
    if(bufpdu->type == 'E') {
        printf("Error: %s\n", bufpdu->data);
        return true;
    }
    return false;
}

void deregisterFile(char* username, char* fileName, int s) {
    char sendBuf[BUFLEN];
    bzero(sendBuf, BUFLEN);
    strcat(sendBuf, username);
    memset(sendBuf + strlen(sendBuf), ' ', 10 - strlen(sendBuf));
    strcat(sendBuf, fileName);
    memset(sendBuf + strlen(sendBuf), ' ', 30 - strlen(sendBuf));

    struct pdu * receive_pdu = UDPSendReceive('T', sendBuf, s);
    if(!isError(receive_pdu)) {
        printf("The file %s was successfully deregistered.\n", fileName);
    }
}

void push(struct LinkedListNode ** head, char * newData) {
    struct LinkedListNode * nextNode = (struct LinkedListNode*)malloc(sizeof(struct LinkedListNode));
    strcpy(nextNode->data, newData);
    nextNode->next = NULL;

    if (*head == NULL) {
        *head = nextNode;
    } else {
        struct LinkedListNode * temp = *head;

        while (temp->next != NULL) {
            temp = temp->next;
        };

        temp->next = nextNode;
    }
}

void printLL(struct LinkedListNode ** head) {
    struct LinkedListNode * temp = *head;
    while (temp != NULL) {
        printf("%s\n", temp->data);
        temp = temp->next;
    }
}

bool contains(struct LinkedListNode ** head, char* search) {
    struct LinkedListNode * temp = *head;
    while (temp != NULL) {
        if (strcmp(temp->data, search) == 0) {
            return true;
        }
        temp = temp->next;
    }
    return false;
}

bool deleteLL(struct LinkedListNode ** head, char* uploadFile) {
    if(*head == NULL) {
        return false;
    }

    struct LinkedListNode * tempNode = *head;
    struct LinkedListNode * prevNode;

    if(strcmp(tempNode->data, uploadFile) == 0) {
        *head = tempNode->next;
        return true;
    }


    while (tempNode != NULL) {
        if(strcmp(tempNode->data, uploadFile) == 0) {
            prevNode->next = tempNode->next;
            free(tempNode);
            return true;
        }
        prevNode = tempNode;
        tempNode = tempNode->next;
    }
}

struct pdu * UDPSendReceive(char messageType, char* data, int s) {
    struct pdu * send_pdu = (struct pdu*)malloc(sizeof(struct pdu));
    send_pdu->type = messageType;
    strcpy(send_pdu->data, data);
    write(s, send_pdu, BUFLEN+1);
    free(send_pdu);

    struct pdu * receive_pdu = (struct pdu*)malloc(sizeof(struct pdu));
    read(s, receive_pdu, BUFLEN+1);

    return receive_pdu;
}

int createTCP(struct sockaddr_in * reg_addr) {
    int s_tcp = 0;

    if((s_tcp = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Can't create a socket\n");
        return -1;
    };

    reg_addr->sin_family = AF_INET;
    reg_addr->sin_port = htons(0);
    reg_addr->sin_addr.s_addr = htonl(INADDR_ANY);

    socklen_t a_len = sizeof(*reg_addr);

    if(bind(s_tcp, (struct sockaddr *) reg_addr, sizeof(*reg_addr)) < 0) {
        fprintf(stderr, "Can't bind name to socket\n");
        return -1;
    }

    if(getsockname(s_tcp, (struct sockaddr *) reg_addr, &a_len) < 0) {
        fprintf(stderr, "Can't get sock name\n");
        return -1;
    }

    return s_tcp;
}

void runTCP(char* fileName, int sd) {
    if (fork() == 0) {
        // TCP SERVER
        int 	new_sd, client_len;
        struct	sockaddr_in client;
        fd_set rfds, afds;

        listen(sd, 5);

        FD_ZERO(&afds);
        FD_SET(sd, &afds);
        FD_SET(0, &afds);
        memcpy(&rfds, &afds, sizeof(rfds));

        (void) signal(SIGCHLD, reaper);

        client_len = sizeof(client);

        while(1) {
          select(FD_SETSIZE, &rfds, NULL, NULL, NULL);

          if(FD_ISSET(sd, &rfds)){
            new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
            send_file(new_sd, fileName);
          }
        }
    }
}

void send_file(int sd, char* file) {
	FILE * file_to_send;
	file_to_send = fopen(file, "r");
	FILE * dump_file;
	dump_file = fopen("dump.txt", "w");

    struct pdu * writebuf = (struct pdu*) malloc(sizeof(struct pdu));

	if (file_to_send != NULL) {
        writebuf->type = 'C';
        long unsigned int position;

        char write_buffer[100];
        write_buffer[0] = writebuf->type;

		while(fread(writebuf->data, 1, 100, file_to_send) > 0) {
			write(sd, writebuf, strlen(writebuf->data)+1);
			bzero(writebuf, 101);
		}

		fclose(dump_file);
		fclose(file_to_send);
        shutdown(sd, 1); // shutdown 1 means shutdown of sends. Already-sent packets are still received by client, unlike with close.
	} else {
        writebuf->type = 'E';
        strcpy(writebuf->data, fileError);
        write(sd, writebuf, BUFLEN+1);
	}
    free(writebuf);
}

bool TCPDownload(char* filename, char* port, char* host) {
    int 	n, i, bytes_to_read;
	int 	sd;
	struct	hostent		*hp;
	struct	sockaddr_in server;
	char	*bp;
	struct pdu * requestbuf = (struct pdu *) malloc(sizeof(struct pdu));

	/* Create a stream socket	*/
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		return false;
	}

	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(port));

	if (hp = gethostbyname(host))
	  bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	else if ( inet_aton(host, (struct in_addr *) &server.sin_addr) ){
	  fprintf(stderr, "Can't get server's address\n");
	  return false;
	}

	/* Connecting to the server */
	if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
	  fprintf(stderr, "Can't connect \n");
	  return false;
	}

	requestbuf->type = 'D';
	strcpy(requestbuf->data, filename);

	write(sd, requestbuf, BUFLEN+1);
	FILE *new_file = fopen(filename, "w");

	char receive_buffer[100];
	int size = 0;

	while(true) {
        size = read(sd, (struct pdu * ) requestbuf, BUFLEN+1);
        fputs(requestbuf->data, new_file);
        bzero(requestbuf, 101);
        if (size < BUFLEN+1) {
            break;
        }
	}

    fclose(new_file);

	close(sd);

    return true;
}

char * copy_buf(char * buffer, int length) {
    char * answer;

    for(int i = 0; i < length; i++) {
        if(buffer[i] == ' ' || buffer[i] == '\0') {
            answer[i] = '\0';
            break;
        } else {
            answer[i] = buffer[i];
        }
    }

    return answer;
}
