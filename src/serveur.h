#ifndef SERVEUR_H
#define SERVEUR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>

#include "reseau.h"
#include "reseau_ext.h"
#include "salon.h"

#define MAX_JOUEURS 100
#define PORT_SERVEUR 8080




typedef struct {
    char pseudo[MAX_PSEUDO];
    struct sockaddr_in addr;
    int actif;
    int en_attente;
    int dans_tournoi;
} joueur_t;






extern joueur_t lobby[MAX_JOUEURS];
extern int nb_joueurs;
extern int udp_fd_serveur;
extern salon_info_t salons[MAX_SALONS];
extern int nb_salons;


extern joueur_t file_tournoi[MAX_JOUEURS];
extern int nb_tournoi;
extern joueur_t vainqueurs[MAX_JOUEURS];
extern int nb_vainqueurs;






void serveur_traiter_matchmaking(void);
void serveur_traiter_tournoi(void);
void serveur_ajouter_salon(int port, const char *p1, const char *p2);
void serveur_retirer_salon(int port);
void serveur_traiter_vainqueurs(void);


int trouver_joueur(const char *pseudo);
void envoyer_paquet_udp(paquet_t *pkt, struct sockaddr_in *dest);

#endif 
