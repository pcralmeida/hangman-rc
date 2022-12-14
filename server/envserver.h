#ifndef ENVSERVER_H
#define ENVSERVER_H


#define BUFFERSIZE 4096 // Buffer size for the socket (2^12 = 4096, chosen because it is a power of 2)
#define DEFAULT_PORT "58079" // The group number is 079, DEFAULT_PORT = PORT + 079
#define VERBOSE_MODIFIER "-v" // The verbose modifier argument
#define PORT_MODIFIER "-p" // The port modifier argument

#define START_GAME_RESPONSE_PROTOCOL "RSG" //Start game response protocol
#define PLAY_LETTER_RESPONSE_PROTOCOL "RLG" //Play letter response protocol
#define GUESS_WORD_RESPONSE_PROTOCOL "RWG" //Guess word response protocol
#define QUIT_RESPONSE_PROTOCOL "RQT" //Quit response protocol
#define REVEAL_RESPONSE_PROTOCOL "RRV" //Game over response protocol
#define SCOREBOARD_RESPONSE_PROTOCOL "RSB" //Scoreboard response protocol
#define HINT_RESPONSE_PROTOCOL "RHL" //Hint response protocol
#define STATE_RESPONSE_PROTOCOL "RST" //State response protocol
#define ERROR_RESPONSE_PROTOCOL "ERR" //Error response protocol

struct SCORELIST {
    int* score;
    char** PLID;
    char** word;
    int* n_succ;
    int* n_tot;
    int* n_scores;
};

void applyModifiers(int argc, char *argv[]); // Apply the modifiers

#endif