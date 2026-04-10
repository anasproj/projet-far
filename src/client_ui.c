#include "client.h"


char chat_messages[20][300] = {0};
int chat_count = 0;
char chat_input[256] = "";
int chat_input_len = 0;
int chat_visible = 0;

salon_list_item_t salons_disponibles[MAX_SALONS];
int nb_salons_disponibles = 0;

char historique_txt[8192] = "";


square_t pixel_vers_case(int px, int py) {
    int file = px / CELL_SIZE;
    int rank = 7 - (py / CELL_SIZE);

    if (ma_couleur == CHESS_BLACK) {
        file = 7 - file;
        rank = 7 - rank;
    }
    return MK_SQUARE(rank, file);
}

Vector2 case_vers_pixel(int file, int rank) {
    if (ma_couleur == CHESS_BLACK) {
        file = 7 - file;
        rank = 7 - rank;
    }
    return (Vector2){ file * CELL_SIZE, (7 - rank) * CELL_SIZE };
}

void dessiner_choix_mode(void) {
    ClearBackground(COL_BG);
    const char *title = "CHESS TOURNAMENT";
    int fontSize = 50;
    int titleW = MeasureText(title, fontSize);
    DrawText(title, (SCREEN_W - titleW) / 2, 100, fontSize, COL_TEXT);

    Rectangle btn1 = { (SCREEN_W - 300) / 2, 250, 300, 60 };
    Rectangle btn2 = { (SCREEN_W - 300) / 2, 330, 300, 60 };
    Rectangle btn3 = { (SCREEN_W - 300) / 2, 410, 300, 60 };
    Rectangle btn4 = { (SCREEN_W - 300) / 2, 490, 300, 60 };

    
    DrawRectangleRec(btn1, DARKGRAY);
    DrawText("JOUER (1v1)", btn1.x + 60, btn1.y + 15, 25, WHITE);
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), btn1)) {
        reseau_joindre_jeu();
    }

    
    DrawRectangleRec(btn2, (Color){0, 121, 241, 255});
    DrawText("TOURNOI (4p)", btn2.x + 55, btn2.y + 15, 25, WHITE);
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), btn2)) {
        reseau_rejoindre_tournoi();
    }

    
    DrawRectangleRec(btn3, DARKGRAY);
    DrawText("SPECTATEUR", btn3.x + 75, btn3.y + 15, 25, WHITE);
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), btn3)) {
        etat = STATE_LISTING_ROOMS;
        
        paquet_t pkt; pkt.type = MSG_LIST_ROOMS; strcpy(pkt.pseudo, pseudo);
        sendto(udp_fd, &pkt, sizeof(paquet_t), 0, (struct sockaddr *)&serveur_addr, sizeof(serveur_addr));
    }

    
    DrawRectangleRec(btn4, DARKGRAY);
    DrawText("HISTORIQUE", btn4.x + 80, btn4.y + 15, 25, WHITE);
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), btn4)) {
        etat = STATE_HISTORY;
        paquet_t pkt; pkt.type = MSG_HISTORY; strcpy(pkt.pseudo, pseudo);
        sendto(udp_fd, &pkt, sizeof(paquet_t), 0, (struct sockaddr *)&serveur_addr, sizeof(serveur_addr));
    }

    DrawText(info_msg, 20, SCREEN_H - 30, 20, LIGHTGRAY);
}

void dessiner_lobby(void) {
    ClearBackground(COL_BG);
    DrawText("LOBBY - ATTENTE D'ADVERSAIRE", 150, 200, 30, COL_TEXT);
    DrawText(TextFormat("Joueur : %s", pseudo), 150, 250, 25, LIGHTGRAY);
    
    
    static float timer = 0;
    timer += GetFrameTime();
    const char *dots = "...";
    int numDots = ((int)(timer * 2) % 4);
    DrawText(TextSubtext(dots, 0, numDots), 600, 200, 30, COL_TEXT);

    DrawText("Recherche d'un match en cours", 250, 350, 20, GRAY);
    
    Rectangle btnBack = { (SCREEN_W - 200) / 2, 500, 200, 50 };
    DrawRectangleRec(btnBack, DARKGRAY);
    DrawText("RETOUR", btnBack.x + 55, btnBack.y + 12, 25, WHITE);

    if ((IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), btnBack)) || IsKeyPressed(KEY_ESCAPE)) {
        etat = STATE_CHOOSING_MODE;
    }
}

void dessiner_liste_salons(void) {
    ClearBackground(COL_BG);
    DrawText("SALONS ACTIFS", 50, 50, 30, COL_TEXT);
    
    if (nb_salons_disponibles == 0) {
        DrawText("Aucun salon trouvé", 50, 100, 20, GRAY);
    }

    for (int i = 0; i < nb_salons_disponibles; i++) {
        Rectangle r = { 50, 100 + i * 50, 500, 40 };
        DrawRectangleRec(r, DARKGRAY);
        DrawText(salons_disponibles[i].joueurs, r.x + 10, r.y + 10, 20, WHITE);
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), r)) {
            
