#include "client.h"


chess_state_t partie;
ds_dynamic_array coups_legaux = {0};
DS_ALLOCATOR *allocator = NULL;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <IP_SERVEUR> <PSEUDO>\n", argv[0]);
        return 1;
    }

    const char *ip_serveur = argv[1];
    strncpy(pseudo, argv[2], sizeof(pseudo) - 1);

    
    InitWindow(SCREEN_W, SCREEN_H, "Chess Tournament — World Polytech");
    SetTargetFPS(60);
    InitAudioDevice();

    charger_textures();
    charger_sons();

    
    while (!WindowShouldClose()) {
        
        switch (etat) {
            case STATE_CHOOSING_MODE:
            case STATE_HISTORY:
            case STATE_LISTING_ROOMS:
            case STATE_LOBBY:
                reseau_connecter_lobby(ip_serveur);
                reseau_traiter_udp();
                break;
            case STATE_CONNECTING:
                reseau_traiter_udp();
                reseau_traiter_tcp();
                break;
            case STATE_PLAYING:
            case STATE_SPECTATING:
                reseau_traiter_tcp();
                if (!chat_visible) {
                    gerer_clic_echiquier();
                    if (IsKeyPressed(KEY_BACKSPACE)) {
                        if (tcp_fd >= 0) { close(tcp_fd); tcp_fd = -1; }
                        etat = STATE_CHOOSING_MODE;
                        piece_selectionnee = 0;
                        snprintf(info_msg, sizeof(info_msg), "Retour à l'accueil...");
                    }
                }
                gerer_chat_input();
                break;
            case STATE_GAME_OVER:
                reseau_traiter_udp();
                if (IsKeyPressed(KEY_ENTER)) {
                    if (tcp_fd >= 0) { close(tcp_fd); tcp_fd = -1; }
                    etat = STATE_CHOOSING_MODE;
                    piece_selectionnee = 0;
                    snprintf(info_msg, sizeof(info_msg), "Retour à l'accueil...");
                }
                break;
        }

        
        BeginDrawing();
        switch (etat) {
            case STATE_CHOOSING_MODE: dessiner_choix_mode(); break;
            case STATE_HISTORY:       dessiner_historique(); break;
            case STATE_LISTING_ROOMS: dessiner_liste_salons(); break;
            case STATE_LOBBY:
            case STATE_CONNECTING:    dessiner_lobby(); break;
            case STATE_PLAYING:
            case STATE_SPECTATING:
                ClearBackground(COL_BG);
                dessiner_echiquier();
                dessiner_barre_info();
                dessiner_chat();
                break;
            case STATE_GAME_OVER:
                ClearBackground(COL_BG);
                dessiner_game_over();
                break;
        }
        EndDrawing();
    }

    
    for (int i = 0; i < 32; i++) {
        if (textures_pieces[i].id > 0) UnloadTexture(textures_pieces[i]);
    }
    if (sons_charges) {
        UnloadSound(sound_move); UnloadSound(sound_capture); UnloadSound(sound_castle);
    }
    CloseAudioDevice();
    CloseWindow();

    if (udp_fd >= 0) close(udp_fd);
    if (tcp_fd >= 0) close(tcp_fd);

    return 0;
}
