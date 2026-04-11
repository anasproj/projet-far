#include "client.h"


int piece_selectionnee = 0;
square_t case_selectionnee = {-1, -1};
ds_dynamic_array coups_possibles = {0};
char ma_couleur = 0;
int mon_tour = 0;

void gerer_clic_echiquier(void) {
    if (ma_couleur == 2) return; 
    if (partie.current_player != ma_couleur) return;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse = GetMousePosition();
        if (mouse.y > 800) return; 

        square_t sq = pixel_vers_case(mouse.x, mouse.y);
        int piece = partie.board[SQUARE_RANK(sq) * 8 + SQUARE_FILE(sq)];

        if (!piece_selectionnee) {
            
            if (piece != CHESS_NONE && (CHESS_PIECE_COLOR(piece) == ma_couleur)) {
                piece_selectionnee = 1;
                case_selectionnee = sq;
                
                ds_dynamic_array_init(&coups_possibles, sizeof(move_t));
                for (size_t i = 0; i < coups_legaux.count; i++) {
                    move_t m;
                    ds_dynamic_array_get(&coups_legaux, i, &m);
                    if (SQUARES_EQUAL(m.start, sq)) {
                        ds_dynamic_array_append(&coups_possibles, &m);
                    }
                }
            }
        } else {
            
            move_t coup_joue;
            int valide = 0;
            for (size_t i = 0; i < coups_possibles.count; i++) {
                move_t m;
                ds_dynamic_array_get(&coups_possibles, i, &m);
                if (SQUARES_EQUAL(m.end, sq)) {
                    coup_joue = m;
                    valide = 1;
                    break;
                }
            }

            if (valide) {
                
                paquet_t pkt;
                pkt.type = MSG_MOVE;
                strcpy(pkt.pseudo, pseudo);
                sprintf(pkt.data, "%d,%d,%d,%d,%d,%d",
                        SQUARE_RANK(coup_joue.start), SQUARE_FILE(coup_joue.start),
                        SQUARE_RANK(coup_joue.end), SQUARE_FILE(coup_joue.end),
                        (int)coup_joue.move, (int)coup_joue.promotion);
                send(tcp_fd, &pkt, sizeof(paquet_t), 0);

                
                chess_apply_move(&partie, coup_joue);
                partie.current_player = chess_flip_player(partie.current_player);
                jouer_son_coup(coup_joue);

                
                ds_dynamic_array_clear(&coups_legaux);
                chess_generate_moves(&partie, &coups_legaux);
            }

            piece_selectionnee = 0;
            ds_dynamic_array_clear(&coups_possibles);
        }
    }
}

void gerer_chat_input(void) {
    bool was_visible = chat_visible;
    if (IsKeyPressed(KEY_C)) {
        chat_visible = !chat_visible;
    }

    if (!chat_visible || !was_visible) {
        
        while (GetCharPressed() > 0);
        return;
    }

    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= 32) && (key <= 125) && (chat_input_len < 255)) {
            chat_input[chat_input_len] = (char)key;
            chat_input[chat_input_len+1] = '\0';
            chat_input_len++;
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        chat_input_len--;
        if (chat_input_len < 0) chat_input_len = 0;
        chat_input[chat_input_len] = '\0';
    }

    if (IsKeyPressed(KEY_ENTER) && chat_input_len > 0) {
        paquet_t pkt;
        pkt.type = MSG_CHAT;
        strcpy(pkt.pseudo, pseudo);
        strcpy(pkt.data, chat_input);
        send(tcp_fd, &pkt, sizeof(paquet_t), 0);
        
        chat_input[0] = '\0';
        chat_input_len = 0;
    }
}
