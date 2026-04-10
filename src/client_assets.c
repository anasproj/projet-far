#include "client.h"


Texture2D textures_pieces[32] = {0};
int textures_chargees = 0;
Sound sound_move;
Sound sound_capture;
Sound sound_castle;
int sons_charges = 0;

void charger_textures(void) {
    const char *codes_pieces[] = { NULL, "p", "r", "n", "b", "q", "k" };
    const char codes_couleurs[] = { 0, 'l', 'd' };
    const char *noms_pieces[] = { NULL, "pawn", "rook", "knight", "bishop", "queen", "king" };
    const char *couleurs[] = { NULL, "white", "black" };

    for (int c = 1; c <= 2; c++) {
        for (int p = 1; p <= 6; p++) {
            char path[512];
            int color_flag = (c == 1) ? CHESS_WHITE : CHESS_BLACK;
            int idx = color_flag | p;

            const char *dirs[] = { "assets/", "build/assets/", "../assets/", "../../assets/" };
            int found = 0;
            
            for (int d = 0; d < 4; d++) {
                snprintf(path, sizeof(path), "%sChess_%c%ct60.png",
                         dirs[d], codes_pieces[p][0], codes_couleurs[c]);
                if (FileExists(path)) {
                    textures_pieces[idx] = LoadTexture(path);
                    if (textures_pieces[idx].id > 0) { found = 1; break; }
                }

                snprintf(path, sizeof(path), "%s%s_%s.png",
                         dirs[d], couleurs[c], noms_pieces[p]);
                if (FileExists(path)) {
                    textures_pieces[idx] = LoadTexture(path);
                    if (textures_pieces[idx].id > 0) { found = 1; break; }
                }
            }
            if (!found) {
                printf("[DEBUG] Texture non trouvée pour : %s %s\n", couleurs[c], noms_pieces[p]);
            } else {
                printf("[DEBUG] Texture chargée : %s (%s %s)\n", path, couleurs[c], noms_pieces[p]);
            }
        }
    }
    textures_chargees = 1;
}

void charger_sons(void) {
    const char *dirs[] = { "assets/", "build/assets/", "../assets/", "../../assets/" };
    char path[512];

    for (int d = 0; d < 4; d++) {
        snprintf(path, sizeof(path), "%smove.ogg", dirs[d]);
        if (FileExists(path)) {
            sound_move = LoadSound(path);
            
            snprintf(path, sizeof(path), "%scapture.ogg", dirs[d]);
            if (FileExists(path)) sound_capture = LoadSound(path);
            
            snprintf(path, sizeof(path), "%scastle.ogg", dirs[d]);
            if (FileExists(path)) sound_castle = LoadSound(path);
            
            sons_charges = 1;
            printf("[DEBUG] Sons chargés depuis %s\n", dirs[d]);
            break;
        }
    }
}

void jouer_son_coup(move_t coup) {
    if (!sons_charges) return;
    
    if (coup.move & CHESS_CAPTURE) {
        PlaySound(sound_capture);
    } else if ((coup.move & CHESS_CASTLE_SHORT) || (coup.move & CHESS_CASTLE_LONG)) {
        PlaySound(sound_castle);
    } else {
        PlaySound(sound_move);
    }
}
