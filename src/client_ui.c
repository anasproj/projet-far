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
            
            tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in addr = serveur_addr;
            addr.sin_port = htons(salons_disponibles[i].port);
            if (connect(tcp_fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                etat = STATE_SPECTATING;
                
                paquet_t pkt; pkt.type = MSG_SPECTATE; strcpy(pkt.pseudo, pseudo);
                send(tcp_fd, &pkt, sizeof(paquet_t), 0);
            }
        }
    }

    Rectangle btnBack = { 50, 700, 150, 40 };
    DrawRectangleRec(btnBack, MAROON);
    DrawText("RETOUR", btnBack.x + 30, btnBack.y + 10, 20, WHITE);

    if ((IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), btnBack)) || IsKeyPressed(KEY_ESCAPE)) {
        etat = STATE_CHOOSING_MODE;
    }
}

void dessiner_echiquier(void) {
    for (int r = 0; r < 8; r++) {
        for (int f = 0; f < 8; f++) {
            square_t sq = MK_SQUARE(r, f);
            Vector2 p = case_vers_pixel(f, r);
            Rectangle rect = { p.x, p.y, CELL_SIZE, CELL_SIZE };
            
            Color col = ((r + f) % 2 == 0) ? COL_DARK : COL_LIGHT;
            
            
            if (piece_selectionnee && SQUARES_EQUAL(case_selectionnee, sq)) col = COL_SEL;
            
            DrawRectangleRec(rect, col);

            
            int piece = partie.board[r * 8 + f]; 
            if (piece != CHESS_NONE) {
                Texture2D tex = textures_pieces[piece];
                if (tex.id > 0) {
                    DrawTexturePro(tex, 
                        (Rectangle){0, 0, tex.width, tex.height},
                        (Rectangle){rect.x + 5, rect.y + 5, CELL_SIZE - 10, CELL_SIZE - 10},
                        (Vector2){0, 0}, 0, WHITE);
                } else {
                    
                    const char *symbole = "?";
                    DrawText(symbole, rect.x + 25, rect.y + 20, 40, BLACK);
                }
            }
        }
    }

    
    if (piece_selectionnee) {
        for (size_t i = 0; i < coups_possibles.count; i++) {
            move_t m;
            ds_dynamic_array_get(&coups_possibles, i, &m);
            Vector2 p = case_vers_pixel(SQUARE_FILE(m.end), SQUARE_RANK(m.end));
            DrawCircle(p.x + CELL_SIZE/2, p.y + CELL_SIZE/2, 10, (Color){0, 0, 0, 100});
        }
    }
}

void dessiner_barre_info(void) {
    DrawRectangle(0, 800, SCREEN_W, INFO_BAR_H, DARKGRAY);
    const char *txt_tour = (partie.current_player == CHESS_WHITE) ? "Tour : Blancs" : "Tour : Noirs";
    DrawText(txt_tour, 20, 815, 20, WHITE);
    
    const char *mon_role = (ma_couleur == CHESS_WHITE) ? "Vous jouez les BLANCS" : 
                           (ma_couleur == CHESS_BLACK) ? "Vous jouez les NOIRS" : "MODE SPECTATEUR";
    DrawText(mon_role, 250, 815, 20, LIGHTGRAY);
    
    DrawText("Appuyez sur [C] pour le Chat", 550, 815, 18, (chat_visible ? GOLD : RAYWHITE));
}

void dessiner_chat(void) {
    if (!chat_visible) return;

    DrawRectangle(500, 400, 280, 380, (Color){40, 40, 40, 220});
    DrawRectangleLines(500, 400, 280, 380, GOLD);

    for (int i = 0; i < chat_count; i++) {
        DrawText(chat_messages[i], 510, 410 + i * 18, 15, WHITE);
    }

    char input_display[300];
    snprintf(input_display, sizeof(input_display), "> %s_", chat_input);
    DrawText(input_display, 510, 750, 15, GOLD);
}

void dessiner_game_over(void) {
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){0, 0, 0, 150});
    DrawText("FIN DE PARTIE", 250, 350, 40, GOLD);
    DrawText(info_msg, 200, 420, 25, WHITE);
    
    Rectangle btnBack = { (SCREEN_W - 250) / 2, 550, 250, 50 };
    DrawRectangleRec(btnBack, DARKGRAY);
    DrawText("RETOUR MENU", btnBack.x + 40, btnBack.y + 12, 25, WHITE);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), btnBack)) {
        etat = STATE_CHOOSING_MODE;
    }
    
    DrawText("Appuyez sur [ENTREE] pour revenir au lobby", 180, 500, 20, LIGHTGRAY);
}

void dessiner_historique(void) {
    ClearBackground(COL_BG);
    DrawText("HISTORIQUE DES PARTIES", 50, 50, 30, COL_TEXT);
    DrawText(historique_txt, 50, 120, 15, LIGHTGRAY);

    Rectangle btnBack = { 50, 750, 150, 40 };
    DrawRectangleRec(btnBack, DARKGRAY);
    DrawText("RETOUR", btnBack.x + 30, btnBack.y + 10, 20, WHITE);

    if ((IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), btnBack)) || IsKeyPressed(KEY_ESCAPE)) {
        etat = STATE_CHOOSING_MODE;
    }
}
