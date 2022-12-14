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

    return 0;

}