#ifndef SALON_H
#define SALON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>

#include "reseau.h"
#include "reseau_ext.h"
#include "chess.h"
#include "ds.h"

#define MAX_PSEUDO 32
#define MAX_PLAYERS 10
#define PORT_SERVEUR 8080

#ifndef SO_REUSEPORT
#define SO_REUSEPORT SO_REUSEADDR
#endif




typedef enum {
    ROLE_WHITE = 0,
    ROLE_BLACK = 1,
    ROLE_SPECTATOR = 2
} role_t;

typedef struct {
    int fd;
    struct sockaddr_in addr;
    char pseudo[MAX_PSEUDO];
    role_t role;
    int actif;
} client_t;




extern client_t clients[MAX_PLAYERS];
extern int nb_clients;
extern chess_state_t partie_salon;
extern int port_salon;






void creer_salon_tcp(int port);


void salon_init_jeu(void);
void salon_traiter_paquet(int client_idx, paquet_t *pkt);
void salon_diffuser(paquet_t *pkt, int except_fd);
void salon_envoyer_etat_complet(int client_idx);

#endif 
