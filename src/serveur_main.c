#include "serveur.h"

DS_ALLOCATOR allocator = {0};


int running = 1;


void handle_sigint(int sig) {
    (void)sig; 
    running = 0; 
}

void envoyer_paquet_udp(paquet_t *pkt, struct sockaddr_in *dest) {
    sendto(udp_fd_serveur, pkt, sizeof(paquet_t), 0, (struct sockaddr *)dest, sizeof(struct sockaddr_in));
}

int trouver_joueur(const char *pseudo) {
    for (int i = 0; i < MAX_JOUEURS; i++) {
        if (lobby[i].actif && strcmp(lobby[i].pseudo, pseudo) == 0) return i;
    }
    return -1;
}

int main(void) {
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    paquet_t pkt;
    setbuf(stdout, NULL);

    
    signal(SIGINT, handle_sigint);
    
    signal(SIGCHLD, SIG_IGN);

    if ((udp_fd_serveur = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket fail");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(udp_fd_serveur, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#ifdef SO_REUSEPORT
    setsockopt(udp_fd_serveur, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT_SERVEUR);

    if (bind(udp_fd_serveur, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind fail");
        exit(EXIT_FAILURE);
    }

    printf("[SERVEUR] Lobby UDP démarré sur le port %d\n", PORT_SERVEUR);

    while (running) {
        fd_set readfds;
        
        struct timeval tv = {1, 0};
        FD_ZERO(&readfds);
        
        FD_SET(udp_fd_serveur, &readfds);

        
        int activity = select(udp_fd_serveur + 1, &readfds, NULL, NULL, &tv);

        if (activity > 0 && FD_ISSET(udp_fd_serveur, &readfds)) {
            len = sizeof(cliaddr);
            int n = recvfrom(udp_fd_serveur, &pkt, sizeof(paquet_t), 0, (struct sockaddr *)&cliaddr, &len);
            if (n <= 0) continue;

            printf("[LOBBY] Packet type %d de %s (%s:%d)\n", 
                   pkt.type, pkt.pseudo, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

            switch (pkt.type) {
                case MSG_JOIN: 
                case MSG_TOURNAMENT_JOIN:
                case MSG_LIST_ROOMS:
                case MSG_HISTORY: {
                    int idx = trouver_joueur(pkt.pseudo);
                    if (idx != -1) lobby[idx].addr = cliaddr;
                }
                default: break;
            }

            switch (pkt.type) {
                case MSG_JOIN: {
                    int idx = trouver_joueur(pkt.pseudo);
                    if (idx == -1) {
                        for (int i = 0; i < MAX_JOUEURS; i++) {
                            if (!lobby[i].actif) {
                                idx = i; break;
                            }
                        }
                    }
                    if (idx != -1) {
                        lobby[idx].actif = 1;
                        lobby[idx].en_attente = 1;
                        lobby[idx].dans_tournoi = 0; // Réinitialisation si retour au lobby "frais"
                        lobby[idx].addr = cliaddr;
                        strncpy(lobby[idx].pseudo, pkt.pseudo, MAX_PSEUDO - 1);
                        
                        paquet_t welcome;
                        welcome.type = MSG_WELCOME;
                        strcpy(welcome.data, "Connecté au lobby !");
                        envoyer_paquet_udp(&welcome, &cliaddr);
                        
                        
                        serveur_traiter_matchmaking();
                    }
                    break;
                }
                case MSG_TOURNAMENT_JOIN: {
                    if (nb_tournoi < MAX_JOUEURS) {
                        int l_idx = trouver_joueur(pkt.pseudo);
                        if (l_idx != -1) lobby[l_idx].dans_tournoi = 1;

                        file_tournoi[nb_tournoi].addr = cliaddr;
                        strncpy(file_tournoi[nb_tournoi].pseudo, pkt.pseudo, MAX_PSEUDO - 1);
                        file_tournoi[nb_tournoi].actif = 1;
                        nb_tournoi++;
                        
                        paquet_t welcome;
                        welcome.type = MSG_WELCOME;
                        sprintf(welcome.data, "Inscrit au tournoi (%d/4)", nb_tournoi);
                        envoyer_paquet_udp(&welcome, &cliaddr);
                        
                        serveur_traiter_tournoi();
                    }
                    break;
                }
                case MSG_LIST_ROOMS: {
                    paquet_t list;
                    list.type = MSG_ROOM_LIST;
                    list.data[0] = '\0';
                    for (int i = 0; i < nb_salons; i++) {
                        char chunk[128];
                        sprintf(chunk, "%d:%s vs %s|", salons[i].id, salons[i].joueur1, salons[i].joueur2);
                        strncat(list.data, chunk, sizeof(list.data) - strlen(list.data) - 1);
                    }
                    envoyer_paquet_udp(&list, &cliaddr);
                    break;
                }
                case MSG_RESULT_REPORT: {
                    
                    printf("[SERVER] Rapport de résultat : %s\n", pkt.data);
                    
                    int port_r;
                    char p1_r[32], p2_r[32], winner_r[32];
                    if (sscanf(pkt.data, "%d|%s vs %s : %s", &port_r, p1_r, p2_r, winner_r) == 4) {
                        
                        serveur_retirer_salon(port_r);

                        
                        if (port_r >= 10000 && strcmp(winner_r, "NULLE") != 0) {
                            int v_idx = trouver_joueur(winner_r);
                            if (v_idx != -1 && nb_vainqueurs < MAX_JOUEURS) {
                                vainqueurs[nb_vainqueurs] = lobby[v_idx];
                                nb_vainqueurs++;
                                serveur_traiter_vainqueurs();
                            }
                        } else if (port_r >= 10000 && strcmp(winner_r, "NULLE") == 0) {
                            
                            int v_idx = trouver_joueur(p1_r);
                            if (v_idx != -1 && nb_vainqueurs < MAX_JOUEURS) {
                                vainqueurs[nb_vainqueurs] = lobby[v_idx];
                                nb_vainqueurs++;
                                serveur_traiter_vainqueurs();
                            }
                        }

                        
                        if (port_r >= 10000) {
                            int l_p1 = trouver_joueur(p1_r);
                            int l_p2 = trouver_joueur(p2_r);
                            if (l_p1 != -1 && strcmp(p1_r, winner_r) != 0 && strcmp(winner_r, "NULLE") != 0) lobby[l_p1].dans_tournoi = 0;
                            if (l_p2 != -1 && strcmp(p2_r, winner_r) != 0 && strcmp(winner_r, "NULLE") != 0) lobby[l_p2].dans_tournoi = 0;
                        }
                    }

                    FILE *f = fopen("data/history.txt", "a");
                    if (f) {
                        fprintf(f, "%s\n", pkt.data);
                        fclose(f);
                    }
                    break;
                }
                case MSG_HISTORY: {
                    paquet_t hist;
                    hist.type = MSG_HISTORY;
                    hist.data[0] = '\0';
                    FILE *f = fopen("data/history.txt", "r");
                    if (f) {
                        char line[128];
                        while(fgets(line, sizeof(line), f)) {
                            strncat(hist.data, line, sizeof(hist.data) - strlen(hist.data) - 2);
                        }
                        fclose(f);
                    } else {
                        strcpy(hist.data, "Aucun historique disponible.");
                    }
                    envoyer_paquet_udp(&hist, &cliaddr);
                    break;
                }
                default: break;
            }
        }
    }

    close(udp_fd_serveur);
    printf("[SERVEUR] Fermeture.\n");
    return 0;
}
