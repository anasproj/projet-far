#include "salon.h"

#ifndef PORT_SERVEUR
#define PORT_SERVEUR 8080
#endif


client_t clients[MAX_PLAYERS] = {0};
int nb_clients = 0;
chess_state_t partie_salon;
int port_salon = 0;

void salon_init_jeu(void) {
    chess_init_fen(&partie_salon, DS_STRING_SLICE(CHESS_START));
}

void salon_diffuser(paquet_t *pkt, int except_fd) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (clients[i].actif && clients[i].fd != except_fd) {
            send(clients[i].fd, pkt, sizeof(paquet_t), 0);
        }
    }
}



void salon_envoyer_etat_complet(int client_idx) {
    paquet_t pkt;
    pkt.type = MSG_BOARD_STATE;
    strcpy(pkt.pseudo, "SERVEUR");
    
    char *fen = NULL;
    chess_dump_fen(&partie_salon, &fen);
    if (fen) {
        strncpy(pkt.data, fen, sizeof(pkt.data) - 1);
        send(clients[client_idx].fd, &pkt, sizeof(paquet_t), 0);
        free(fen);
    }
}

void salon_traiter_paquet(int client_idx, paquet_t *pkt) {
    (void)client_idx;
    int fd = clients[client_idx].fd;

    switch (pkt->type) {
        case MSG_MOVE: {
            printf("[SALON %d] Move de %s: %s\n", port_salon, pkt->pseudo, pkt->data);
            
            
            int attendu = (partie_salon.current_player == CHESS_WHITE) ? ROLE_WHITE : ROLE_BLACK;
            if ((int)clients[client_idx].role != attendu) {
                printf("[SALON %d] Erreur: Ce n'est pas le tour de %s\n", port_salon, pkt->pseudo);
                return;
            }

            
            int sr, sf, er, ef, m, p;
            sscanf(pkt->data, "%d,%d,%d,%d,%d,%d", &sr, &sf, &er, &ef, &m, &p);
            move_t coup = MK_MOVE(MK_SQUARE(sr, sf), MK_SQUARE(er, ef), (char)m, (char)p);
            chess_apply_move(&partie_salon, coup);
            partie_salon.current_player = chess_flip_player(partie_salon.current_player);

            
            salon_diffuser(pkt, fd);

            
            char winner_pseudo[32] = "";
            int over = 0;
            if (chess_is_checkmate(&partie_salon, partie_salon.current_player)) {
                over = 1;
                
                strcpy(winner_pseudo, pkt->pseudo);
                printf("[SALON %d] ÉCHEC ET MAT ! Gagnant: %s\n", port_salon, winner_pseudo);
            } else if (chess_is_draw(&partie_salon, partie_salon.current_player) || 
                       chess_is_stalemate(&partie_salon, partie_salon.current_player)) {
                over = 1;
                strcpy(winner_pseudo, "NULLE");
                printf("[SALON %d] MATCH NUL (Pat ou matériel insuffisant)\n", port_salon);
            }

            if (over) {
                paquet_t over_pkt;
                over_pkt.type = MSG_GAME_OVER;
                strcpy(over_pkt.pseudo, "SERVEUR");
                sprintf(over_pkt.data, "Fin de partie: %s", (strcmp(winner_pseudo, "NULLE") == 0) ? "Match Nul" : winner_pseudo);
                salon_diffuser(&over_pkt, -1);

                
                paquet_t report;
                report.type = MSG_RESULT_REPORT;
                char p1[32] = "Inconnu", p2[32] = "Inconnu";
                for(int i=0; i<MAX_PLAYERS; i++) {
                    if (clients[i].actif) {
                        if (clients[i].role == ROLE_WHITE) strcpy(p1, clients[i].pseudo);
                        if (clients[i].role == ROLE_BLACK) strcpy(p2, clients[i].pseudo);
                    }
                }
                sprintf(report.data, "%s vs %s : %s", p1, p2, winner_pseudo);
                
                struct sockaddr_in serv;
                memset(&serv, 0, sizeof(serv));
                serv.sin_family = AF_INET;
                serv.sin_addr.s_addr = inet_addr("127.0.0.1");
                serv.sin_port = htons(PORT_SERVEUR);
                int tmp_fd = socket(AF_INET, SOCK_DGRAM, 0);
                if (tmp_fd >= 0) {
                    sendto(tmp_fd, &report, sizeof(paquet_t), 0, (struct sockaddr *)&serv, sizeof(serv));
                    close(tmp_fd);
                }
            }
            break;
        }
        case MSG_CHAT: {
            printf("[SALON %d] Chat de %s: %s\n", port_salon, pkt->pseudo, pkt->data);
            salon_diffuser(pkt, -1); 
            break;
        }
        default:
            break;
    }
}
