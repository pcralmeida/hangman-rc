#ifndef ENVSERVER_H
#define ENVSERVER_H


#define BUFFERSIZE 4096 // Buffer size for the socket (2^12 = 4096, chosen because it is a power of 2)
#define PLIDSIZE 6 // Size of the player ID (6 characters)
#define PROTOCOL_SIZE 3 // Size of the protocol "XXX" (3 characters)
#define DEFAULT_PORT "58079" // The group number is 079, DEFAULT_PORT = PORT + 079
#define VERBOSE_MODIFIER "-v" // The verbose modifier argument
#define PORT_MODIFIER "-p" // The port modifier argument
#define WORDFILE "wordfile.txt" // The word file
#define WORDFILE_SIZE 648 // The size of the word file

#define START_GAME_PROTOCOL "SNG" // Start game protocol
#define GUESS_WORD_PROTOCOL "PWG" // Guess protocol
#define PLAY_LETTER_PROTOCOL "PLG" // Play protocol
#define SCOREBOARD_PROTOCOL "GSB" // Scoreboard protocol
#define HINT_PROTOCOL "GHL" // Hint protocol
#define STATE_PROTOCOL "STA" // State protocol
#define REVEAL_PROTOCOL "REV" // Reveal protocol
#define QUIT_PROTOCOL "QUT" // Quit protocol

#define START_GAME_RESPONSE_PROTOCOL "RSG" //Start game response protocol
#define PLAY_LETTER_RESPONSE_PROTOCOL "RLG" //Play letter response protocol
#define GUESS_WORD_RESPONSE_PROTOCOL "RWG" //Guess word response protocol
#define QUIT_RESPONSE_PROTOCOL "RQT" //Quit response protocol
#define REVEAL_RESPONSE_PROTOCOL "RRV" //Game over response protocol
#define SCOREBOARD_RESPONSE_PROTOCOL "RSB" //Scoreboard response protocol
#define HINT_RESPONSE_PROTOCOL "RHL" //Hint response protocol
#define STATE_RESPONSE_PROTOCOL "RST" //State response protocol
#define ERROR_RESPONSE_PROTOCOL "ERR" //Error response protocol

#define STATUS_OK "OK" // Status OK
#define STATUS_NOK "NOK" // Status NOK
#define STATUS_DUP "DUP" // Status DUP
#define STATUS_ERR "ERR" // Status ERR
#define STATUS_INV "INV" // Status INV
#define STATUS_WIN "WIN" // Status WIN
#define STATUS_OVR "OVR" // Status OVR
#define STATUS_EMPTY "EMPTY" // Status EMPTY
#define STATUS_ACT "ACT" // Status ACT
#define STATUS_FIN "FIN" // Status FIN

#define CODE_LETTER "L" // Code LETTER
#define CODE_GUESS "G" // Code GUESS
#define CODE_WIN "W" // Code WIN
#define CODE_FAIL "F" // Code FAIL
#define CODE_QUIT "Q" // Code QUIT

struct SCORELIST {
    int* score;
    char** PLID;
    char** word;
    int* n_succ;
    int* n_tot;
    int* n_scores;
};

void applyModifiers(int argc, char *argv[]); // Apply the modifiers
int receiveUDP(int fd); // Receive UDP packets

#endif