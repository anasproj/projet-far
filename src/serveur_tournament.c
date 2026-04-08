#include "serveur.h"


joueur_t lobby[MAX_JOUEURS] = {0};
int nb_joueurs = 0;
int udp_fd_serveur = -1;
salon_info_t salons[MAX_SALONS] = {0};
int nb_salons = 0;

joueur_t file_tournoi[MAX_JOUEURS] = {0};
int nb_tournoi = 0;

void serveur_ajouter_salon(int port, const char *p1, const char *p2) {
    if (nb_salons < MAX_SALONS) {
        salons[nb_salons].id = port;
        strncpy(salons[nb_salons].joueur1, p1, MAX_PSEUDO - 1);
        strncpy(salons[nb_salons].joueur2, p2, MAX_PSEUDO - 1);
        salons[nb_salons].actif = 1;
        nb_salons++;
    }
}

void serveur_retirer_salon(int port) {
    for (int i = 0; i < nb_salons; i++) {
        if (salons[i].id == port) {
            salons[i] = salons[nb_salons - 1];
            nb_salons--;
            break;
        }
    }
}

void serveur_traiter_matchmaking(void) {
    
    int p1 = -1, p2 = -1;
    for (int i = 0; i < MAX_JOUEURS; i++) {
        if (lobby[i].actif && lobby[i].en_attente) {
            if (p1 == -1) p1 = i;
            else { p2 = i; break; }
        }
    }

    if (p1 != -1 && p2 != -1) {
        
        static int next_port = 9000;
        int port = next_port++;
        
        printf("[LOBBY] Match trouvé: %s vs %s sur le port %d\n", lobby[p1].pseudo, lobby[p2].pseudo, port);
        
        
        paquet_t pkt;
        pkt.type = MSG_MATCH_FOUND;
        sprintf(pkt.data, "%d", port);
        envoyer_paquet_udp(&pkt, &lobby[p1].addr);
        envoyer_paquet_udp(&pkt, &lobby[p2].addr);
        
        
        lobby[p1].en_attente = 0;
        lobby[p2].en_attente = 0;

        serveur_ajouter_salon(port, lobby[p1].pseudo, lobby[p2].pseudo);

        
        
        if (fork() == 0) {
            creer_salon_tcp(port); 
            exit(0);              
        }
        
    }
}

void serveur_traiter_tournoi(void) {
    if (nb_tournoi >= 4) {
        printf("[TOURNOI] Lancement d'un tournoi à 4 joueurs !\n");
        
        for (int i = 0; i < 2; i++) {
            int idx1 = i * 2;
            int idx2 = i * 2 + 1;
            
            static int next_port = 10000;
            int port = next_port++;
            
            paquet_t pkt;
            pkt.type = MSG_MATCH_FOUND;
            sprintf(pkt.data, "%d", port);
            
            envoyer_paquet_udp(&pkt, &file_tournoi[idx1].addr);
            envoyer_paquet_udp(&pkt, &file_tournoi[idx2].addr);
            
            serveur_ajouter_salon(port, file_tournoi[idx1].pseudo, file_tournoi[idx2].pseudo);

            if (fork() == 0) {
                creer_salon_tcp(port);
                exit(0);
            }
        }
        
        
        for (int i = 4; i < nb_tournoi; i++) {
            file_tournoi[i-4] = file_tournoi[i];
        }
        nb_tournoi -= 4;
    }
}
