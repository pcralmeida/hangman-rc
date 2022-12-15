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
#include <errno.h>
#include <signal.h>
#include <ctype.h>

extern int errno;
int errcode;
int trials = TRIAL_INIT;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
struct sigaction act;
char PLID[PLIDSIZE] = "";
char *GSIP = "";
char *GSport = "";
char letters[MAX_PRINTED_WORD_LENGTH];
char *strerror(int errnum);

void signalHandler(int sig) {
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    if (sigaction(sig, &act, NULL) == -1) {
        printf("Error: %s\n", strerror(errno));
        exit(1);
    }
}

void applyModifiers(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], IP_MODIFIER) == 0) {
        GSIP = argv[2];
        if (GSport == NULL) 
            GSport = DEFAULT_PORT;
    }

    if (argc > 3 && strcmp(argv[3], IP_MODIFIER) == 0) {
        GSIP = argv[4];
        if (GSport == NULL) 
            GSport = DEFAULT_PORT;
    }

    if (argc > 3 && strcmp(argv[3], PORT_MODIFIER) == 0) {
        GSport = argv[4];
        if (GSIP == NULL) 
            GSIP = LOCALHOST;
    }

    if (argc > 1 && strcmp(argv[1], PORT_MODIFIER) == 0) {
        GSport = argv[2];
        if (GSIP == NULL) 
            GSIP = LOCALHOST;
    }

    if (argc == 1) {
        GSIP = LOCALHOST;
        GSport = DEFAULT_PORT;
    }
}

void spaceGenerator(int number, char *str) {
    int i;
    strcpy(str," _");
    for(i=1;i<number;i++)
        strcat(str," _");
}

void composeWord(char letter[], int pos[]) {
    int i;
    int j = 0;
    char upper_letter = (char) toupper(letter[0]);
    for(i=0;i<strlen(letters);i++) {
        if ((i/2) == (pos[j] - 1)) {
            letters[i+1] = upper_letter;
            j++;   
        } 
    }
    letters[strlen(letters)] = '\0';
}

char* correctWord(char *str, char* correct) {
    int i = 0;
    int j = 0;
    while (i < strlen(str)) {
        char letter = (char) toupper(str[i]);
        sprintf(correct + j, " %c", letter);
        i++;
        j = j + 2;
    }
    correct[strlen(correct)] = '\0';
    return correct;
}

int startGame(int fd, char *Plid) {
    char message[BUFFERSIZE], command[BUFFERSIZE], status[BUFFERSIZE], errors[BUFFERSIZE], spaces[MAX_PRINTED_WORD_LENGTH];
    memset(&hints,0,sizeof hints);
    PLID[1] = '\0';
    hints.ai_family= AF_INET;  //IPv4
    hints.ai_socktype= SOCK_DGRAM;  //UDP socket
    trials = 1;
    errcode=getaddrinfo(GSIP,GSport,&hints,&res); //Get address info
    if(errcode!=0) { 
        fprintf(stderr,"Error getting address info: %s\n",gai_strerror(errcode));
    }
    sprintf(PLID,"%s",Plid);
    strcpy(message, START_GAME_PROTOCOL);
    strcat(message, " ");
    strcat(message, Plid);
    strcat(message, "\n");
    n=sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if(n==-1) {
        fprintf(stderr,"Error sending message: %s\n",strerror(errno));
        exit(1);
    }
    addrlen=sizeof(addr);
    n=recvfrom(fd,message,BUFFERSIZE,0,(struct sockaddr*)&addr,&addrlen);
    if(n==-1) {
        fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
        exit(1);
    }
    sscanf(message, "%s", command);
    sscanf(message + strlen(command), "%s", status);
    if (strcmp(status, STATUS_NOK) == 0) {
        printf("Warning: game has already started.\n");
        return 0;
    }
    sscanf(message, "%s %s %s %s", command, status, letters, errors);
    spaceGenerator(atoi(letters), spaces);
    spaceGenerator(atoi(letters), letters);
    printf("New game started (max %d errors):%s\n", atoi(errors), spaces);
    freeaddrinfo(res);
    return 0;
}

int playLetter(int fd, char* letter) {
    int i = 0, letter_occurences = 0;
    int positions[MAX_WORD_LENGTH - 1] = {0};
    char command[BUFFERSIZE], status[BUFFERSIZE], tries[BUFFERSIZE];
    char message[BUFFERSIZE] = "";
    message[1] = '\0';
    strcpy(message, "");
    memset(&hints,0,sizeof hints);
    hints.ai_family= AF_INET; //IPv4
    hints.ai_socktype= SOCK_DGRAM; //UDP socket
    errcode=getaddrinfo(GSIP,GSport,&hints,&res); // trocar por GSIP!
    if(errcode!=0) { 
        fprintf(stderr,"Error getting address info: %s\n",gai_strerror(errcode));
        exit(1);
    }
    sprintf(tries, "%d", trials);
    strcpy(message, PLAY_LETTER_PROTOCOL);
    strcat(message, " ");
    strcat(message, PLID);
    strcat(message, " ");
    strcat(message, letter);
    strcat(message, " ");
    strcat(message, tries);
    strcat(message, "\n");
    n=sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if(n==-1) {
        fprintf(stderr,"Error sending message: %s\n",strerror(errno));
        exit(1);
    }
    addrlen=sizeof(addr);
    n=recvfrom(fd,message,BUFFERSIZE,0,(struct sockaddr*)&addr,&addrlen);
    if(n==-1) {
        fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
        exit(1);
    }

    sscanf(message, "%s %s %s %d", command, status, tries, &letter_occurences);

    if (strcmp(status, STATUS_OK) == 0) {
        while (i < letter_occurences) {
            sscanf(message + strlen(command) + strlen(status) + strlen(tries) + 5 + 2*i,"%d", &positions[i]);
            i++;
        }
        composeWord(letter, positions);
        printf("Yes, '%s' is part of the word:%s\n", letter, letters);
        trials++;
    }

    if (strcmp(status, STATUS_NOK) == 0) {
        printf("No, '%s' is not part of the word:%s\n", letter, letters);
        trials++;
    }

    if (strcmp(status, STATUS_WIN) == 0) {
        while (i < strlen(letters)) {
            if (letters[i] == '_') {
                letters[i] = (char) toupper(letter[0]);
            }
            i++;
        }
        composeWord(letter, positions);
        printf("WELL DONE! You guessed:%s\n", letters);
    }

    if (strcmp(status, STATUS_OVR) == 0) {
        printf("GAME OVER! You lost.\n");
    }

    if (strcmp(status, STATUS_ERR) == 0) {
        printf("An error occurred. Please try again.\n");
    }

    if (strcmp(status, STATUS_DUP) == 0) {
        printf("You already sent the letter '%s'. Please try again.\n", letter);
    }

    if (strcmp(status, STATUS_INV) == 0) {
        printf("Invalid trial. Please try again with the same letter.\n");
    }

    freeaddrinfo(res);
    return 0;

}

int guessWord(int fd, char* guess) {
    char command[BUFFERSIZE], status[BUFFERSIZE], tries[BUFFERSIZE];
    char message[BUFFERSIZE] = "";
    char correct[BUFFERSIZE] = "";
    correct[1] = '\0';
    message[1] = '\0';
    strcpy(message, "");
    memset(&hints,0,sizeof hints);
    hints.ai_family= AF_INET; //IPv4
    hints.ai_socktype= SOCK_DGRAM; //UDP socket
    errcode=getaddrinfo(GSIP,GSport,&hints,&res); // trocar por GSIP!
    if(errcode!=0) { 
        fprintf(stderr,"Error getting address info: %s\n",gai_strerror(errcode));
        exit(1);
    }
    sprintf(tries, "%d", trials);
    strcpy(message, GUESS_WORD_PROTOCOL);
    strcat(message, " ");
    strcat(message, PLID);
    strcat(message, " ");
    strcat(message, guess);
    strcat(message, " ");
    strcat(message, tries);
    strcat(message, "\n");
    n=sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if(n==-1) {
        fprintf(stderr,"Error sending message: %s\n",strerror(errno));
        exit(1);
    }
    addrlen=sizeof(addr);
    n=recvfrom(fd,message,BUFFERSIZE,0,(struct sockaddr*)&addr,&addrlen);
    if(n==-1) {
        fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
        exit(1);
    }

    sscanf(message, "%s %s", command, status);

    if (strcmp(status, STATUS_NOK) == 0) {
        printf("TRY AGAIN! You didn't guess the word.\n");
        trials++;
    }

    if (strcmp(status, STATUS_WIN) == 0) {
        printf("WELL DONE! You guessed:%s\n", correctWord(guess, correct));
    }

    if (strcmp(status, STATUS_OVR) == 0) {
        printf("GAME OVER! You lost.\n");
    }

    if (strcmp(status, STATUS_ERR) == 0) {
        printf("An error occurred. Please try again.\n");
    }

    if (strcmp(status, STATUS_DUP) == 0) {
        printf("You already sent the word '%s'. Please try again.\n", guess);
    }

    if (strcmp(status, STATUS_INV) == 0) {
        printf("Invalid trial. Please try again with the same word.\n");
    }

    freeaddrinfo(res);
    return 0;
}

int revealWord(int fd) {
    char command[BUFFERSIZE], word[BUFFERSIZE];
    char message[BUFFERSIZE] = "";
    char correct[BUFFERSIZE] = "";
    correct[1] = '\0';
    message[1] = '\0';
    strcpy(message, "");
    memset(&hints,0,sizeof hints);
    hints.ai_family= AF_INET; //IPv4
    hints.ai_socktype= SOCK_DGRAM; //UDP socket
    errcode=getaddrinfo(GSIP,GSport,&hints,&res); // trocar por GSIP!
    if(errcode!=0) { 
        fprintf(stderr,"Error getting address info: %s\n",gai_strerror(errcode));
        exit(1);
    }
    strcpy(message, REVEAL_PROTOCOL);
    strcat(message, " ");
    strcat(message, PLID);
    strcat(message, "\n");
    n=sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if(n==-1) {
        fprintf(stderr,"Error sending message: %s\n",strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }
    addrlen=sizeof(addr);
    n=recvfrom(fd,message,BUFFERSIZE,0,(struct sockaddr*)&addr,&addrlen);
    if(n==-1) {
        fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    sscanf(message, "%s %s", command, word);

    if (strcmp(command, STATUS_ERR) == 0) {
        printf("There is not a game in progress. Please start a new game.\n");
    }

    else {
        printf("The word was:%s\n", correctWord(word, correct));
    }

    freeaddrinfo(res);

    return 0;
}

int quitGame(int fd, int exit_code) {
    char command[BUFFERSIZE] = "";
    char message[BUFFERSIZE] = "";
    char status[BUFFERSIZE] = "";
    command[1] = '\0';
    message[1] = '\0';
    status[1] = '\0';
    strcpy(message, "");
    strcpy(status, "");
    strcpy(command, "");
    memset(&hints,0,sizeof hints);
    hints.ai_family= AF_INET; //IPv4
    hints.ai_socktype= SOCK_DGRAM; //UDP socket
    errcode=getaddrinfo(GSIP,GSport,&hints,&res); 
    if(errcode!=0) { 
        fprintf(stderr,"Error getting address info: %s\n",gai_strerror(errcode));
        exit(1);
    }
    strcpy(message, QUIT_PROTOCOL);
    strcat(message, " ");
    strcat(message, PLID);
    strcat(message, "\n");
    n=sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if(n==-1) {
        fprintf(stderr,"Error sending message: %s\n",strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }
    addrlen=sizeof(addr);
    n=recvfrom(fd,message,BUFFERSIZE,0,(struct sockaddr*)&addr,&addrlen);
    if(n==-1) {
        fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    sscanf(message, "%s %s", command, status);

    if (!exit_code) {
        if (strcmp(status, STATUS_ERR) == 0 || strcmp(command, STATUS_ERR) == 0) 
            printf("There is not a game in progress.\n");
        else
            printf("You have quit the game.\n");
    }

    else {
        printf("Goodbye!\n");
        freeaddrinfo(res);
        exit(0);
    }

    freeaddrinfo(res);

    return 0;
}

int scoreboard(int fd) {
    FILE *scores = NULL;
    size_t protocol_length = 0;
    size_t filesize = 0;
    size_t bytes = 0;
    char *cursor;
    char command[BUFFERSIZE] = "";
    char message[BUFFERSIZE] = "";
    char status[BUFFERSIZE] = "";
    char filename[BUFFERSIZE] = "";
    char filesize_length[BUFFERSIZE] = "";
    command[1] = '\0';
    message[1] = '\0';
    status[1] = '\0';
    filename[1] = '\0';
    filesize_length[1] = '\0';
    strcpy(message, "");
    strcpy(status, "");
    strcpy(command, "");
    memset(&hints,0,sizeof hints);
    hints.ai_family= AF_INET; //IPv4
    hints.ai_socktype= SOCK_STREAM; //TCP socket
    errcode=getaddrinfo(GSIP,GSport,&hints,&res); 
    if(errcode!=0) { 
        fprintf(stderr,"Error getting address info: %s\n",gai_strerror(errcode));
        exit(1);
    }
    strcpy(message, SCOREBOARD_PROTOCOL);
    strcat(message, "\n");
    n=connect(fd, res->ai_addr, res->ai_addrlen);
    if(n==-1) {
        fprintf(stderr,"Error connecting to server: %s\n",strerror(errno));
        exit(1);
    }
    bytes = strlen(message);
    cursor = message;
    while (bytes > 0) {
        n = write(fd, cursor, bytes);
        if (n == -1) {
            fprintf(stderr,"Error sending message: %s\n",strerror(errno));
            exit(1);
        }
        bytes -= (size_t) n;
        cursor += n;
    }

    fprintf(stdout, "Downloading scoreboard file...\n");

    memset(message, 0, BUFFERSIZE);
    bytes = MAX_RESPONSE_SIZE; 
    cursor = message;
    while (bytes > 0) {
        n=read(fd, cursor, bytes);
        if(n == -1) {
            fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
            exit(1);
        }
        bytes -= (size_t) n;
        cursor += n;
    }

    sscanf(message, "%s %s %s %lu", command, status, filename, &filesize);

    sprintf(filesize_length, "%lu", filesize);

    protocol_length = strlen(SCOREBOARD_PROTOCOL) + strlen(status) + strlen(filename) + strlen(filesize_length) + 4; 

    bytes = filesize - MAX_RESPONSE_SIZE + protocol_length;

    while (bytes > 0) {
        n=read(fd, cursor, bytes);
        if(n == -1) {
            fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
            exit(1);
        }
        bytes -= (size_t) n;
        cursor += n;
    }

    if (strcmp(status, STATUS_EMPTY) == 0) {
        printf("There are still no wins! Keep grinding!\n");
        return 0;
    }

    scores = fopen(filename, "wb");

    if (scores == NULL) {
        fprintf(stderr,"Error opening score file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    if (fwrite(message + protocol_length, 1, filesize, scores) != filesize) {
        fprintf(stderr,"Error writing to score file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    if (fclose(scores) == EOF) {
        fprintf(stderr,"Error closing score file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    fprintf(stdout, "Scoreboard saved as %s\n", filename);

    freeaddrinfo(res);

    return 0;

}

int getHint(int fd) {
    FILE *hint = NULL;
    size_t protocol_length = 0;
    size_t filesize = 0;
    size_t bytes = 0;
    char *cursor;
    char command[BUFFERSIZE] = "";
    char message[BUFFERSIZE_LARGE] = "";
    char status[BUFFERSIZE] = "";
    char filename[BUFFERSIZE] = "";
    char filesize_length[BUFFERSIZE] = "";
    command[1] = '\0';
    message[1] = '\0';
    status[1] = '\0';
    filename[1] = '\0';
    filesize_length[1] = '\0';
    strcpy(message, "");
    strcpy(status, "");
    strcpy(command, "");
    memset(&hints,0,sizeof hints);
    hints.ai_family= AF_INET; //IPv4
    hints.ai_socktype= SOCK_STREAM; //TCP socket
    errcode=getaddrinfo(GSIP,GSport,&hints,&res); 
    if(errcode!=0) { 
        fprintf(stderr,"Error getting address info: %s\n",gai_strerror(errcode));
        exit(1);
    }
    if (strcmp(PLID, "") == 0) {
        fprintf(stderr, "Error: No PLID set. Use 'start' followed by your PLID to start a new game.\n");
        return 0;
    }
    strcpy(message, HINT_PROTOCOL);
    strcat(message, " ");
    strcat(message, PLID);
    strcat(message, "\n");
    n=connect(fd, res->ai_addr, res->ai_addrlen);
    if(n==-1) {
        fprintf(stderr,"Error connecting to server: %s\n",strerror(errno));
        exit(1);
    }
    bytes = strlen(message);
    cursor = message;
    while (bytes > 0) {
        n = write(fd, cursor, bytes);
        if (n == -1) {
            fprintf(stderr,"Error sending message: %s\n",strerror(errno));
            exit(1);
        }
        bytes -= (size_t) n;
        cursor += n;
    }
    
    memset(message, 0, BUFFERSIZE_LARGE);
    bytes = MAX_RESPONSE_SIZE; 
    cursor = message;
    fprintf(stdout, "Downloading hint image...\n");
    while (bytes > 0) {
        n=read(fd, cursor, bytes);
        if(n == -1) {
            fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
            exit(1);
        }
        bytes -= (size_t) n;
        cursor += n;
    }

    sscanf(message, "%s %s %s %lu", command, status, filename, &filesize);

    sprintf(filesize_length, "%lu", filesize);

    protocol_length = strlen(HINT_PROTOCOL) + strlen(status) + strlen(filename) + strlen(filesize_length) + 4; 

    bytes = filesize - MAX_RESPONSE_SIZE + protocol_length;

    while (bytes > 0) {
        n=read(fd, cursor, bytes);
        if(n == -1) {
            fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
            exit(1);
        }
        bytes -= (size_t) n;
        cursor += n;
    }

    hint = fopen(filename, "wb");

    if (hint == NULL) {
        fprintf(stderr,"Error opening hint file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    if (fwrite(message + protocol_length, 1, filesize, hint) != filesize) {
        fprintf(stderr,"Error writing to hint file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    if (fclose(hint) == EOF) {
        fprintf(stderr,"Error closing hint file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    fprintf(stdout, "Hint image saved as '%s'.\n", filename);

    freeaddrinfo(res);

    return 0;
}


int getState(int fd) {
    FILE *gameinfo = NULL;
    size_t protocol_length = 0;
    size_t filesize = 0;
    size_t bytes = 0;
    char *cursor;
    char command[BUFFERSIZE] = "";
    char message[BUFFERSIZE] = "";
    char status[BUFFERSIZE] = "";
    char filename[BUFFERSIZE] = "";
    char filesize_length[BUFFERSIZE] = "";
    command[1] = '\0';
    message[1] = '\0';
    status[1] = '\0';
    filename[1] = '\0';
    filesize_length[1] = '\0';
    strcpy(message, "");
    strcpy(status, "");
    strcpy(command, "");
    memset(&hints,0,sizeof hints);
    hints.ai_family= AF_INET; //IPv4
    hints.ai_socktype= SOCK_STREAM; //TCP socket
    errcode=getaddrinfo(GSIP,GSport,&hints,&res); 
    if(errcode!=0) { 
        fprintf(stderr,"Error getting address info: %s\n",gai_strerror(errcode));
        exit(1);
    }
    if (strcmp(PLID, "") == 0) {
        fprintf(stderr, "Error: No PLID set. Use 'start' followed by your PLID to start a new game.\n");
        return 0;
    }
    strcpy(message, STATE_PROTOCOL);
    strcat(message, " ");
    strcat(message, PLID);
    strcat(message, "\n");
    n=connect(fd, res->ai_addr, res->ai_addrlen);
    if(n==-1) {
        fprintf(stderr,"Error connecting to server: %s\n",strerror(errno));
        exit(1);
    }
    bytes = strlen(message);
    cursor = message;
    while (bytes > 0) {
        n = write(fd, cursor, bytes);
        if (n == -1) {
            fprintf(stderr,"Error sending message: %s\n",strerror(errno));
            exit(1);
        }
        bytes -= (size_t) n;
        cursor += n;
    }

    fprintf(stdout, "Downloading status file...\n");

    memset(message, 0, BUFFERSIZE);
    bytes = MAX_RESPONSE_SIZE; 
    cursor = message;
    while (bytes > 0) {
        n=read(fd, cursor, bytes);
        if(n == -1) {
            fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
            exit(1);
        }
        bytes -= (size_t) n;
        cursor += n;
    }

    sscanf(message, "%s %s %s %lu", command, status, filename, &filesize);

    sprintf(filesize_length, "%lu", filesize);

    protocol_length = strlen(HINT_PROTOCOL) + strlen(status) + strlen(filename) + strlen(filesize_length) + 4; 

    bytes = filesize - MAX_RESPONSE_SIZE + protocol_length;

    while (bytes > 0) {
        n=read(fd, cursor, bytes);
        if(n == -1) {
            fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
            exit(1);
        }
        bytes -= (size_t) n;
        cursor += n;
    }

    if (strcmp(status, STATUS_NOK) == 0) {
        printf("Please start a game before using 'state' command.\n");
        return 0;
    }

    if (strcmp(status, STATUS_ACT) == 0) {
        printf("Ongoing game detected. Received current game summary.\n");
    }

    if (strcmp(status, STATUS_FIN) == 0) {
        printf("No game detected. Received recent games summary.\n");
    }

    gameinfo = fopen(filename, "wb");

    if (gameinfo == NULL) {
        fprintf(stderr,"Error opening state file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    if (fwrite(message + protocol_length, 1, filesize, gameinfo) != filesize) {
        fprintf(stderr,"Error writing to state file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    if (fclose(gameinfo) == EOF) {
        fprintf(stderr,"Error closing state file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    fprintf(stdout, "State file saved as '%s'.\n", filename);

    freeaddrinfo(res);

    return 0;

}