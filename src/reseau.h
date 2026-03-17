#ifndef RESEAU_H
#define RESEAU_H

#include <netinet/in.h>

// Définition des types de messages possibles sur le réseau
typedef enum {
    MSG_JOIN,      // Un client demande à rejoindre le lobby
    MSG_WELCOME,   // Le serveur accepte la connexion
    MSG_MOVE,      // Un joueur envoie un déplacement (ex: e2e4)
    MSG_CHAT       // Un message de discussion dans un salon
} type_msg_t;

// Structure universelle du paquet qui voyagera en UDP et TCP
typedef struct {
    type_msg_t type;          // Le type d'action
    char pseudo[32];          // Le nom de l'expéditeur
    char data[256];           // Le contenu (texte du chat, ou coordonnées du coup)
} paquet_t;

#endif // RESEAU_H