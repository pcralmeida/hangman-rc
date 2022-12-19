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

char buffer[BUFFERSIZE];
int tcp_fd, udp_fd;
pid_t pid;

int main(int argc, char *argv[]) {

    applyModifiers(argc, argv);

    if ((pid = fork()) == -1) {
        fprintf(stderr, "Error: Child process could not be created.\n");
        exit(1);
    }

    else if (pid > 0) { // Parent process handles UDP
        printf("UDP server is now open.\n");
        receiveUDP(udp_fd);
        printf("UDP server is now closed.\n");
    }

    else if (pid == 0) { // Child process handles TCP
        printf("TCP server is now open.\n");
        receiveTCP(tcp_fd);
        printf("TCP server is now closed.\n");
    }

    return 0;

}