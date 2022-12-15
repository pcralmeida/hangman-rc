#include "envserver.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>

int verbose = 0;
char *GSport = DEFAULT_PORT;

void applyModifiers(int argc, char *argv[]) {


    if ((argc >= 3 && strcmp(argv[2], VERBOSE_MODIFIER) == 0) || (argc >= 5 && strcmp(argv[4], VERBOSE_MODIFIER) == 0)) {
        printf("Verbose mode enabled.\n");
        verbose = 1;
    }


    if (argc > 3 && strcmp(argv[2], PORT_MODIFIER) == 0) {
        printf("Port modifier enabled 1.\n");
        GSport = argv[3];
    }

    if (argc >= 5 && strcmp(argv[3], PORT_MODIFIER) == 0) {
        printf("Port modifier enabled 2.\n");
        GSport = argv[4];
    }

    if (argc == 1) {
        fprintf(stderr, "Error: No word file specified. Usage: ./GS word_file [-p GSport] [-v].\n");
        exit(1);
    }

    printf("Done! Server Port: %s, Verbose: %d.\n", GSport, verbose);
}

int startGame(char message[]) {
    char PLID[PLIDSIZE + 1] = "";
    PLID[1] = '\0';
    sscanf(message + PROTOCOLSIZE, "%s", PLID);
    printf("Starting game for player %s.\n", PLID);
    memset(message, 0, BUFFERSIZE);
    strcat(message, START_GAME_RESPONSE_PROTOCOL);
    strcat(message, " ");
    strcat(message, STATUS_OK);
    strcat(message, " ");
    strcat(message, "10");
    strcat(message, " ");
    strcat(message, "9");
    strcat(message, "\n");
    printf("Message formed: %s", message);
    return 0;
}

void analyzeMessage(char message[]) {
    char command[BUFFERSIZE] = "";
    command[1] = '\0';
    sscanf(message, "%s", command);

    if ((strcmp(command, START_GAME_PROTOCOL) == 0) && (strlen(message) == PROTOCOLSIZE + PLIDSIZE + 2)) {
        printf("Start game command detected.\n");
        startGame(message);

    }

    else if (strcmp(command, PLAY_LETTER_PROTOCOL) == 0) {
        printf("Play letter command detected.\n");
    }

    else if (strcmp(command, GUESS_WORD_PROTOCOL) == 0) {
        printf("Guess word command detected.\n");
    }

    else if (strcmp(command, SCOREBOARD_PROTOCOL) == 0) {
        printf("Scoreboard command detected.\n");
    }

    else if (strcmp(command, HINT_PROTOCOL) == 0) {
        printf("Hint command detected.\n");
    }

    else if (strcmp(command, STATE_PROTOCOL) == 0) {
        printf("State command detected.\n");
    }

    else if (strcmp(command, REVEAL_PROTOCOL) == 0) {
        printf("Reveal command detected.\n");
    }

    else if (strcmp(command, QUIT_PROTOCOL) == 0) {
        printf("Quit command detected.\n");
    }
    
    else {
        printf("Invalid command detected.\n");
    }
}

int receiveUDP(int fd) {
    struct addrinfo hints,*res;
    int errcode;
    struct sockaddr_in addr;
    socklen_t addrlen;
    ssize_t n, nread;
    char message[BUFFERSIZE] = "";
    message[1] = '\0';

    if((fd=socket(AF_INET,SOCK_DGRAM,0))==-1) { //error
        fprintf(stderr, "Socket creation failed.\n");
        exit(1); 
    }

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; //IPv4
    hints.ai_socktype=SOCK_DGRAM;//UDP socket
    hints.ai_flags=AI_PASSIVE; 

    errcode=getaddrinfo(NULL,GSport,&hints,&res); /*error*/
    
    if(errcode!=0) { 
        fprintf(stderr,"Error getting address info: %s\n",gai_strerror(errcode));
        exit(1);
    }

    if(bind(fd,res->ai_addr,res->ai_addrlen)==-1) {
        fprintf(stderr, "Error: server address binding failed.\n"); /*error*/
        exit(1);
    }

    while(1) {
        addrlen=sizeof(addr);
        fprintf(stdout, "Waiting for message...\n");
        nread=recvfrom(fd,message,BUFFERSIZE,0,(struct sockaddr*)&addr, &addrlen);
        if(nread==-1) {/*error*/
            fprintf(stderr, "Error receiving message: %s\n", strerror(errno));
            exit(1);
        }
        if (verbose == 1) {
            fprintf(stdout, "Message received from %s: %d\n", inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
            fprintf(stdout, "Message content: %s", message);
        }

        analyzeMessage(message);

        printf("Message to be sent: %s", message);

        n=sendto(fd,message, (size_t)nread,0,(struct sockaddr*)&addr, addrlen);

        if(n==-1) {
            fprintf(stderr, "Error sending message: %s\n", strerror(errno));
            exit(1);
        }
    }
    freeaddrinfo(res);
    close(fd);
    return 0;
}
