#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>
#include <fcntl.h>

#include "raylib.h"
#include "reseau.h"
#include "reseau_ext.h"
#include "salon.h"
#include "ds.h"
#include "chess.h"


#define SQUARES_EQUAL(s1, s2) ((s1).rank == (s2).rank && (s1).file == (s2).file)
#define SQUARE_RANK(sq) ((sq).rank)
#define SQUARE_FILE(sq) ((sq).file)
#define CHESS_PIECE_COLOR(p) ((p) & COLOR_FLAG)
#define CHESS_PIECE_TYPE(p) ((p) & PIECE_FLAG)




#define SCREEN_W      800
#define SCREEN_H      850
#define CELL_SIZE     (SCREEN_W / 8)
#define PORT_SERVEUR  8080
#define INFO_BAR_H    50


#define COL_LIGHT  (Color){0xF4, 0xD3, 0xB2, 0xFF}
#define COL_DARK   (Color){0xA9, 0x7C, 0x6D, 0xFF}
#define COL_SEL    (Color){0x64, 0xB5, 0xF6, 0x99}
#define COL_MOVE   (Color){0x81, 0xC7, 0x84, 0x99}
#define COL_CHECK  (Color){0xFF, 0x00, 0x00, 0x99}
#define COL_LAST_L (Color){0xC4, 0xA3, 0x82, 0xFF}
#define COL_LAST_D (Color){0x79, 0x4C, 0x3D, 0xFF}
#define COL_BG     (Color){0x2E, 0x2E, 0x2E, 0xFF}
#define COL_TEXT   (Color){0xFF, 0xFF, 0xFF, 0xFF}




typedef enum {
    STATE_CHOOSING_MODE,
    STATE_LOBBY,
    STATE_LISTING_ROOMS,
    STATE_HISTORY,
    STATE_CONNECTING,
    STATE_PLAYING,
    STATE_SPECTATING,
    STATE_GAME_OVER
} client_state_t;

typedef struct {
    int port;
    char joueurs[64];
} salon_list_item_t;




extern client_state_t etat;
extern char pseudo[32];
extern char info_msg[256];


extern int udp_fd;
extern int tcp_fd;
extern struct sockaddr_in serveur_addr;


extern chess_state_t partie;
extern ds_dynamic_array coups_legaux;
extern char ma_couleur;
extern int mon_tour;


extern int piece_selectionnee;
extern square_t case_selectionnee;
extern ds_dynamic_array coups_possibles;


extern char chat_messages[20][300];
extern int chat_count;
extern char chat_input[256];
extern int chat_input_len;
extern int chat_visible;


extern salon_list_item_t salons_disponibles[MAX_SALONS];
extern int nb_salons_disponibles;


extern char historique_txt[8192];


extern Texture2D textures_pieces[32];
extern int textures_chargees;
extern Sound sound_move;
extern Sound sound_capture;
extern Sound sound_castle;
extern int sons_charges;






void reseau_connecter_lobby(const char *ip_serveur);
void reseau_joindre_jeu(void);
void reseau_rejoindre_tournoi(void);
void reseau_traiter_udp(void);
void reseau_traiter_tcp(void);


void dessiner_choix_mode(void);
void dessiner_lobby(void);
void dessiner_liste_salons(void);
void dessiner_echiquier(void);
void dessiner_barre_info(void);
void dessiner_chat(void);
void dessiner_game_over(void);
void dessiner_historique(void);


void gerer_clic_echiquier(void);
void gerer_chat_input(void);


void charger_textures(void);
void charger_sons(void);
void jouer_son_coup(move_t coup);


square_t pixel_vers_case(int px, int py);
Vector2 case_vers_pixel(int file, int rank);

#endif 
