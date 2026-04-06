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
