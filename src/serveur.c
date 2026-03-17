#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h> // Requis pour gérer les signaux Unix

#define PORT 8080 //Définit le "numéro de porte" sur lequel ton serveur va écouter. Le port 8080 est un standard pour les tests.
#define BUFFER_SIZE 1024 //Fixe la taille maximale (en octets) des messages que le serveur peut recevoir en une seule fois.

// Variable globale pour contrôler la boucle d'exécution.
// Le mot-clé 'volatile' prévient le compilateur que cette variable peut 
// être modifiée de manière asynchrone (par le gestionnaire de signaux).
volatile int serveur_actif = 1;

// Fonction appelée automatiquement par l'OS lors d'un Ctrl+C (SIGINT) Permet de stopper le serveur
void gerer_signal(int sig) {
    printf("\n[Signal %d intercepté] - Arrêt du serveur demandé...\n", sig);
    serveur_actif = 0; // On casse la condition de la boucle
}

int main() {
    int server_fd; //C'est un entier qui servira d'identifiant (une sorte de ticket) pour le socket réseau.
    struct sockaddr_in server_addr, client_addr;//Ces structures stockent les adresses IP et les ports. server_addr pour ton serveur, et client_addr pour mémoriser l'adresse de la personne qui t'envoie un message.
    char buffer[BUFFER_SIZE]; //tab de characteres qui va receptionner le message
    socklen_t client_len = sizeof(client_addr); //Stocke la taille en mémoire de l'adresse du client (requis par les fonctions réseau).

    // 1. Association du signal SIGINT (Ctrl+C) à notre fonction
    // signal(SIGINT, gerer_signal); //On dit à l'OS si tu reçcois Ctrl+C,execute gerer_signal()

    struct sigaction sa;
        sa.sa_handler = gerer_signal;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0; // Le secret est ici : 0 signifie "Ne pas redémarrer l'appel système"

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Erreur lors du placement du signal");
        exit(EXIT_FAILURE);
}

    // 2. Création du socket UDP

    //Si le resultat < 0, -> erreur
    //socket(...) : Crée le point d'accès réseau.
    //AF_INET signifie qu'on utilise des adresses IP version 4 (IPv4).
    //SOCK_DGRAM signifie qu'on utilise le protocole UDP (datagrammes).

    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erreur lors de la création du socket UDP");
        exit(EXIT_FAILURE);
    }
    
    //memset met toute la mémoire de ces structures à zéro pour éviter 
    //d'avoir des "déchets" (valeurs résiduelles aléatoires) qui 
    //feraient bugger la configuration.
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    //C'est dans le cours ( Far-Res-Socket)
    //sin_family : On confirme qu'on est en IPv4.
    //INADDR_ANY : Dit au serveur d'écouter sur toutes les cartes réseau de la machine (le Wi-Fi, le câble Ethernet, et le réseau local interne 127.0.0.1).
    //htons(PORT) :Les ordinateurs et les réseaux ne lisent pas les nombres dans le même sens (problème d'Endianness). Cette fonction traduit le port 8080 dans le langage universel du réseau.
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 3. Attachement du socket (bind)
    //bind (lier) : C'est ici que l'OS associe officiellement le programme au port 8080. Si un autre programme utilise déjà le port 8080, le bind échouera (renvoie < 0).
    if (bind(server_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erreur de bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Serveur global (Lobby UDP) démarré sur le port %d.\n", PORT);
    printf("Appuyez sur Ctrl+C pour l'arrêter proprement.\n\n");

    // 4. Boucle contrôlée par la variable d'état
    //La boucle tourne tant qu'on n'a pas fait de Ctrl+C.
    while (serveur_actif) {
        
        // recvfrom met le processus en pause jusqu'à l'arrivée d'un paquet.
        // Quand un paquet arrive, l'OS remplit ton buffer avec le texte, et remplit la structure client_addr avec l'IP et le port du joueur qui vient de l'envoyer.
        // n contient le nombre de lettres (octets) reçues.
        int n = recvfrom(server_fd, (char *)buffer, BUFFER_SIZE - 1,
                         0, (struct sockaddr *)&client_addr,
                         &client_len);
        
        // Gestion de l'interruption par le signal
        if (n < 0) {
            if (!serveur_actif) {
                break; // Sortie propre de la boucle si serveur_actif est passé à 0
            }
            perror("Erreur lors de la réception");
            continue;
        }
        //n a reçu des données brutes du réseau, on ajoute manuellement ce caractère
        buffer[n] = '\0'; 
        
        printf("--- Nouveau paquet reçu ---\n");
        //inet_ntoa : Traduit l'adresse IP réseau brute en une chaîne lisible (ex: "192.168.1.15").
        //ntohs : "Network TO Host Short", l'inverse de tout à l'heure. Reconvertit le port du client en entier classique lisible.
        printf("Client IP: %s | Port: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        printf("Message: %s\n", buffer);

        // 5. Réponse au client
        char *reponse = "Bienvenue sur le lobby du tournoi ! Message bien reçu.";
        //sendto : On prend le message reponse et on le "jette" sur le réseau en visant l'adresse client_addr (qu'on avait récupérée grâce au recvfrom).
        sendto(server_fd, (const char *)reponse, strlen(reponse),
               0, (const struct sockaddr *)&client_addr,
               client_len);
    }

    // 6. Nettoyage et fermeture propre
    //Quand on est sorti de la boucle (après un Ctrl+C), on arrive ici. close rend le port 8080 à l'OS pour que d'autres applications puissent l'utiliser. C'est l'intérêt principal d'avoir fait un arrêt propre !
    printf("Fermeture du socket réseau...\n");
    close(server_fd);
    printf("Ressources libérées. Serveur arrêté avec succès.\n");

    return 0;
}