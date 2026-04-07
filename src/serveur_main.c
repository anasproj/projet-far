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
