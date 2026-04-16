#include "salon.h"

void creer_salon_tcp(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket fail");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setsockopt fail");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind fail");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen fail");
        exit(EXIT_FAILURE);
    }

    printf("[SALON] Match lancé sur le port %d\n", port);
    port_salon = port;

    salon_init_jeu();
    nb_clients = 0;
    
    
    for (int i = 0; i < MAX_PLAYERS; i++) clients[i].actif = 0;

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (clients[i].actif) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > max_sd) max_sd = clients[i].fd;
            }
        }

        
        
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            perror("Select error");
        }

        
        if (FD_ISSET(server_fd, &readfds)) {
            int new_socket;
            struct sockaddr_in new_addr;
            socklen_t addrlen = sizeof(new_addr);
            if ((new_socket = accept(server_fd, (struct sockaddr *)&new_addr, &addrlen)) < 0) {
                perror("Accept error");
                continue;
            }

            printf("[SALON %d] Nouveau client connecté: %s:%d\n", port, inet_ntoa(new_addr.sin_addr), ntohs(new_addr.sin_port));

            
            int slot = -1;
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (!clients[i].actif) {
                    slot = i;
                    break;
                }
            }

            if (slot != -1) {
                clients[slot].fd = new_socket;
                clients[slot].addr = new_addr;
                clients[slot].actif = 1;
                
                
                
                if (slot == 0)      clients[slot].role = ROLE_WHITE;
                else if (slot == 1) clients[slot].role = ROLE_BLACK;
                else                clients[slot].role = ROLE_SPECTATOR;
                
                nb_clients++;
            } else {
                close(new_socket);
            }
        }

        
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (clients[i].actif && FD_ISSET(clients[i].fd, &readfds)) {
                paquet_t pkt;
                int n = recv(clients[i].fd, &pkt, sizeof(paquet_t), 0);
                if (n <= 0) {
                    
                    if (clients[i].role == ROLE_WHITE || clients[i].role == ROLE_BLACK) {
                        int winner_idx = (i == 0) ? 1 : 0;
                        if (clients[winner_idx].actif && clients[winner_idx].role != ROLE_SPECTATOR) {
                            printf("[SALON %d] Victoire par forfait de %s\n", port, clients[winner_idx].pseudo);
                            
                            paquet_t report;
                            report.type = MSG_RESULT_REPORT;
                            sprintf(report.data, "%d|%s vs %s : %s", port, 
                                    (i == 0) ? clients[i].pseudo : clients[winner_idx].pseudo,
                                    (i == 0) ? clients[winner_idx].pseudo : clients[i].pseudo,
                                    clients[winner_idx].pseudo);
                            
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
                    }

                    printf("[SALON %d] Client déconnecté (slot %d)\n", port, i);
                    close(clients[i].fd);
                    clients[i].actif = 0;
                    nb_clients--;

                    if (nb_clients == 0) {
                        printf("[SALON %d] Fin de partie, fermeture.\n", port);
                        exit(0);
                    }
                } else {
                    
                    if (pkt.type == MSG_SPECTATE) {
                        clients[i].role = ROLE_SPECTATOR;
                        strncpy(clients[i].pseudo, pkt.pseudo, MAX_PSEUDO - 1);
                        salon_envoyer_etat_complet(i);
                    } else if (pkt.type == MSG_JOIN || pkt.type == MSG_MOVE || pkt.type == MSG_CHAT) {
                        if (strlen(clients[i].pseudo) == 0) strncpy(clients[i].pseudo, pkt.pseudo, MAX_PSEUDO - 1);
                        
                        
                        if (pkt.type == MSG_JOIN) {
                            paquet_t welcome;
                            welcome.type = MSG_WELCOME;
                            sprintf(welcome.data, "%d", clients[i].role);
                            send(clients[i].fd, &welcome, sizeof(paquet_t), 0);
                        } else {
                            salon_traiter_paquet(i, &pkt);
                        }
                    }
                }
            }
        }
    }
}
