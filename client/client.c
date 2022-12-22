#include "envclient.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>

char buffer[BUFFERSIZE];
int fd;

int main(int argc, char *argv[]) {

    applyModifiers(argc, argv);

    while(fgets(buffer,BUFFERSIZE,stdin)) {
        char command[BUFFERSIZE];
        sscanf(buffer,"%s",command);

        if (strcmp(command,START_GAME) == 0 || strcmp(command,START_GAME_LONG) == 0) {
            fd=socket(AF_INET,SOCK_DGRAM,0); 
            if(fd==-1) {
                printf("Socket creation failed.\n");
                fprintf(stderr,"error: %s\n",strerror(errno));
                exit(1);
            }
            sscanf(buffer + strlen(command),"%s",command);
            if (strlen(command) != PLIDSIZE - 1) 
                fprintf(stderr, "Invalid player ID. Please try again.\n");
            else
                startGame(fd, command);
            close(fd);
        }
        
        else if (strcmp(command,GUESS_WORD) == 0 || strcmp(command,GUESS_WORD_LONG) == 0) {
            fd=socket(AF_INET,SOCK_DGRAM,0); 
            if(fd==-1) {
                printf("Socket creation failed.\n");
                fprintf(stderr,"error: %s\n",strerror(errno));
                exit(1);
            }
            sscanf(buffer + strlen(command),"%s",command);
            guessWord(fd, command);
            close(fd);
        }
        else if (strcmp(command,PLAY_LETTER) == 0 || strcmp(command,PLAY_LETTER_LONG) == 0) {
            fd=socket(AF_INET,SOCK_DGRAM,0);
            if(fd==-1) {
                printf("Socket creation failed.\n");
                fprintf(stderr,"error: %s\n",strerror(errno));
                exit(1);
            }
            sscanf(buffer + strlen(command),"%s",command);
            playLetter(fd, command);
            close(fd);
        }
        else if (strcmp(command,SCOREBOARD) == 0 || strcmp(command,SCOREBOARD_LONG) == 0) {
            fd=socket(AF_INET,SOCK_STREAM,0);
            if(fd==-1) {
                printf("Socket creation failed.\n");
                fprintf(stderr,"error: %s\n",strerror(errno));
                exit(1);
            }
            scoreboard(fd);
            close(fd);
        }
        else if (strcmp(command,HINT) == 0 || strcmp(command,HINT_LONG) == 0) {
            fd=socket(AF_INET,SOCK_STREAM,0);
            if (fd == -1) {
                printf("Socket creation failed.\n");
                fprintf(stderr,"error: %s\n",strerror(errno));
                exit(1);
            }
            getHint(fd);
            close(fd);
        }
        else if (strcmp(command,STATE) == 0 || strcmp(command,STATE_LONG) == 0) {
            fd=socket(AF_INET,SOCK_STREAM,0);
            if (fd == -1) {
                printf("Socket creation failed.\n");
                fprintf(stderr,"error: %s\n",strerror(errno));
                exit(1);
            }
            getState(fd);
            close(fd);
        }
        else if (strcmp(command,QUIT) == 0) {
            fd=socket(AF_INET,SOCK_DGRAM,0); 
            if(fd==-1) {
                printf("Socket creation failed.\n");
                fprintf(stderr,"error: %s\n",strerror(errno));
                exit(1);
            }
            quitGame(fd, 0);
            close(fd);
        }
        else if (strcmp(command,EXIT) == 0) {
            fd=socket(AF_INET,SOCK_DGRAM,0); 
            if(fd==-1) {
                printf("Socket creation failed.\n");
                fprintf(stderr,"error: %s\n",strerror(errno));
                exit(1);
            }
            quitGame(fd, 1);
            close(fd);
        }
        
        else {
            printf("Invalid command. Please try again.\n");
        }
        
        
    }

    return 0; 
}
    