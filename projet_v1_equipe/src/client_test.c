#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "reseau.h" // Indispensable pour avoir la structure paquet_t

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in serv_addr;
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    paquet_t p;
    memset(&p, 0, sizeof(paquet_t)); // On vide la mémoire pour être propre
    p.type = MSG_JOIN;
    strcpy(p.pseudo, "Joueur_Test");

    // On envoie la structure ENTIÈRE
    sendto(sock, &p, sizeof(paquet_t), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    
    printf("Paquet de type MSG_JOIN envoyé au serveur !\n");
    return 0;
}