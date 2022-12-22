#include "envclient.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <dirent.h>
#include <math.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

extern int errno; // Error number
int errcode; // Error code
int trials = TRIAL_INIT; // Number of trials
ssize_t n; // Number of bytes
socklen_t addrlen; // Address length
struct addrinfo hints,*res; // Address info
struct sockaddr_in addr; // Socket address
struct sigaction act; // Signal action
char PLID[PLIDSIZE] = ""; // Player ID
char *GSIP = ""; // Game server IP
char *GSport = ""; // Game server port
char letters[MAX_PRINTED_WORD_LENGTH]; // Letters
char *strerror(int errnum); // Error message


void applyModifiers(int argc, char *argv[]) { // Apply the modifiers
    if (argc > 1 && strcmp(argv[1], IP_MODIFIER) == 0) { // If the first argument is the IP modifier
        GSIP = argv[2];
        if (GSport == NULL) 
            GSport = DEFAULT_PORT;
    }

    if (argc > 3 && strcmp(argv[3], IP_MODIFIER) == 0) { // If the third argument is the IP modifier
        GSIP = argv[4];
        if (GSport == NULL) 
            GSport = DEFAULT_PORT;
    }

    if (argc > 3 && strcmp(argv[3], PORT_MODIFIER) == 0) { // If the third argument is the port modifier
        GSport = argv[4];
        if (GSIP == NULL) 
            GSIP = LOCALHOST;
    }

    if (argc > 1 && strcmp(argv[1], PORT_MODIFIER) == 0) { // If the first argument is the port modifier
        GSport = argv[2];
        if (GSIP == NULL) 
            GSIP = LOCALHOST;
    }

    if (argc == 1) { // If there are no arguments
        GSIP = LOCALHOST;
        GSport = DEFAULT_PORT;
    }
}

void spaceGenerator(int number, char *str) { // Generate spaces
    int i;
    strcpy(str," _");
    for(i=1;i<number;i++)
        strcat(str," _");
}

void composeWord(char letter[], int pos[]) { // Compose the word
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

char* correctWord(char *str, char* correct) { // Correct the word
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

int startGame(int fd, char *Plid) { // Start the game
    char message[BUFFERSIZE], command[BUFFERSIZE], status[BUFFERSIZE], errors[BUFFERSIZE], spaces[MAX_PRINTED_WORD_LENGTH];
    struct timeval timeout; // Timeout
    memset(&hints,0,sizeof hints);
    PLID[1] = '\0';
    hints.ai_family= AF_INET;  //IPv4
    hints.ai_socktype= SOCK_DGRAM;  //UDP socket
    trials = 1;
    errcode=getaddrinfo(GSIP,GSport,&hints,&res); //Get address info

    timeout.tv_sec = 10; // Timeout of 10 seconds
    timeout.tv_usec = 0;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0) { // Set the timeout
        fprintf(stderr, "UDP Error: setsockopt failed...%s\n", strerror(errno)); /*error*/
        exit(1);
    }

    if(errcode!=0) { 
        fprintf(stderr,"Error getting address info: %s\n",gai_strerror(errcode));
    }
    sprintf(PLID,"%s",Plid);
    strcpy(message, START_GAME_PROTOCOL);
    strcat(message, " ");
    strcat(message, Plid);
    strcat(message, "\n");
    n=sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen); // Send the message
    if(n==-1) {
        fprintf(stderr,"Error sending message: %s\n",strerror(errno));
        exit(1);
    }
    addrlen=sizeof(addr);
    n=recvfrom(fd,message,BUFFERSIZE,0,(struct sockaddr*)&addr,&addrlen); // Receive the message
    if(n==-1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { // If the timeout has expired
            fprintf(stdout, "UDP Timeout: no message received.\n");
            freeaddrinfo(res);
            return 0;
        }
        fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
        exit(1);
    }
    sscanf(message, "%s", command);
    sscanf(message + strlen(command), "%s", status);
    if (strcmp(status, STATUS_NOK) == 0) { // If the game has already started
        printf("Warning: game has already started.\n");
        return 0;
    }
    sscanf(message, "%s %s %s %s", command, status, letters, errors);
    spaceGenerator(atoi(letters), spaces); // Generate spaces
    spaceGenerator(atoi(letters), letters); 
    printf("New game started (max %d errors):%s\n", atoi(errors), spaces); // Print the spaces in the message
    freeaddrinfo(res);
    return 0;
}

int playLetter(int fd, char* letter) { // Play a letter
    int i = 0, letter_occurences = 0;
    int positions[MAX_WORD_LENGTH - 1] = {0};
    char command[BUFFERSIZE], status[BUFFERSIZE], tries[BUFFERSIZE];
    char message[BUFFERSIZE] = "";
    struct timeval timeout;
    message[1] = '\0';
    strcpy(message, "");
    memset(&hints,0,sizeof hints);
    hints.ai_family= AF_INET; //IPv4
    hints.ai_socktype= SOCK_DGRAM; //UDP socket
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0) {
        fprintf(stderr, "UDP Error: setsockopt failed...%s\n", strerror(errno)); // error
        exit(1);
    }

    errcode=getaddrinfo(GSIP,GSport,&hints,&res); //Get address info
    if(errcode!=0) { 
        fprintf(stderr,"Error getting address info: %s\n",gai_strerror(errcode));
        exit(1);
    }
    if ((strlen(letter) > 1) || (!isalpha(letter[0]))) {
        printf("Invalid letter. Please try again.\n");
        return 0;
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
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            fprintf(stdout, "UDP Timeout: no message received.\n");
            freeaddrinfo(res);
            return 0;
        }
        fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
        exit(1);
    }

    sscanf(message, "%s %s %s %d", command, status, tries, &letter_occurences);

    if (strcmp(status, STATUS_OK) == 0) { // If the letter is part of the word
        while (i < letter_occurences) {
            sscanf(message + strlen(command) + strlen(status) + strlen(tries) + 5 + 2*i,"%d", &positions[i]);
            i++;
        }
        composeWord(letter, positions); // Compose the word
        printf("Yes, '%s' is part of the word:%s\n", letter, letters);
        trials++;
    }

    if (strcmp(status, STATUS_NOK) == 0) { // If the letter is not part of the word
        printf("No, '%s' is not part of the word:%s\n", letter, letters);
        trials++;
    }

    if (strcmp(status, STATUS_WIN) == 0) { // If the player has won
        while (i < strlen(letters)) {
            if (letters[i] == '_') {
                letters[i] = (char) toupper(letter[0]);
            }
            i++;
        }
        composeWord(letter, positions);
        printf("WELL DONE! You guessed:%s\n", letters);
    }

    if (strcmp(status, STATUS_OVR) == 0) { // If the player has lost
        printf("GAME OVER! You lost.\n");
    }

    if (strcmp(status, STATUS_ERR) == 0 || strcmp(command, STATUS_ERR) == 0) { // If there is an error
        printf("An error has ocurred. Please try again.\n");
    }

    if (strcmp(status, STATUS_DUP) == 0) { // If the letter has already been sent
        printf("You already sent the letter '%s'. Please try again.\n", letter);
    }

    if (strcmp(status, STATUS_INV) == 0) { // If the letter is invalid
        printf("Invalid trial. Please try again with the same letter.\n");
    }

    freeaddrinfo(res);
    return 0;

}

int guessWord(int fd, char* guess) { // Guess the word
    char command[BUFFERSIZE], status[BUFFERSIZE], tries[BUFFERSIZE];
    char message[BUFFERSIZE] = "";
    char correct[BUFFERSIZE] = "";
    struct timeval timeout;
    correct[1] = '\0';
    message[1] = '\0';
    strcpy(message, "");
    memset(&hints,0,sizeof hints);
    hints.ai_family= AF_INET; //IPv4
    hints.ai_socktype= SOCK_DGRAM; //UDP socket
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0) {
        fprintf(stderr, "UDP Error: setsockopt failed...%s\n", strerror(errno)); // error
        exit(1);
    }

    errcode=getaddrinfo(GSIP,GSport,&hints,&res); 
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
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            fprintf(stdout, "UDP Timeout: no message received.\n");
            freeaddrinfo(res);
            return 0;
        }
        fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
        exit(1);
    }

    sscanf(message, "%s %s", command, status);

    if (strcmp(status, STATUS_NOK) == 0) { // If the word is not correct
        printf("TRY AGAIN! You didn't guess the word.\n");
        trials++;
    }

    if (strcmp(status, STATUS_WIN) == 0) { // If the player has won
        printf("WELL DONE! You guessed:%s\n", correctWord(guess, correct));
    }

    if (strcmp(status, STATUS_OVR) == 0) { // If the player has lost
        printf("GAME OVER! You lost.\n");
    }

    if (strcmp(status, STATUS_ERR) == 0) { // If there is an error
        printf("An error has ocurred. Please try again.\n");
    }

    if (strcmp(status, STATUS_DUP) == 0) { // If the word has already been sent
        printf("You already sent the word '%s'. Please try again.\n", guess);
    }

    if (strcmp(status, STATUS_INV) == 0) { // If the word is invalid
        printf("Invalid trial. Please try again with the same word.\n");
    }

    freeaddrinfo(res);
    return 0;
}

int quitGame(int fd, int exit_code) { // Quit the game
    char command[BUFFERSIZE] = "";
    char message[BUFFERSIZE] = "";
    char status[BUFFERSIZE] = "";
    struct timeval timeout;
    command[1] = '\0';
    message[1] = '\0';
    status[1] = '\0';
    strcpy(message, "");
    strcpy(status, "");
    strcpy(command, "");
    memset(&hints,0,sizeof hints);
    hints.ai_family= AF_INET; //IPv4
    hints.ai_socktype= SOCK_DGRAM; //UDP socket

    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0) {
        fprintf(stderr, "UDP Error: setsockopt failed...%s\n", strerror(errno)); /*error*/
        exit(1);
    }

    errcode=getaddrinfo(GSIP,GSport,&hints,&res); 

    if(errcode!=0) { 
        fprintf(stderr,"Error getting address info: %s\n",gai_strerror(errcode));
        exit(1);
    }

    if (strcmp(PLID, "") == 0) { // If there is no PLID set
        fprintf(stderr, "Error: No PLID set. Use 'start' followed by your PLID to start a new game.\n");
        return 0;
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
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            fprintf(stdout, "UDP Timeout: no message received.\n");
            freeaddrinfo(res);
            return 0;
        }
        fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    sscanf(message, "%s %s", command, status);

    if (!exit_code) { // If the player has asked to quit the game (not the program)
        if (strcmp(status, STATUS_ERR) == 0 || strcmp(command, STATUS_ERR) == 0) 
            printf("An error has ocurred. Please try again.\n");

        else if (strcmp(status, STATUS_NOK) == 0 || strcmp(command, STATUS_NOK) == 0)
            printf("There is not a game being played currently.\n");
        else
            printf("You have quit the game.\n");
    }

    else { // If the player has asked to quit the program
        printf("Goodbye!\n");
        freeaddrinfo(res);
        exit(0);
    }

    freeaddrinfo(res);

    return 0;
}

int scoreboard(int fd) { // Show the scoreboard
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
    n=connect(fd, res->ai_addr, res->ai_addrlen); // Connect to the server
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

    fprintf(stdout, "Downloading scoreboard file...\n"); // Confirm that the file is being downloaded

    memset(message, 0, BUFFERSIZE);
    bytes = MAX_RESPONSE_SIZE; 
    cursor = message;
    printf("fd: %d\n", fd);
    while (bytes > 0) {
        n=read(fd, cursor, bytes);
        sscanf(message, "%s %s %s %lu", command, status, filename, &filesize);
        if (strcmp(status, STATUS_EMPTY) == 0) { // If there are no wins
            printf("There are still no wins! Keep grinding!\n");
            return 0;
        }
        if(n == -1) {
            fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
            exit(1);
        }
        bytes -= (size_t) n;
        cursor += n;
    }

    sprintf(filesize_length, "%lu", filesize);

    protocol_length = strlen(SCOREBOARD_PROTOCOL) + strlen(status) + strlen(filename) + strlen(filesize_length) + 4; // Size of the protocol+ 4 spaces

    bytes = filesize - MAX_RESPONSE_SIZE + protocol_length; // Size of the file

    while (bytes > 0) {
        n=read(fd, cursor, bytes);
        if(n == -1) {
            fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
            exit(1);
        }
        bytes -= (size_t) n;
        cursor += n;
    }

    scores = fopen(filename, "wb"); // Open the file to write the scores

    if (scores == NULL) { // If there is an error opening the file
        fprintf(stderr,"Error opening score file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    if (fwrite(message + protocol_length, 1, filesize, scores) != filesize) { // Write the scores to the file
        fprintf(stderr,"Error writing to score file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    if (fclose(scores) == EOF) { // Close the file
        fprintf(stderr,"Error closing score file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    fprintf(stdout, "Scoreboard saved as %s.\n", filename); // Confirm that the file has been saved

    freeaddrinfo(res);

    return 0;

}

int getHint(int fd) { // Get a hint
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
    if (strcmp(PLID, "") == 0) { // If there is no PLID
        fprintf(stderr, "Error: No PLID set. Use 'start' followed by your PLID to start a new game.\n");
        return 0;
    }
    strcpy(message, HINT_PROTOCOL);
    strcat(message, " ");
    strcat(message, PLID);
    strcat(message, "\n");
    n=connect(fd, res->ai_addr, res->ai_addrlen); // Connect to the server
    if(n==-1) {
        fprintf(stderr,"Error connecting to server: %s\n",strerror(errno));
        exit(1);
    }
    bytes = strlen(message); // Size of the message
    cursor = message; // Cursor to keep track of where we are in the message
    while (bytes > 0) {
        n = write(fd, cursor, bytes);
        if (n == -1) {
            fprintf(stderr,"Error sending message: %s\n",strerror(errno));
            exit(1);
        }
        bytes -= (size_t) n;
        cursor += n;
    }
    
    memset(message, 0, BUFFERSIZE_LARGE); // Clear the message buffer
    bytes = MAX_RESPONSE_SIZE; // Size of the message
    cursor = message;
    fprintf(stdout, "Downloading hint image...\n"); // Downloading the hint image
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

    protocol_length = strlen(HINT_PROTOCOL) + strlen(status) + strlen(filename) + strlen(filesize_length) + 4; // Length of the protocol + 4 spaces

    bytes = filesize - MAX_RESPONSE_SIZE + protocol_length; // Size of the message

    while (bytes > 0) {
        n=read(fd, cursor, bytes);
        if(n == -1) {
            fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
            exit(1);
        }
        bytes -= (size_t) n;
        cursor += n;
    }

    hint = fopen(filename, "wb"); // Open the hint file

    if (hint == NULL) { // If the file cannot be opened
        fprintf(stderr,"Error opening hint file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    if (fwrite(message + protocol_length, 1, filesize, hint) != filesize) { // Write the hint image to the file
        fprintf(stderr,"Error writing to hint file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    if (fclose(hint) == EOF) { // Close the hint file
        fprintf(stderr,"Error closing hint file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    fprintf(stdout, "Hint image saved as '%s'.\n", filename); // Hint image saved

    freeaddrinfo(res);

    return 0;
}


int getState(int fd) { // Get the state of the game
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
    if (strcmp(PLID, "") == 0) { // If the PLID is not set
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

    fprintf(stdout, "Downloading status file...\n"); // Downloading the status file

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

    protocol_length = strlen(HINT_PROTOCOL) + strlen(status) + strlen(filename) + strlen(filesize_length) + 4; // Length of the protocol + 4 spaces

    bytes = filesize - MAX_RESPONSE_SIZE + protocol_length; // Size of the message

    while (bytes > 0) {
        n=read(fd, cursor, bytes);
        if(n == -1) {
            fprintf(stderr,"Error receiving message: %s\n",strerror(errno));
            exit(1);
        }
        bytes -= (size_t) n;
        cursor += n;
    }

    if (strcmp(status, STATUS_NOK) == 0) { // If the status is not OK
        printf("Please start a game before using 'state' command.\n");
        return 0;
    }

    if (strcmp(status, STATUS_ACT) == 0) { // If the status is active
        printf("Ongoing game detected. Received current game summary.\n");
    }

    if (strcmp(status, STATUS_FIN) == 0) { // If the status is finished
        printf("No game detected. Received recent games summary.\n");
    }

    gameinfo = fopen(filename, "wb"); // Open the state file

    if (gameinfo == NULL) { // If the file cannot be opened
        fprintf(stderr,"Error opening state file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    if (fwrite(message + protocol_length, 1, filesize, gameinfo) != filesize) { // Write the state file
        fprintf(stderr,"Error writing to state file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    if (fclose(gameinfo) == EOF) { // Close the state file
        fprintf(stderr,"Error closing state file %s: %s\n",filename, strerror(errno));
        freeaddrinfo(res);
        exit(1);
    }

    fprintf(stdout, "State file saved as '%s'.\n", filename); // State file saved

    freeaddrinfo(res);

    return 0;

}