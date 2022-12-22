#include "envserver.h"
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

int verbose = 0;
int sequential = 0;
char *GSport = DEFAULT_PORT;

void applyModifiers(int argc, char *argv[]) {


    if ((argc >= 3 && strcmp(argv[2], VERBOSE_MODIFIER) == 0) || (argc >= 5 && strcmp(argv[4], VERBOSE_MODIFIER) == 0)) {
        fprintf(stdout, "Verbose mode enabled.\n");
        verbose = 1;
    }

    if ((argc >= 3 && strcmp(argv[2], SEQUENTIAL_MODIFIER) == 0) || (argc >= 4 && strcmp(argv[3], SEQUENTIAL_MODIFIER) == 0) ||(argc >= 5 && strcmp(argv[4], SEQUENTIAL_MODIFIER) == 0) || (argc >= 6 && strcmp(argv[5], SEQUENTIAL_MODIFIER) == 0)) {
        fprintf(stdout, "Sequential mode enabled.\n");
        sequential = 20;
    }


    if (argc > 3 && strcmp(argv[2], PORT_MODIFIER) == 0) {
        fprintf(stdout, "Port number changed to '%s'.\n", argv[3]);
        GSport = argv[3];
    }

    if (argc >= 5 && strcmp(argv[3], PORT_MODIFIER) == 0) {
        fprintf(stdout, "Port number changed to '%s'.\n", argv[4]);
        GSport = argv[4];
    }

    if (argc == 1) {
        fprintf(stderr, "Error: No word file specified. Usage: ./GS word_file [-p GSport] [-v] [-s].\n");
        exit(1);
    }
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

int findTopScores(int score[10], char PLID[10][BUFFERSIZE], char word[10][BUFFERSIZE], int n_succ[10], int n_trials[10]) {
    struct dirent **filelist;
    int n_entries = 0, i_file = 0;
    char fname[BUFFERSIZE] = "";
    FILE *fp = NULL;

    n_entries = scandir("SCORES/", &filelist, 0, alphasort);
    i_file = 0;
    if (n_entries < 0) {
        return 0;
    }
    else {
        while(n_entries--) {
            if (filelist[n_entries]->d_name[0] != '.') {
                sprintf(fname, "SCORES/%s", filelist[n_entries]->d_name);
                fp = fopen(fname, "r");
                fseek(fp, 0, SEEK_SET);
                if (fp != NULL) {
                    fscanf(fp, "%d %s %s %d %d", &score[i_file], PLID[i_file], word[i_file], &n_succ[i_file], &n_trials[i_file]);
                    fclose(fp);
                    i_file++;
                }
            }
            free(filelist[n_entries]);
            if (i_file == 10)
                break;
        }
        free(filelist);
    }
    return i_file;
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

void sequentialLineSelection(FILE *wordfile, char *word, char *guess) {
    int count = 0;
    char *fline =  NULL;
    ssize_t i = 0;
    size_t fline_size = 0;
    while (count < sequential) {
        i = getline(&fline, &fline_size, wordfile);
        count++;
        if (i == -1) {
            fseek(wordfile, 0, SEEK_SET);
            getline(&fline, &fline_size, wordfile);
            sscanf(fline, "%s %s", word, guess);
            sequential = 1;
            break;
        }
        else {
            sscanf(fline, "%s %s", word, guess);
        }
    }
    sequential++;
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

int lettersGuessed(FILE *file, char correct[], char letter) {
    int i = 0;
    int n_letters = 0;
    int status = 0;
    char guess_letter;
    char *fline = NULL;
    char guess[BUFFERSIZE] = "";
    char code[BUFFERSIZE] = "";
    size_t fline_size = 0;

    if (strchr(correct, letter) != NULL) 
        status = 1;
    else
        return 0;
    while (getline(&fline, &fline_size, file) != -1) {
        sscanf(fline, "%s %s", code, guess);
        guess_letter = guess[0];
        if (strlen(guess) == 1 && strchr(correct, guess_letter) != NULL) {
            while (correct[i] != '\0') {
                if (correct[i] == guess_letter) 
                    n_letters++;
                i++;
            }
            i = 0;
        }
    }
    fseek(file, 0, SEEK_SET);
    return n_letters + status;
}

int errorCount(FILE* gamefile, char correct[], char letter) {
    int n_errors = 0;
    int status = 0;
    char *fline = NULL;
    char guess[BUFFERSIZE] = "";
    char code[BUFFERSIZE] = "";
    size_t fline_size = 0;
    if (strchr(correct, letter) == NULL) 
        status = 1;
    else
        return 0;
    while (getline(&fline, &fline_size, gamefile) != -1) {
        sscanf(fline, "%s %s", code, guess);
        if ((strchr(correct, guess[0]) == NULL) && (strcmp(code, CODE_LETTER) == 0))
            n_errors++;
        if ((strcmp(guess, correct) != 0) && (strcmp(code, CODE_GUESS) == 0))
            n_errors++;
    }
    fseek(gamefile, 0, SEEK_SET);
    return n_errors + status;
}

void letterPositions(char message[], char letter, char correct[]) {
    int number_of_letters = 0;
    char number_of_letters_str[BUFFERSIZE] = "";
    char current_letter[BUFFERSIZE] = "";
    number_of_letters_str[0] = '\0';
    current_letter[0] = '\0';

    for (int i = 0;i < strlen(correct); i++) {
        if (correct[i] == letter) 
            number_of_letters++;
        if (i == strlen(correct) - 1) {
            sprintf(number_of_letters_str, "%d", number_of_letters);
            strcat(message, " ");
            strcat(message, number_of_letters_str);
        }
    }
    
    for (int i = 0; i < strlen(correct); i++) {
        if (correct[i] == letter) {
            sprintf(current_letter, "%d", i + 1);
            strcat(message, " ");
            strcat(message, current_letter);
        }
    }
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

void saveScore(FILE* gamefile, char PLID[], char word[], int n_succ, int n_trials) {
    FILE *scorefile;
    int score = 0;
    char sbfname[BUFFERSIZE] = "";
    sbfname[1] = '\0';
    score = (n_succ * 100) / n_trials;
    fseek(gamefile, 0, SEEK_SET);
    if (score < 10) 
        sprintf(sbfname, "SCORES/00%d_%s_%ld.txt", score, PLID, time(NULL));
    else if (score < 100) 
        sprintf(sbfname, "SCORES/0%d_%s_%ld.txt", score, PLID, time(NULL));
    else
        sprintf(sbfname, "SCORES/%d_%s_%ld.txt", score, PLID, time(NULL)); 
    scorefile = fopen(sbfname, "w");
    if (scorefile == NULL) 
        printf("Error creating scorefile: Server error.\n");
    else 
        fprintf(scorefile, "%d %s %s %d %d", score, PLID, word, n_succ, n_trials);
    fclose(scorefile);
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

    sscanf(message + PROTOCOL_SIZE, "%s", PLID);

    strcat(gfname, "GAME_");
    strcat(gfname, PLID);
    strcat(gfname, ".txt");

    wordfile = fopen(WORDFILE, "a+");

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
        if (sequential > 0) 
            sequentialLineSelection(wordfile, word, hint);
        else 
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

    else if (numberOfLines(gamefile) == 1) {
        fscanf(gamefile, "%s %s", word, hint);
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

    if (strlen(word) <= 2) {
        strcat(message, STATUS_ERR);
        strcat(message, "\n");
        return 0;
    }

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
        sprintf(server_tries_str, "%d", server_tries); // FIX HERE
        fclose(gamefile);
        fopen(gfname, "a+");
        saveScore(gamefile, PLID, correct, numberOfLines(gamefile) - errorCount(gamefile, correct, '|'), server_tries);
        fclose(gamefile);
        saveGame(gfname, CODE_WIN, PLID);
        strcat(message, STATUS_WIN);
        strcat(message, " ");
        strcat(message, server_tries_str);
        strcat(message, "\n");
        return 0;
    }

    else if (errorCount(gamefile, correct, '|') > maxErrors(correct)) {
        fprintf(gamefile, "%s %s\n", CODE_GUESS, word);
        fclose(gamefile);
        saveGame(gfname, CODE_FAIL, PLID);
        strcat(message, STATUS_OVR);
        strcat(message, " ");
        strcat(message, server_tries_str);
        strcat(message, "\n");
        return 0;
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

    if (!isalpha(letter)) {
        strcat(message, STATUS_ERR);
        strcat(message, "\n");
        return 0;
    }

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

    else if (lettersGuessed(gamefile, correct, letter) == strlen(correct)) {
        fprintf(gamefile, "%s %c\n", CODE_LETTER, letter);
        sprintf(server_tries_str, "%d", server_tries);
        saveScore(gamefile, PLID, correct, numberOfLines(gamefile) - errorCount(gamefile, correct, letter) + 1, server_tries);
        fclose(gamefile);
        saveGame(gfname, CODE_WIN, PLID);
        strcat(message, STATUS_WIN);
        strcat(message, " ");
        strcat(message, server_tries_str);
        strcat(message, "\n");
        return 0;
    }

    else if (errorCount(gamefile, correct, letter) > maxErrors(correct)) {
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
        strcat(message, STATUS_OK);
        strcat(message, " ");
        strcat(message, server_tries_str);
        letterPositions(message, letter, correct);
        strcat(message, "\n");
        fclose(gamefile);
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

int scoreboard(char message[]) {
    char content[BUFFERSIZE] = "";
    char content_size[BUFFERSIZE] = "";
    int score[10];
    char PLID[10][BUFFERSIZE];
    char word[10][BUFFERSIZE]; 
    int n_succ[10];
    int n_trials[10];
    message[1] = '\0';
    content[1] = '\0';
    content_size[1] = '\0';

    int score_number = findTopScores(score, PLID, word, n_succ, n_trials);
    memset(message, 0, BUFFERSIZE_LARGE);

    printf("score_number is %d\n", score_number);

    if (score_number == 0) {
        printf("No scores found!\n");
        strcat(message, SCOREBOARD_RESPONSE_PROTOCOL);
        strcat(message, " ");
        strcat(message, STATUS_EMPTY);
        strcat(message, "\n");
        return 0;
    }

    for (int i = 0; i < score_number; i++) {
        char order[BUFFERSIZE]= "";
        char score_str[BUFFERSIZE]= "";
        char n_succ_str[BUFFERSIZE]= "";
        char n_tot_str[BUFFERSIZE]= "";
        order[1] = '\0';
        score_str[1] = '\0';
        n_succ_str[1] = '\0';
        n_tot_str[1] = '\0';
        sprintf(score_str, "%d", score[i]);
        sprintf(n_succ_str, "%d", n_succ[i]);
        sprintf(n_tot_str, "%d", n_trials[i]);
        sprintf(order, "%d", i + 1);
        strcat(content, order);
        strcat(content, " - ");
        strcat(content, score_str);
        strcat(content, " ");
        strcat(content, PLID[i]);
        strcat(content, " ");
        strcat(content, word[i]);
        strcat(content, " ");
        strcat(content, n_succ_str);
        strcat(content, " ");
        strcat(content, n_tot_str);
        strcat(content, "\n");
    }


    sprintf(content_size, "%ld", strlen(content));



    strcat(message, SCOREBOARD_RESPONSE_PROTOCOL);
    strcat(message, " ");
    strcat(message, STATUS_OK);
    strcat(message, " ");
    strcat(message, "TOPSCORES.txt");
    strcat(message, " ");
    strcat(message, content_size);
    strcat(message, " ");
    strcat(message, content);
    strcat(message, "\n");

    return 0;
}

int getHint(char message[]) {
    FILE *gamefile = NULL;
    FILE *image = NULL;
    size_t bytes_read = 0;
    size_t image_size = 0;
    char gfname[BUFFERSIZE] = "";
    char PLID[PLIDSIZE + 1] = "";
    char correct[BUFFERSIZE] = "";
    char image_file[BUFFERSIZE] = "";
    char image_name[BUFFERSIZE] = "";
    char *image_content = malloc(BUFFERSIZE_LARGE);

    image_content[1] = '\0';
    gfname[1] = '\0';
    PLID[1] = '\0';
    correct[1] = '\0';
    image_name[1] = '\0';

    sscanf(message + PROTOCOL_SIZE, "%s", PLID);

    strcat(gfname, "GAME_");
    strcat(gfname, PLID);
    strcat(gfname, ".txt");

    memset(message, 0, BUFFERSIZE_LARGE);

    strcat(message, HINT_RESPONSE_PROTOCOL);
    strcat(message, " ");

    gamefile = fopen(gfname, "r");

    if (gamefile == NULL) {
        fprintf(stderr, "Error: Could not find game file.\n");
        sprintf(message, "%s %s\n", HINT_RESPONSE_PROTOCOL, STATUS_NOK);
        return 0;
    }

    fscanf(gamefile, "%s %s", correct, image_file);
    fseek(gamefile, 0, SEEK_SET);
    fclose(gamefile);

    sprintf(image_name, "HINTS/%s", image_file);
    image = fopen(image_name, "r");

    if (image == NULL) {
        fprintf(stderr, "Error: Could not find image file.\n");
        sprintf(message, "%s %s\n", HINT_RESPONSE_PROTOCOL, STATUS_NOK);
        return 0;
    }

    fseek(image, 0, SEEK_END);
    image_size = (size_t) ftell(image);
    fseek(image, 0, SEEK_SET);

    bytes_read = fread(image_content, 1, image_size, image);

    if (bytes_read != image_size) {
        fprintf(stderr, "Error: Could not read image file.\n");
        sprintf(message, "%s %s\n", HINT_RESPONSE_PROTOCOL, STATUS_NOK);
        return 0;
    }

    fclose(image);

    sprintf(message, "%s %s %s %ld ", HINT_RESPONSE_PROTOCOL, STATUS_OK, image_file, image_size);

    free(image_content);
    
    return 0;
}

int getState(char message[]) {
    FILE *gamefile = NULL;
    FILE *last_game = NULL;
    char termination[BUFFERSIZE] = "";
    char last_gfname[BUFFERSIZE] = "";
    char last_gf[BUFFERSIZE] = "";
    char state_fname[BUFFERSIZE] = "";
    char gfname[BUFFERSIZE] = "";
    char PLID[PLIDSIZE + 1] = "";
    char correct[BUFFERSIZE] = "";
    char content[BUFFERSIZE] = "";
    char hint[BUFFERSIZE] = "";
    char content_size[BUFFERSIZE] = "";
    char tries_str[BUFFERSIZE] = "";

    last_gfname[1] = '\0';
    last_gf[1] = '\0';
    gfname[1] = '\0';
    PLID[1] = '\0';
    correct[1] = '\0';
    hint[1] = '\0';
    state_fname[1] = '\0';
    content[1] = '\0';
    content_size[1] = '\0';
    tries_str[1] = '\0';
    termination[1] = '\0';

    sscanf(message + PROTOCOL_SIZE, "%s", PLID);

    strcat(gfname, "GAME_");
    strcat(gfname, PLID);
    strcat(gfname, ".txt");

    strcat(state_fname, "STATE_");
    strcat(state_fname, PLID);
    strcat(state_fname, ".txt");

    memset(message, 0, BUFFERSIZE_LARGE);

    strcat(message, STATE_RESPONSE_PROTOCOL);
    strcat(message, " ");

    gamefile = fopen(gfname, "r");

    if (gamefile == NULL) {
        sprintf(message, "%s %s", STATE_RESPONSE_PROTOCOL, STATUS_FIN);
        findLastGame(PLID, last_gfname);
        sprintf(last_gf, "GAMES/%s", last_gfname);
        printf("Last game file: %s\n", last_gf);
        printf("Last game file name: %s\n", last_gfname);
        last_game = fopen(last_gfname, "r");
        if (strstr(last_gfname, CODE_FAIL) != NULL)
            strcat(termination, CODE_FAIL);
        else if (strstr(last_gfname, CODE_WIN) != NULL)
            strcat(termination, CODE_WIN);
        else
            strcat(termination, CODE_QUIT);
        if (last_game == NULL) {
            fprintf(stderr, "Could not find/open last game file.\n");
            sprintf(message, "%s %s\n", STATE_RESPONSE_PROTOCOL, STATUS_NOK);
            return 0;
        }
        fscanf(last_game, "%s %s", correct, hint);
        sprintf(tries_str, "%d", numberOfLines(last_game) - 1);
        strcat(content, "Last finalized game for player ");
        strcat(content, PLID);
        strcat(content, "\n");
        strcat(content, "Correct word: ");
        strcat(content, correct);
        strcat(content, "; Hint file: ");
        strcat(content, hint);
        strcat(content, "\n");
        strcat(content, "Total number of tries: ");
        strcat(content, tries_str);
        strcat(content, "\n");
        strcat(content, "Termination code: ");
        strcat(content, termination);
        strcat(content, "\n");
        fclose(last_game);
    }

    else {
        sprintf(message, "%s %s", STATE_RESPONSE_PROTOCOL, STATUS_ACT);
        fscanf(gamefile, "%s %s", correct, hint);
        sprintf(tries_str, "%d", numberOfLines(gamefile) - 1);
        strcat(content, "Current game for player ");
        strcat(content, PLID);
        strcat(content, "\n");
        strcat(content, "Correct word: ");
        strcat(content, correct);
        strcat(content, "; Hint file: ");
        strcat(content, hint);
        strcat(content, "\n");
        strcat(content, "Total number of tries: ");
        strcat(content, tries_str);
        strcat(content, "\n");
        fclose(gamefile);
    }

    sprintf(content_size, "%ld", strlen(content));
    strcat(message, " ");
    strcat(message, state_fname);
    strcat(message, " ");
    strcat(message, content_size);
    strcat(message, " ");
    strcat(message, content);
    strcat(message, "\n");

    return 0;

    
}

int quitGame(char message[]) {
    FILE *gamefile = NULL;
    char gfname[BUFFERSIZE] = "";
    char PLID[PLIDSIZE + 1] = "";

    sscanf(message + PROTOCOL_SIZE, "%s", PLID);

    strcat(gfname, "GAME_");
    strcat(gfname, PLID);
    strcat(gfname, ".txt");

    memset(message, 0, BUFFERSIZE);

    strcat(message, QUIT_RESPONSE_PROTOCOL);
    strcat(message, " ");

    gamefile = fopen(gfname, "r");

    if (gamefile == NULL) {
        strcat(message, STATUS_NOK);
        strcat(message, "\n");
    }

    if (gamefile != NULL) {
        fclose(gamefile);
        saveGame(gfname, CODE_QUIT, PLID);
        strcat(message, STATUS_OK);
        strcat(message, "\n");
        return 0;
    }

    return 0;
}

void analyzeMessage(char message[]) {
    char command[BUFFERSIZE] = "";
    command[1] = '\0';
    sscanf(message, "%s", command);

    if ((strcmp(command, START_GAME_PROTOCOL) == 0) && (strlen(message) == PROTOCOL_SIZE + PLIDSIZE + 2)) 
        startGame(message);

    else if (strcmp(command, PLAY_LETTER_PROTOCOL) == 0) 
        playLetter(message);

    else if (strcmp(command, GUESS_WORD_PROTOCOL) == 0) 
        guessWord(message);

    else if (strcmp(command, SCOREBOARD_PROTOCOL) == 0) 
        scoreboard(message);
    
    else if (strcmp(command, HINT_PROTOCOL) == 0) 
        getHint(message);

    else if (strcmp(command, STATE_PROTOCOL) == 0) 
        getState(message);

    else if (strcmp(command, QUIT_PROTOCOL) == 0 && (strlen(message) == PROTOCOL_SIZE + PLIDSIZE + 2)) 
        quitGame(message);
    
    else {
        memset(message, 0, BUFFERSIZE);
        strcat(message, STATUS_ERR);
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
        fprintf(stderr, "UDP Error: server address binding failed...%s\n", strerror(errno)); /*error*/
        exit(1);
    }

    while(1) {
        memset(message, 0, BUFFERSIZE);
        addrlen=sizeof(addr);
        fprintf(stdout, "Waiting for UDP message...\n");
        nread=recvfrom(fd,message,BUFFERSIZE,0,(struct sockaddr*)&addr, &addrlen);

        if(nread==-1) { /*error*/
            fprintf(stderr, "Error receiving message: %s\n", strerror(errno));
            freeaddrinfo(res);
            exit(1);
        }

        if (verbose == 1) {
            fprintf(stdout, "UDP Message received from %s:%d\n", inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
            fprintf(stdout, "Message content: %s\n", message);
        }

        analyzeMessage(message);

        n=sendto(fd,message, (size_t)strlen(message),0,(struct sockaddr*)&addr, addrlen);

        if(n==-1) {
            fprintf(stderr, "Error sending message: %s\n", strerror(errno));
            freeaddrinfo(res);
            exit(1);
        }

        if (verbose == 1) {
            fprintf(stdout, "UDP Message sent to %s:%d\n", inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
            fprintf(stdout, "Message content: %s\n", message);
        }
    }
    freeaddrinfo(res);
    close(fd);
    return 0;
}

int receiveTCP(int fd) {
    struct addrinfo hints, *res;
    int ret, newfd, errcode; 
    pid_t pid;
    ssize_t n;
    size_t bytes;
    struct sockaddr_in addr;
    socklen_t addrlen;
    struct sigaction act;
    char *cursor, message[BUFFERSIZE_LARGE] = "";
    message[1] = '\0';
    
    memset(&act,0,sizeof act);

    act.sa_handler=SIG_IGN;

    if (sigaction(SIGPIPE,&act,NULL) == -1) {
        fprintf(stderr, "TCP Error: sigaction failed.\n");
        exit(1);
    }

    if (sigaction(SIGCHLD, &act, NULL) == -1) {
        fprintf(stderr, "TCP Error: sigaction failed.\n");
        exit(1);
    }
    
    if ((fd=socket(AF_INET,SOCK_STREAM,0)) == -1) {
        fprintf(stderr, "Socket creation failed.\n");
        exit(1);
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
        fprintf(stderr, "TCP Error: setsockopt(SO_REUSEADDR) failed.\n");
        exit(1);
    }

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;//IPv4 
    hints.ai_socktype=SOCK_STREAM;//TCP socket 
    hints.ai_flags=AI_PASSIVE;

    errcode=getaddrinfo(NULL,GSport,&hints,&res); 
    
    if(errcode!=0) { 
        fprintf(stderr,"Error getting address info: %s\n",gai_strerror(errcode)); /*error*/
        exit(1);
    }

    if(bind(fd,res->ai_addr,res->ai_addrlen)==-1) {
        fprintf(stderr, "TCP Error: server address binding failed...%s.\n", strerror(errno)); /*error*/
        exit(1);
    }

    if(listen(fd, 5) == -1) {
        fprintf(stderr, "TCP Error: server failed to listen...%s\n", strerror(errno)); /*error*/
        freeaddrinfo(res);
        exit(1);
    }

    while(1) {

        addrlen=sizeof(addr);

        memset(message, 0, BUFFERSIZE_LARGE);

        fprintf(stdout, "Waiting for TCP message...\n");

        do newfd = accept(fd, (struct sockaddr*)&addr, &addrlen);
        while (newfd == -1 && errno == EINTR);

        if (newfd == -1) {
            fprintf(stderr, "TCP Error: server failed to accept...%s\n", strerror(errno)); /*error*/
            freeaddrinfo(res);
            exit(1);
        }

        if ((pid = fork()) == -1) {
            fprintf(stderr, "TCP Error: server failed to fork...%s\n", strerror(errno)); /*error*/
            freeaddrinfo(res);
            exit(1);
        }

        else if (pid == 0) {
            bytes = PROTOCOL_SIZE;
            cursor = message;
            while (bytes > 0) {
                n = read(newfd, cursor, bytes);
                if (n == -1) {
                    fprintf(stderr, "TCP Error: server failed to receive...%s\n", strerror(errno)); /*error*/
                    freeaddrinfo(res);
                    exit(1);
                }
                bytes -= (size_t) n;
                cursor += n;
            }

            if (strcmp(message, HINT_PROTOCOL) == 0 || strcmp(message, STATE_PROTOCOL) == 0) {
                bytes = PLIDSIZE + 1;
                while (bytes > 0) {
                    n = read(newfd, cursor, bytes);
                    if (n == -1) {
                        fprintf(stderr, "TCP Error: server failed to receive...%s\n", strerror(errno)); /*error*/
                        freeaddrinfo(res);
                        exit(1);
                    }
                    bytes -= (size_t) n;
                    cursor += n;
                }
            }

            if (verbose == 1) {
                fprintf(stdout, "TCP Message received from %s:%d\n", inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
                fprintf(stdout, "Message content: %s\n", message);
            }

            analyzeMessage(message);


            bytes = strlen(message);
            cursor = message;
            while (bytes > 0) {
                n = write(newfd, cursor, bytes);
                if (n == -1) {
                    fprintf(stderr, "TCP Error: server failed to send...%s\n", strerror(errno)); /*error*/
                    freeaddrinfo(res);
                    exit(1);
                }
                bytes -= (size_t) n;
                cursor += n;
            }

            if (verbose == 1) {
                fprintf(stdout, "TCP Message sent to %s:%d\n", inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
                fprintf(stdout, "Message content: %s\n", message);
            }
        }
        
        if (pid > 0) {
            do ret = close(newfd);
            while (close(newfd) == -1 && errno == EINTR);
            if (ret == -1) {
                fprintf(stderr, "TCP Error: server failed to close...%s\n", strerror(errno)); /*error*/
                freeaddrinfo(res);
                exit(1);
            }
        }
        
    }
        
    freeaddrinfo(res);
    close(fd);
    return 0;
}

