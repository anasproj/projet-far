#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "tournoi.h"

static Tournoi tournoi;
static char error_msg[128];

static void set_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(error_msg, sizeof(error_msg), fmt, ap);
    va_end(ap);
}

const char *tournoi_last_error(void) {
    return error_msg;
}

static void notify_client(int client_sock, const char *payload) {
    if (client_sock >= 0) {
        printf("[tournoi] client %d: %s\n", client_sock, payload);
    } else {
        printf("[tournoi] %s\n", payload);
    }
}

void init_tournoi(void) {
    tournoi.is_active = 0;
    tournoi.status = TOURNOI_INSCRIPTIONS;
    tournoi.max_players = 0;
    tournoi.nb_registered = 0;
    tournoi.current_round = 0;
    pthread_mutex_init(&tournoi.lock, NULL);
    error_msg[0] = '\0';
}

int create_tournoi(int max_players) {
    pthread_mutex_lock(&tournoi.lock);

    if (tournoi.is_active) {
        set_error("Un tournoi est deja en cours.");
        pthread_mutex_unlock(&tournoi.lock);
        return -1;
    }

    if (max_players != 2 && max_players != 4 && max_players != 8) {
        set_error("Le nombre de joueurs doit etre 2, 4 ou 8.");
        pthread_mutex_unlock(&tournoi.lock);
        return -1;
    }

    tournoi.is_active = 1;
    tournoi.status = TOURNOI_INSCRIPTIONS;
    tournoi.max_players = max_players;
    tournoi.nb_registered = 0;
    tournoi.current_round = 1;

    pthread_mutex_unlock(&tournoi.lock);

    char msg[128];
    snprintf(msg, sizeof(msg), "Tournoi cree pour %d joueurs.", max_players);
    notify_client(-1, msg);
    return 0;
}

int join_tournoi(int client_sock, const char *pseudo) {
    pthread_mutex_lock(&tournoi.lock);

    if (!tournoi.is_active || tournoi.status != TOURNOI_INSCRIPTIONS) {
        set_error("Aucun tournoi ouvert aux inscriptions.");
        pthread_mutex_unlock(&tournoi.lock);
        return -1;
    }

    if (tournoi.nb_registered >= tournoi.max_players) {
        set_error("Tournoi complet.");
        pthread_mutex_unlock(&tournoi.lock);
        return -1;
    }

    int idx = tournoi.nb_registered;
    tournoi.registered[idx] = client_sock;
    strncpy(tournoi.pseudos[idx], pseudo, MAX_PSEUDO - 1);
    tournoi.pseudos[idx][MAX_PSEUDO - 1] = '\0';
    tournoi.nb_registered++;

    char msg[128];
    snprintf(msg, sizeof(msg), "Inscrit au tournoi (%d/%d).",
             tournoi.nb_registered, tournoi.max_players);
    notify_client(client_sock, msg);

    if (tournoi.nb_registered == tournoi.max_players) {
        tournoi.status = TOURNOI_EN_COURS;

        for (int i = 0; i < tournoi.nb_registered; i++) {
            notify_client(tournoi.registered[i],
                          "Le tournoi commence ! Les matchs sont lances.");
        }

        int n = tournoi.nb_registered;
        (void)n;
        /* Lorsque le tournoi est complet, la logique de matchmaking doit
           être déclenchée par le module dédié au salon / à l'arbitrage. */
        pthread_mutex_unlock(&tournoi.lock);
        return 0;
    }

    pthread_mutex_unlock(&tournoi.lock);
    return 0;
}

void tournoi_match_result(const char *winner_pseudo) {
    pthread_mutex_lock(&tournoi.lock);

    if (!tournoi.is_active) {
        pthread_mutex_unlock(&tournoi.lock);
        return;
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "[Tournoi] %s avance au tour suivant !",
             winner_pseudo);
    printf("%s\n", msg);

    pthread_mutex_unlock(&tournoi.lock);
}
