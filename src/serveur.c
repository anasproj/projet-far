#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h> // Requis pour gérer les signaux Unix
#include "reseau.h" // Inclusion de votre protocole commun

#define PORT 8080 // Définit le "numéro de porte" sur lequel ton serveur va écouter. Le port 8080 est un standard pour les tests.
#define MAX_JOUEURS 100 // Fixe le nombre maximum de joueurs pouvant rejoindre le lobby.

// Variable globale pour contrôler la boucle d'exécution.
// Le mot-clé 'volatile' prévient le compilateur que cette variable peut 
// être modifiée de manière asynchrone (par le gestionnaire de signaux).
volatile int serveur_actif = 1;

// Fonction appelée automatiquement par l'OS lors d'un Ctrl+C (SIGINT) Permet de stopper le serveur proprement
void gerer_signal(int sig) {
    printf("\n[Signal %d intercepté] - Arrêt du serveur demandé...\n", sig);
    serveur_actif = 0; // On casse la condition de la boucle
}

// Structure pour mémoriser qui est dans le lobby
typedef struct {
    struct sockaddr_in adresse;
    char pseudo[32];
} joueur_t;

joueur_t lobby[MAX_JOUEURS];
int nb_joueurs = 0;




// --- FONCTION DU PROCESSUS FILS (LE SALON TCP) ---
// --- FONCTION DU PROCESSUS FILS (LE SALON TCP) ---
void creer_salon_tcp(int port_salon) {
    printf("[SALON TCP] Ouverture du salon sur le port %d...\n", port_salon);
    
    int server_tcp_fd;
    struct sockaddr_in address;
    
    // 1. Création du socket TCP (SOCK_STREAM)
    if ((server_tcp_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("[SALON TCP] Erreur socket TCP");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    // Permet de réutiliser le port 8081 immédiatement après un arrêt
    setsockopt(server_tcp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 2. Configuration de l'adresse
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_salon);
    
    // 3. Attachement au port (Bind)
    if (bind(server_tcp_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[SALON TCP] Erreur bind TCP");
        exit(EXIT_FAILURE);
    }
    
    // 4. Écoute (Listen) - On attend exactement 2 joueurs
    if (listen(server_tcp_fd, 2) < 0) {
        perror("[SALON TCP] Erreur listen");
        exit(EXIT_FAILURE);
    }
    
    printf("[SALON TCP] En attente de la connexion des 2 joueurs en TCP...\n");
    
    int joueur1_fd, joueur2_fd;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // 5. On attend le premier joueur (Blanc)
    printf("[SALON %d] Attente du Joueur 1 (Blancs)...\n", port_salon);
    joueur1_fd = accept(server_tcp_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (joueur1_fd < 0) { perror("Erreur accept J1"); exit(EXIT_FAILURE); }
    printf("[SALON %d] Joueur 1 connecté !\n", port_salon);

    // 6. On attend le second joueur (Noir)
    printf("[SALON %d] Attente du Joueur 2 (Noirs)...\n", port_salon);
    joueur2_fd = accept(server_tcp_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (joueur2_fd < 0) { perror("Erreur accept J2"); exit(EXIT_FAILURE); }
    printf("[SALON %d] Joueur 2 connecté !\n", port_salon);

    printf("[SALON %d] Les deux joueurs sont là. La partie commence !\n", port_salon);

    // 7. On annonce les couleurs aux joueurs (Envoi en TCP)
    paquet_t start_msg;
    memset(&start_msg, 0, sizeof(paquet_t)); // On nettoie la mémoire du paquet
    start_msg.type = MSG_WELCOME; 
    strcpy(start_msg.pseudo, "ARBITRE");

    strcpy(start_msg.data, "Blancs"); // Le client saura qu'il joue en premier
    send(joueur1_fd, &start_msg, sizeof(paquet_t), 0);

    strcpy(start_msg.data, "Noirs");
    send(joueur2_fd, &start_msg, sizeof(paquet_t), 0);

    // Boucle temporaire pour garder le salon ouvert 
    while(1) { sleep(1); } 
}


int main() {
    int server_fd; // C'est un entier qui servira d'identifiant (une sorte de ticket) pour le socket réseau.
    struct sockaddr_in server_addr, client_addr; // Ces structures stockent les adresses IP et les ports. server_addr pour ton serveur, et client_addr pour mémoriser l'adresse de la personne qui t'envoie un message.
    
    paquet_t paquet_recu; // On utilise la structure réseau (paquet_t) au lieu du tab de caractères brut pour réceptionner les messages structurés.
    
    socklen_t client_len = sizeof(client_addr); // Stocke la taille en mémoire de l'adresse du client (requis par les fonctions réseau).

    // 1. Association du signal SIGINT (Ctrl+C) à notre fonction
    struct sigaction sa;
    sa.sa_handler = gerer_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // Le secret est ici : 0 signifie "Ne pas redémarrer l'appel système"

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Erreur lors du placement du signal");
        exit(EXIT_FAILURE);
    }
    // GESTION DES ZOMBIES : Le système nettoiera automatiquement les processus fils terminés
    signal(SIGCHLD, SIG_IGN);

    // 2. Création du socket UDP
    // Si le resultat < 0, -> erreur
    // socket(...) : Crée le point d'accès réseau.
    // AF_INET signifie qu'on utilise des adresses IP version 4 (IPv4).
    // SOCK_DGRAM signifie qu'on utilise le protocole UDP (datagrammes).
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erreur lors de la création du socket UDP");
        exit(EXIT_FAILURE);
    }

    // memset met toute la mémoire de ces structures à zéro pour éviter 
    // d'avoir des "déchets" (valeurs résiduelles aléatoires) qui 
    // feraient bugger la configuration.
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    // C'est dans le cours (Far-Res-Socket)
    // sin_family : On confirme qu'on est en IPv4.
    // INADDR_ANY : Dit au serveur d'écouter sur toutes les cartes réseau de la machine (le Wi-Fi, le câble Ethernet, et le réseau local interne 127.0.0.1).
    // htons(PORT) : Les ordinateurs et les réseaux ne lisent pas les nombres dans le même sens (problème d'Endianness). Cette fonction traduit le port 8080 dans le langage universel du réseau.
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 3. Attachement du socket (bind)
    // bind (lier) : C'est ici que l'OS associe officiellement le programme au port 8080. Si un autre programme utilise déjà le port 8080, le bind échouera (renvoie < 0).
    if (bind(server_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erreur de bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Serveur global démarré sur le port %d.\n", PORT);
    printf("Appuyez sur Ctrl+C pour l'arrêter proprement.\n\n");

    // 4. Boucle contrôlée par la variable d'état
    // La boucle tourne tant qu'on n'a pas fait de Ctrl+C.
    while (serveur_actif) {
        
        // recvfrom met le processus en pause jusqu'à l'arrivée d'un paquet.
        // Quand un paquet arrive, l'OS remplit la structure paquet_recu, et remplit la structure client_addr avec l'IP et le port du joueur qui vient de l'envoyer.
        // n contient le nombre d'octets reçus.
        int n = recvfrom(server_fd, &paquet_recu, sizeof(paquet_t), 0,
                         (struct sockaddr *)&client_addr, &client_len);
        printf("[DEBUG] Octets reçus : %d\n", n); // Tests

        // Gestion de l'interruption par le signal
        if (n < 0) {
            if (!serveur_actif) {
                break; // Sortie propre de la boucle si serveur_actif est passé à 0
            }
            perror("Erreur lors de la réception");
            continue;
        }

        // (Plus besoin d'ajouter manuellement le '\0' car on reçoit une structure de données propre)

        // Le serveur analyse le type de requête reçue
        if (paquet_recu.type == MSG_JOIN) {
            printf(">>> [NOUVEAU] Le joueur '%s' demande à se connecter.\n", paquet_recu.pseudo);
            
            // inet_ntoa : Traduit l'adresse IP réseau brute en une chaîne lisible (ex: "192.168.1.15").
            // ntohs : "Network TO Host Short". Reconvertit le port du client en entier classique lisible.
            printf("Client IP: %s | Port: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            if (nb_joueurs < MAX_JOUEURS) {
                // Enregistrement du joueur
                lobby[nb_joueurs].adresse = client_addr;
                strcpy(lobby[nb_joueurs].pseudo, paquet_recu.pseudo);
                nb_joueurs++;

                // Création d'un paquet de réponse
                paquet_t reponse;
                reponse.type = MSG_WELCOME;
                strcpy(reponse.pseudo, "SERVEUR");
                strcpy(reponse.data, "Bienvenue dans le lobby du tournoi !");

                // 5. Réponse au client
                // sendto : On prend le message reponse et on le "jette" sur le réseau en visant l'adresse client_addr (qu'on avait récupérée grâce au recvfrom).
                sendto(server_fd, &reponse, sizeof(paquet_t), 0,
                       (const struct sockaddr *)&client_addr, client_len);
                
                printf("Joueur accepté. Nombre total dans le lobby : %d\n\n", nb_joueurs);


                // ==========================================
                // SPRINT 3 : MATCHMAKING ET FORK
                // ==========================================
                if (nb_joueurs == 2) {
                    printf("\n=== MATCHMAKING : 2 joueurs trouvés ! ===\n");
                    
                    // On invente un numéro de port pour ce salon (ex: 8081)
                    // (Dans une version avancée, tu pourras incrémenter ce chiffre)
                    int port_salon = 8081; 

                    // 1. On prévient les deux joueurs de basculer en TCP
                    paquet_t paquet_match;
                    paquet_match.type = MSG_MATCH_FOUND;
                    strcpy(paquet_match.pseudo, "SERVEUR");
                    // On envoie le numéro du port sous forme de texte dans 'data'
                    sprintf(paquet_match.data, "%d", port_salon); 

                    sendto(server_fd, &paquet_match, sizeof(paquet_t), 0,
                           (struct sockaddr *)&lobby[0].adresse, sizeof(lobby[0].adresse));
                    sendto(server_fd, &paquet_match, sizeof(paquet_t), 0,
                           (struct sockaddr *)&lobby[1].adresse, sizeof(lobby[1].adresse));

                    printf("Ordre de migration envoyé aux 2 joueurs vers le port TCP %d.\n", port_salon);

                    // 2. On clone le serveur avec fork()
                    pid_t pid = fork();

                    if (pid == 0) {
                        // --- JE SUIS LE FILS (LE SALON) ---
                        close(server_fd); // Je n'ai plus besoin du port UDP 8080
                        creer_salon_tcp(port_salon); // Je lance mon salon TCP !
                        exit(0); // Quand le salon fermera, le processus fils mourra proprement
                    } 
                    else if (pid > 0) {
                        // --- JE SUIS LE PÈRE (LE LOBBY UDP) ---
                        printf("Processus Salon créé avec le PID %d.\n", pid);
                        nb_joueurs = 0; // Je vide le lobby pour accueillir les prochains joueurs !
                    } 
                    else {
                        perror("Erreur lors du fork()");
                    }
                }
                // ==========================================
            }
        }
    }

    // 6. Nettoyage et fermeture propre
    // Quand on est sorti de la boucle (après un Ctrl+C), on arrive ici. close rend le port 8080 à l'OS pour que d'autres applications puissent l'utiliser. C'est l'intérêt principal d'avoir fait un arrêt propre !
    printf("Fermeture du socket réseau...\n");
    close(server_fd);
    printf("Ressources libérées. Serveur arrêté avec succès.\n");

    return 0;
}