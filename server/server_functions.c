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