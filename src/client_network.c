#include "client.h"


int udp_fd = -1;
int tcp_fd = -1;
struct sockaddr_in serveur_addr;
char pseudo[32] = "";
char info_msg[256] = "Bienvenue ! Appuyez sur JOUER pour commencer.";
client_state_t etat = STATE_CHOOSING_MODE;

void reseau_connecter_lobby(const char *ip_serveur) {
    if (udp_fd != -1) return;

    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&serveur_addr, 0, sizeof(serveur_addr));
    serveur_addr.sin_family = AF_INET;
    serveur_addr.sin_port = htons(PORT_SERVEUR);
    
    if (ip_serveur && strlen(ip_serveur) > 0) {
        serveur_addr.sin_addr.s_addr = inet_addr(ip_serveur);
    } else {
        serveur_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    }
}

void reseau_joindre_jeu(void) {
    if (udp_fd == -1) reseau_connecter_lobby(NULL);
    paquet_t pkt;
    pkt.type = MSG_JOIN;
    strcpy(pkt.pseudo, pseudo);
    sendto(udp_fd, &pkt, sizeof(paquet_t), 0, (struct sockaddr *)&serveur_addr, sizeof(serveur_addr));
    printf("[LOBBY] Demande de jeu envoyée.\n");
    etat = STATE_LOBBY;
}

void reseau_rejoindre_tournoi(void) {
    if (udp_fd == -1) reseau_connecter_lobby(NULL);
    paquet_t pkt;
    pkt.type = MSG_TOURNAMENT_JOIN;
    strcpy(pkt.pseudo, pseudo);
    sendto(udp_fd, &pkt, sizeof(paquet_t), 0, (struct sockaddr *)&serveur_addr, sizeof(serveur_addr));
    printf("[TOURNOI] Demande d'inscription envoyée.\n");
    etat = STATE_LOBBY;
}

void reseau_traiter_udp(void) {
    fd_set set;
    
    
    struct timeval tv = {0, 0};
    FD_ZERO(&set);
    FD_SET(udp_fd, &set);

    if (select(udp_fd + 1, &set, NULL, NULL, &tv) > 0) {
        paquet_t reponse;
        struct sockaddr_in from;
        socklen_t from_len = sizeof(from);
        int n = recvfrom(udp_fd, &reponse, sizeof(paquet_t), 0, (struct sockaddr *)&from, &from_len);
        
        if (n > 0) {
            if (reponse.type == MSG_WELCOME) {
                snprintf(info_msg, sizeof(info_msg), "%s", reponse.data);
            } else if (reponse.type == MSG_MATCH_FOUND) {
                
                int port_salon = atoi(reponse.data);
                printf("MATCH TROUVÉ sur le port %d ! Connexion TCP...\n", port_salon);
                
                
                
                tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
                struct sockaddr_in salon_addr = serveur_addr;
                salon_addr.sin_port = htons(port_salon);
                
                if (connect(tcp_fd, (struct sockaddr *)&salon_addr, sizeof(salon_addr)) == 0) {
                    etat = STATE_CONNECTING;
                    snprintf(info_msg, sizeof(info_msg), "Connexion au salon %d...", port_salon);
                    
                    
                    paquet_t join_pkt;
                    join_pkt.type = MSG_JOIN;
                    strcpy(join_pkt.pseudo, pseudo);
                    send(tcp_fd, &join_pkt, sizeof(paquet_t), 0);
                }
            } else if (reponse.type == MSG_ROOM_LIST) {
                
                nb_salons_disponibles = 0;
                char *token = strtok(reponse.data, "|");
                while (token && nb_salons_disponibles < MAX_SALONS) {
                    sscanf(token, "%d:%[^|]", &salons_disponibles[nb_salons_disponibles].port, 
                                            salons_disponibles[nb_salons_disponibles].joueurs);
                    nb_salons_disponibles++;
                    token = strtok(NULL, "|");
                }
            } else if (reponse.type == MSG_HISTORY) {
                strncpy(historique_txt, reponse.data, sizeof(historique_txt) - 1);
            }
        }
    }
}

void reseau_traiter_tcp(void) {
    fd_set set;
    struct timeval tv = {0, 0};
    FD_ZERO(&set);
    FD_SET(tcp_fd, &set);

    if (select(tcp_fd + 1, &set, NULL, NULL, &tv) > 0) {
        paquet_t pkt;
        int n = recv(tcp_fd, &pkt, sizeof(paquet_t), 0);
        if (n <= 0) {
            etat = STATE_GAME_OVER;
            snprintf(info_msg, sizeof(info_msg), "Déconnecté du salon.");
            return;
        }

        switch ((int)pkt.type) {
            case MSG_WELCOME: {
                if (strcmp(pkt.data, "0") == 0) ma_couleur = CHESS_WHITE;
                else if (strcmp(pkt.data, "1") == 0) ma_couleur = CHESS_BLACK;
                else ma_couleur = 2; 
                
                etat = (ma_couleur == 2) ? STATE_SPECTATING : STATE_PLAYING;
                snprintf(info_msg, sizeof(info_msg), "Partie lancée !");
                
                
                chess_init_fen(&partie, DS_STRING_SLICE(CHESS_START));
                ds_dynamic_array_init(&coups_legaux, sizeof(move_t));
                ds_dynamic_array_init(&coups_possibles, sizeof(move_t)); 
                chess_generate_moves(&partie, &coups_legaux); 
                break;
            }
            case MSG_MOVE: {
                int sr, sf, er, ef, m, p;
                sscanf(pkt.data, "%d,%d,%d,%d,%d,%d", &sr, &sf, &er, &ef, &m, &p);
                move_t coup = MK_MOVE(MK_SQUARE(sr, sf), MK_SQUARE(er, ef), (char)m, (char)p);
                chess_apply_move(&partie, coup);
                partie.current_player = chess_flip_player(partie.current_player);
                jouer_son_coup(coup);

                
                ds_dynamic_array_clear(&coups_legaux);
                chess_generate_moves(&partie, &coups_legaux);

                
                if (coups_legaux.count == 0) {
                    etat = STATE_GAME_OVER;
                    if (chess_is_in_check(&partie, partie.current_player)) {
                        snprintf(info_msg, sizeof(info_msg), "ÉCHEC ET MAT ! La partie est finie.");
                    } else {
                        snprintf(info_msg, sizeof(info_msg), "PAT ! Match nul.");
                    }
                }
                break;
            }
            case MSG_CHAT: {
                char final_msg[300];
                snprintf(final_msg, sizeof(final_msg), "[%s]: %s", pkt.pseudo, pkt.data);
                if (chat_count < 20) {
                    strcpy(chat_messages[chat_count++], final_msg);
                } else {
                    for (int i = 0; i < 19; i++) strcpy(chat_messages[i], chat_messages[i+1]);
                    strcpy(chat_messages[19], final_msg);
                }
                break;
            }
            case MSG_BOARD_STATE: {
                
                chess_init_fen(&partie, DS_STRING_SLICE(pkt.data));
                ds_dynamic_array_clear(&coups_legaux); 
                chess_generate_moves(&partie, &coups_legaux);
                break;
            }
            case MSG_GAME_OVER: {
                etat = STATE_GAME_OVER;
                strncpy(info_msg, pkt.data, sizeof(info_msg) - 1);
                break;
            }
            default: break;
        }
    }
}
