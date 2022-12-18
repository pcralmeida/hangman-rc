#include "envserver.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
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

int maxErrors(char word[]) {
    if (strlen(word) <= 6) {
        return 7;
    }
    else if (strlen(word) >= 7 && strlen(word) <= 10) {
        return 8;
    }
    else if (strlen(word) >= 11) {
        return 9;
    }

    return -1;
    
}

void randomLineSelection(FILE *wordfile, char *word, char *guess) {
    int line_number = 1;
    int value = 0;
    char *fline =  NULL;
    size_t fline_size = 0;
    srand((unsigned int) time(NULL));
    value = rand() % 26;
    while (getline(&fline, &fline_size, wordfile) != -1) {
        if (value == (line_number - 1)) {
            sscanf(fline, "%s %s", word, guess);
            break;
        }
        line_number++;
    }
    fseek(wordfile, 0, SEEK_SET);
}

int numberOfLines(FILE *file) {
    int n_lines = 0;
    char *fline = NULL;
    size_t fline_size = 0;
    while (getline(&fline, &fline_size, file) != -1) {
        n_lines++;
    }
    fseek(file, 0, SEEK_SET);
    return n_lines;
}

int repeatedGuess(FILE *file, char new_guess[]) {
    char *fline = NULL;
    char guess[BUFFERSIZE] = "";
    char code[BUFFERSIZE] = "";
    size_t fline_size = 0;
    while (getline(&fline, &fline_size, file) != -1) {
        sscanf(fline, "%s %s", code, guess);
        if (strcmp(new_guess, guess) == 0) {
            fseek(file, 0, SEEK_SET);
            return 1;
        }
    }
    fseek(file, 0, SEEK_SET);
    return 0;
}

int lettersGuessed(FILE *file, char correct[]) {
    int n_letters = 0;
    char guess_letter;
    char *fline = NULL;
    char guess[BUFFERSIZE] = "";
    char code[BUFFERSIZE] = "";
    size_t fline_size = 0;
    while (getline(&fline, &fline_size, file) != -1) {
        sscanf(fline, "%s %s", code, guess);
        guess_letter = guess[0];
        if (strlen(guess) == 1 && strchr(correct, guess_letter) != NULL)
            n_letters++;
    }
    fseek(file, 0, SEEK_SET);
    return n_letters;
}

void saveGame(char gfname[], char code[], char PLID[]) {
    char savedfilename[BUFFERSIZE] = "";
    char dirname[BUFFERSIZE] = "";
    savedfilename[1] = '\0';
    dirname[1] = '\0';
    sprintf(dirname, "GAMES/%s", PLID);
    mkdir(dirname, 0777);
    sprintf(savedfilename, "%s/%ld_(%s).txt", dirname, time(NULL), code);
    rename(gfname, savedfilename);
}

int findLastGame(char *PLID, char *fname) {
    struct dirent **filelist;
    int n_entries, found;
    char dirname [20] ;
    sprintf(dirname , "GAMES/%s/", PLID);
    n_entries = scandir(dirname ,&filelist, 0, alphasort);
    found = 0;

    if (n_entries <= 0)
        return 0;

    else {
        while (n_entries--) {
            if (filelist[n_entries]->d_name[0] != '.') {
                sprintf(fname, "GAMES/%s/%s" ,PLID, filelist[n_entries]->d_name);
                found = 1;
            }
            free(filelist[n_entries]);
            if(found)
                break;
        }
        free(filelist);
    }
    return found;
}

int startGame(char message[]) {
    FILE *wordfile = NULL;
    FILE *gamefile = NULL;
    ssize_t fsize = 0;
    char PLID[PLIDSIZE + 1] = "";
    char word[BUFFERSIZE] = "";
    char hint[BUFFERSIZE] = "";
    char gfname[BUFFERSIZE] = "";
    PLID[1] = '\0';
    word[1] = '\0';
    hint[1] = '\0';
    gfname[1] = '\0';

    printf("message size: %d\n", (int) strlen(message));

    sscanf(message + PROTOCOL_SIZE, "%s", PLID);

    strcat(gfname, "GAME_");
    strcat(gfname, PLID);
    strcat(gfname, ".txt");

    wordfile = fopen(WORDFILE, "r");

    if (wordfile == NULL) {
        fprintf(stderr,"Error opening word file %s: %s\n", WORDFILE, strerror(errno));
        exit(1);
    }

    memset(message, 0, BUFFERSIZE);
    strcat(message, START_GAME_RESPONSE_PROTOCOL);
    strcat(message, " ");

    gamefile = fopen(gfname, "a+");

    if (gamefile == NULL) {
        fprintf(stderr,"Error opening game file %s: %s\n",gfname, strerror(errno));
        exit(1);
    }

    fseek(gamefile, 0, SEEK_END);
    fsize = ftell(gamefile);
    fseek(gamefile, 0, SEEK_SET);

    if (fsize == 0) {
        randomLineSelection(wordfile, word, hint);
        fprintf(gamefile, "%s %s\n", word, hint);
        strcat(message, STATUS_OK);
        strcat(message, " ");
        sprintf(gfname, "%ld", strlen(word));
        strcat(message, gfname);
        strcat(message, " ");
        sprintf(word, "%d", maxErrors(word));
        strcat(message, word);
        strcat(message, "\n");
    }

    else if (fscanf(gamefile, "%s %s", word, hint) != EOF) {
        strcat(message, STATUS_OK);
        strcat(message, " ");
        sprintf(gfname, "%ld", strlen(word));
        strcat(message, gfname);
        strcat(message, " ");
        sprintf(word, "%d", maxErrors(word));
        strcat(message, word);
        strcat(message, "\n");
    }

    
    else {
        strcat(message, STATUS_NOK);
        strcat(message, "\n");
    }

    fclose(wordfile);
    fclose(gamefile);

    return 0;
}

int guessWord(char message[]) {
    FILE *gamefile = NULL;
    int tries = 0, server_tries = 0;
    char PLID[PLIDSIZE + 1] = "";
    char correct[BUFFERSIZE] = "";
    char word[BUFFERSIZE] = "";
    char gfname[BUFFERSIZE] = "";
    char server_tries_str[BUFFERSIZE] = "";
    PLID[1] = '\0';
    correct[1] = '\0';
    word[1] = '\0';
    gfname[1] = '\0';
    server_tries_str[1] = '\0';

    sscanf(message + PROTOCOL_SIZE, "%s %s %d", PLID, word, &tries);

    strcat(gfname, "GAME_");
    strcat(gfname, PLID);
    strcat(gfname, ".txt");

    memset(message, 0, BUFFERSIZE);

    strcat(message, GUESS_WORD_RESPONSE_PROTOCOL);
    strcat(message, " ");

    gamefile = fopen(gfname, "r");

    if (gamefile == NULL) {
        fprintf(stderr,"Error opening game file %s: %s\n",gfname, strerror(errno));
        strcat(message, STATUS_ERR);
        strcat(message, "\n");
        return 0;
    }

    fclose(gamefile);

    gamefile = fopen(gfname, "a+");

    fscanf(gamefile, "%s", correct);

    server_tries = numberOfLines(gamefile);

    sprintf(server_tries_str, "%d", server_tries);

    if (tries != server_tries) {
        strcat(message, STATUS_INV);
        strcat(message, " ");
        strcat(message, server_tries_str);
        strcat(message, "\n");
        fclose(gamefile);
        return 0;
    }

    else if (repeatedGuess(gamefile, word)) {
        strcat(message, STATUS_DUP);
        strcat(message, " ");
        strcat(message, server_tries_str);
        strcat(message, "\n");
        fclose(gamefile);
        return 0;
    }

    if (strcmp(word, correct) == 0) {
        fprintf(gamefile, "%s %s\n", CODE_GUESS, word);
        sprintf(server_tries_str, "%d", server_tries + 1);
        fclose(gamefile);
        saveGame(gfname, CODE_WIN, PLID);
        strcat(message, STATUS_WIN);
        strcat(message, " ");
        strcat(message, server_tries_str);
        strcat(message, "\n");
        return 0;
    }

    else if (server_tries > maxErrors(correct)) {
        fprintf(gamefile, "%s %s\n", CODE_GUESS, word);
        fclose(gamefile);
        saveGame(gfname, CODE_FAIL, PLID);
        strcat(message, STATUS_OVR);
        strcat(message, " ");
        strcat(message, server_tries_str);
        strcat(message, "\n");
    }

    else {
        fprintf(gamefile, "%s %s\n", CODE_GUESS, word);
        sprintf(server_tries_str, "%d", server_tries + 1);
        strcat(message, STATUS_NOK);
        strcat(message, " ");
        strcat(message, server_tries_str);
        strcat(message, "\n");
    }

    fclose(gamefile);

    return 0;
}

int playLetter(char message[]) {
    FILE *gamefile = NULL;
    int tries = 0, server_tries = 0;
    char letter;
    char PLID[PLIDSIZE + 1] = "";
    char correct[BUFFERSIZE] = "";
    char gfname[BUFFERSIZE] = "";
    char server_tries_str[BUFFERSIZE] = "";
    PLID[1] = '\0';
    correct[1] = '\0';
    gfname[1] = '\0';
    server_tries_str[1] = '\0';

    sscanf(message + PROTOCOL_SIZE, "%s %c %d", PLID, &letter, &tries);

    strcat(gfname, "GAME_");
    strcat(gfname, PLID);
    strcat(gfname, ".txt");

    memset(message, 0, BUFFERSIZE);

    strcat(message, PLAY_LETTER_RESPONSE_PROTOCOL);
    strcat(message, " ");

    gamefile = fopen(gfname, "r");

    if (gamefile == NULL) {
        fprintf(stderr,"Error opening game file %s: %s\n",gfname, strerror(errno));
        strcat(message, STATUS_ERR);
        strcat(message, "\n");
        return 0;
    }

    fclose(gamefile);

    gamefile = fopen(gfname, "a+");

    fscanf(gamefile, "%s", correct);

    server_tries = numberOfLines(gamefile);

    sprintf(server_tries_str, "%d", server_tries);

    if (tries != server_tries) {
        printf("tries, server_tries: %d, %d", tries, server_tries);
        strcat(message, STATUS_INV);
        strcat(message, " ");
        strcat(message, server_tries_str);
        strcat(message, "\n");
    }

    else if (repeatedGuess(gamefile, (char[]) {letter, '\0'})) {
        strcat(message, STATUS_DUP);
        strcat(message, " ");
        strcat(message, server_tries_str);
        strcat(message, "\n");
    }

    else if (lettersGuessed(gamefile, correct) == strlen(correct)) {
        fprintf(gamefile, "%s %c\n", CODE_LETTER, letter);
        sprintf(server_tries_str, "%d", server_tries + 1);
        fclose(gamefile);
        saveGame(gfname, CODE_WIN, PLID);
        strcat(message, STATUS_WIN);
        strcat(message, " ");
        strcat(message, server_tries_str);
        strcat(message, "\n");
        return 0;
    }

    else if (server_tries > maxErrors(correct)) {
        printf("server_tries: %d, maxErrors: %d\n", server_tries, maxErrors(correct));
        fprintf(gamefile, "%s %c\n", CODE_LETTER, letter);
        fclose(gamefile);
        saveGame(gfname, CODE_FAIL, PLID);
        strcat(message, STATUS_OVR);
        strcat(message, " ");
        strcat(message, server_tries_str);
        strcat(message, "\n");
        return 0;
    }

    else if (strchr(correct, letter) != NULL) {
        fprintf(gamefile, "%s %c\n", CODE_LETTER, letter);
        sprintf(server_tries_str, "%d", server_tries + 1);
        strcat(message, STATUS_OK);
        strcat(message, " ");
        strcat(message, server_tries_str);
        for (int i = 0, j = 0; i < strlen(correct); i++) {
            if (correct[i] == letter) 
                j++;
            if (i == strlen(correct) - 1) {
                strcat(message, " ");
                strcat(message, (char[]) {(char) (j + '0'), '\0'});
            }
        }
        for (int i = 0; i < strlen(correct); i++) {
            if (correct[i] == letter) {
                strcat(message, " ");
                strcat(message, (char[]) {(char) (i + '1'), '\0'});
            }
        }
        strcat(message, "\n");
        return 0;
    }

    else {
        fprintf(gamefile, "%s %c\n", CODE_LETTER, letter);
        sprintf(server_tries_str, "%d", server_tries + 1);
        strcat(message, STATUS_NOK);
        strcat(message, " ");
        strcat(message, server_tries_str);
        strcat(message, "\n");
    }

    fclose(gamefile);

    return 0;
}

void analyzeMessage(char message[]) {
    char command[BUFFERSIZE] = "";
    command[1] = '\0';
    sscanf(message, "%s", command);

    if ((strcmp(command, START_GAME_PROTOCOL) == 0)) {
        printf("Start game command detected.\n");
        startGame(message);

    }

    else if (strcmp(command, PLAY_LETTER_PROTOCOL) == 0) {
        printf("Play letter command detected.\n");
        playLetter(message);
    }

    else if (strcmp(command, GUESS_WORD_PROTOCOL) == 0) {
        printf("Guess word command detected.\n");
        guessWord(message);
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
        memset(message, 0, BUFFERSIZE);
        addrlen=sizeof(addr);
        fprintf(stdout, "Waiting for message...\n");
        nread=recvfrom(fd,message,BUFFERSIZE,0,(struct sockaddr*)&addr, &addrlen);
        if(nread==-1) {/*error*/
            fprintf(stderr, "Error receiving message: %s\n", strerror(errno));
            exit(1);
        }

        if (verbose == 1) {
            fprintf(stdout, "Message received from %s:%d\n", inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
            fprintf(stdout, "Message content: %s", message);
        }

        analyzeMessage(message);

        n=sendto(fd,message, (size_t)nread,0,(struct sockaddr*)&addr, addrlen);

        if(n==-1) {
            fprintf(stderr, "Error sending message: %s\n", strerror(errno));
            exit(1);
        }

        if (verbose == 1) {
            fprintf(stdout, "Message sent to %s:%d\n", inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
            fprintf(stdout, "Message content: %s", message);
        }
    }
    freeaddrinfo(res);
    close(fd);
    return 0;
}
