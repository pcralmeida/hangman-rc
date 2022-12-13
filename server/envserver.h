#ifndef ENVSERVER_H
#define ENVSERVER_H

#include <time.h>
#define DEFAULT_PORT 58079; // The group number is 079, DEFAULT_PORT = PORT + 079

struct SCORELIST {
    int* score;
    char** PLID;
    char** word;
    int* n_succ;
    int* n_tot;
    int* n_scores;
};


#endif