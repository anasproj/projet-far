#ifndef TOURNOI_H
#define TOURNOI_H

#include <pthread.h>

#define MAX_PSEUDO 32
#define MAX_TOURNOI_PLAYERS 8

typedef enum {
    TOURNOI_INACTIF = 0,
    TOURNOI_INSCRIPTIONS,
    TOURNOI_EN_COURS,
    TOURNOI_TERMINE
} tournoi_status_t;

typedef struct {
    int is_active;
    tournoi_status_t status;
    int max_players;
    int nb_registered;
    int current_round;
    int registered[MAX_TOURNOI_PLAYERS];
    char pseudos[MAX_TOURNOI_PLAYERS][MAX_PSEUDO];
    pthread_mutex_t lock;
} Tournoi;

void init_tournoi(void);
int create_tournoi(int max_players);
int join_tournoi(int client_sock, const char *pseudo);
void tournoi_match_result(const char *winner_pseudo);
const char *tournoi_last_error(void);

#endif // TOURNOI_H
