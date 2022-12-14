#ifndef ENVCLIENT_H
#define ENVCLIENT_H


#define BUFFERSIZE 4096 // Buffer size for the socket (2^12 = 4096, chosen because it is a power of 2)
#define BUFFERSIZE_LARGE 8000000 // Buffer size for images 
#define MAX_RESPONSE_SIZE 44 // Maximum size of the response (39 characters + 4 spaces + null terminator)
#define PLIDSIZE 7 // Size of the player ID (6 characters + null terminator)
#define TRIAL_INIT 1 // Initial number of trials
#define MAX_PRINTED_WORD_LENGTH 61 // Maximum length of the word (spaces included) to be printed (60 characters + null terminator)
#define MIN_PRINTED_WORD_LENGTH 7 // Minimum length of the word (spaces included) to be printed (6 characters + null terminator)
#define MAX_WORD_LENGTH 31 // Maximum word length (30 characters + null terminator)
#define MIN_WORD_LENGTH 4 // Minimum word length (3 characters + null terminator)
#define DEFAULT_PORT "58079" // The group number is 079, DEFAULT_PORT = PORT + 079
#define LOCALHOST "127.0.0.1" // The local host address
#define IP_MODIFIER "-n" // The IP modifier argument
#define PORT_MODIFIER "-p" // The port modifier argument

#define START_GAME "sg" // Start game command
#define START_GAME_LONG "start" // Start game command
#define START_GAME_PROTOCOL "SNG" // Start game protocol
#define GUESS_WORD "gw" // Guess command
#define GUESS_WORD_LONG "guess" // Guess command
#define GUESS_WORD_PROTOCOL "PWG" // Guess protocol
#define PLAY_LETTER "pl" // Play command
#define PLAY_LETTER_LONG "play" // Play command
#define PLAY_LETTER_PROTOCOL "PLG" // Play protocol
#define SCOREBOARD "sb" // Scoreboard command
#define SCOREBOARD_LONG "scoreboard" // Scoreboard command
#define SCOREBOARD_PROTOCOL "GSB" // Scoreboard protocol
#define HINT "h" // Hint command
#define HINT_LONG "hint" // Hint command
#define HINT_PROTOCOL "GHL" // Hint protocol
#define STATE "st" // State command
#define STATE_LONG "state" // State command
#define STATE_PROTOCOL "STA" // State protocol
#define REVEAL "rev" // Reveal command
#define REVEAL_PROTOCOL "REV" // Reveal protocol
#define QUIT "quit" // Quit command
#define QUIT_PROTOCOL "QUT" // Quit protocol
#define EXIT "exit" // Exit command

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

extern char *GSIP; // The IP address of the game server
extern char *GSport; // The port of the game server

void signalHandler(); // Signal handler
void applyModifiers(int argc, char *argv[]); // Apply the modifiers
void spaceGenerator(int n, char *str); // Function to generate spaces
int startGame(int fd, char *Plid); // Function to start a game
int playLetter(int fd, char *letter); // Function to play a letter
int guessWord(int fd, char *guess); // Function to guess a word
int quitGame(int fd, int exit_code); // Function to quit a game
int revealWord(int fd); // Function to reveal the correct word
int scoreboard(int fd); // Function to print the scoreboard
int getHint(int fd); // Function to print a hint file
int getState(int fd); // Function to print the state of the game or the recent games

#endif