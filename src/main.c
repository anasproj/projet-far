#include "stdlib.h"
#include "raylib.h"
#include "chess.h"
#include "game.h"
#include "ds.h"
#include <dlfcn.h>
#include <fcntl.h>       // Pour rendre le socket non-bloquant
#include <sys/socket.h>  // Pour le réseau
#include <arpa/inet.h>   // Pour les adresses IP
#include <unistd.h>
#include "reseau.h"      // Le protocole que le Rôle 2 a créé


#define SUBCMD_GAME "game"
#define SUBCMD_COUNT_POSITIONS "count-positions"
#define SUBCMD_MOVE "move"

typedef struct arguments_t {
    char *subcmd;
    int depth;
    char *player1;
    char *player2;
    char *fen;
} arguments_t;

init_fn init_player1 = NULL;
init_fn init_player2 = NULL;
move_fn move_player1 = NULL;
move_fn move_player2 = NULL;

extern void init_player1_fn(void *memory, unsigned long size) { return init_player1(memory, size); }
extern void init_player2_fn(void *memory, unsigned long size) { return init_player2(memory, size); }

extern void move_player1_fn(const chess_state_t *state, move_t *choices, int count, int *index) { return move_player1(state, choices, count, index); }
extern void move_player2_fn(const chess_state_t *state, move_t *choices, int count, int *index) { return move_player2(state, choices, count, index); }

void parse_arguments(int argc, char **argv, arguments_t *args) {
    ds_argparse_parser parser = {0};
    ds_argparse_parser_init(
        &parser,
        "chess-engine",
        "Chess Engine in C",
        "0.1"
    );

    ds_argparse_add_argument(&parser, (ds_argparse_options){
        .short_name = 's',
        .long_name = "subcmd",
        .description = "The subcommand to use with the engine: `game`, `count-positions`, `move`. Default: `game`",
        .type = ARGUMENT_TYPE_POSITIONAL,
        .required = false,
    });

    ds_argparse_add_argument(&parser, (ds_argparse_options){
        .short_name = 'd',
        .long_name = "depth",
        .description = "The depth to use with the engine. Default: `0`.",
        .type = ARGUMENT_TYPE_VALUE,
        .required = false,
    });

    ds_argparse_add_argument(&parser, (ds_argparse_options){
        .short_name = 'a',
        .long_name = "player1",
        .description = "the strategy for player 1, a path to a DLL",
        .type = ARGUMENT_TYPE_POSITIONAL,
        .required = false,
    });

    ds_argparse_add_argument(&parser, (ds_argparse_options){
        .short_name = 'b',
        .long_name = "player2",
        .description = "the strategy for player 2, a path to a DLL",
        .type = ARGUMENT_TYPE_POSITIONAL,
        .required = false,
    });

    ds_argparse_add_argument(&parser, (ds_argparse_options){
        .short_name = 'f',
        .long_name = "fen",
        .description = "the FEN string to use for the game",
        .type = ARGUMENT_TYPE_VALUE,
        .required = false,
    });

    DS_UNREACHABLE(ds_argparse_parse(&parser, argc, argv));

    args->subcmd = ds_argparse_get_value_or_default(&parser, "subcmd", SUBCMD_GAME);
    args->depth = atoi(ds_argparse_get_value_or_default(&parser, "depth", "0"));
    args->player1 = ds_argparse_get_value(&parser, "player1");
    args->player2 = ds_argparse_get_value(&parser, "player2");
    args->fen = ds_argparse_get_value_or_default(&parser, "fen", CHESS_START);

    ds_argparse_parser_free(&parser);
}

int load_move_function(const char *player, move_fn *move_player, init_fn *init_player) {
    void *strat = dlopen(player, RTLD_NOW);
    if (strat == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    *move_player = (move_fn)dlsym(strat, "chess_move");
    if (*move_player == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    *init_player = (init_fn)dlsym(strat, "chess_init");
    if (*init_player == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    return 0;
}

int game(arguments_t args) {
    /*if (load_move_function(args.player1, &move_player1, &init_player1) != 0) {
        return 1;
    }
    if (load_move_function(args.player2, &move_player2, &init_player2) != 0) {
        return 1;
    }

    init_player1_fn(NULL, 0);
    init_player2_fn(NULL, 0); */ 

    init(NULL, 0);
    state_init(args.fen); 

    // ==========================================
    // 1. INITIALISATION DU RÉSEAU (Avant la boucle)
    // ==========================================
    int socket_client = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_client < 0) {
        perror("Erreur création socket client");
        return 1;
    }

    // L'INSTRUCTION MAGIQUE : Rendre le socket NON-BLOQUANT
    int flags = fcntl(socket_client, F_GETFL, 0);
    fcntl(socket_client, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // L'IP de ton serveur

    // Envoi du tout premier message pour se connecter (MSG_JOIN)
    paquet_t paquet_connexion;
    paquet_connexion.type = MSG_JOIN;
    strcpy(paquet_connexion.pseudo, "Joueur_Raylib");
    
    sendto(socket_client, &paquet_connexion, sizeof(paquet_t), 0,
           (struct sockaddr *)&server_addr, sizeof(server_addr));
           
    printf("Demande de connexion envoyée au serveur...\n");
    // ==========================================

    // La boucle principale du jeu Raylib
    while (!WindowShouldClose()) {
        
        // ==========================================
        // 2. LECTURE DU RÉSEAU SANS BLOQUER (À chaque frame)
        // ==========================================
        paquet_t paquet_recu;
        
        // recvfrom va vérifier très vite s'il y a un message. 
        // Comme on est en O_NONBLOCK, si c'est vide, ça renvoie < 0 instantanément et on passe à la suite !
        int n = recvfrom(socket_client, &paquet_recu, sizeof(paquet_t), 0, NULL, NULL);

        if (n > 0) {
            // S'il y a plus de 0 octet, c'est qu'on a vraiment reçu un paquet de ton serveur !
            if (paquet_recu.type == MSG_WELCOME) {
                printf("[SERVEUR] %s\n", paquet_recu.data);
            }
        }
        // ==========================================

        tick(GetFrameTime()); // Fait avancer la logique du jeu Raylib (affichage, clics...)
    }

    CloseWindow();
    close(socket_client); // On ferme proprement en quittant

    return 0;
}

int count_positions(arguments_t args) {
    chess_state_t state = {0};

    ds_string_slice fen = DS_STRING_SLICE(args.fen);
    chess_init_fen(&state, fen);

    perft_t perft = {0};
    chess_count_positions(&state, args.depth, &perft);
    DS_LOG_INFO("Depth: %d => Positions: %d Captures: %d E.p.: %d Castles: %d "
                "Promotions: %d Checks: %d Checkmates: %d",
                args.depth, perft.nodes, perft.captures, perft.enp,
                perft.castles, perft.promote, perft.checks, perft.checkmates);

    return 0;
}

int move(arguments_t args) {
    if (load_move_function(args.player1, &move_player1, &init_player1) != 0) {
        return 1;
    }

    init_player1_fn(NULL, 0);

    int index = -1;
    chess_state_t state = {0};
    ds_dynamic_array moves = {0}; /* move_t */

    ds_string_slice fen = DS_STRING_SLICE(args.fen);
    chess_init_fen(&state, fen);

    ds_dynamic_array_init_allocator(&moves, sizeof(move_t), NULL);
    chess_generate_moves(&state, &moves);

    move_player1_fn(&state, moves.items, moves.count, &index);

    move_t *move = NULL;
    ds_dynamic_array_get_ref(&moves, index, (void **)&move);

    DS_LOG_INFO("Move: %d,%d %d,%d", move->start.file, move->start.rank, move->end.file, move->end.rank);

    ds_dynamic_array_free(&moves);

    return 0;
}

int main(int argc, char **argv) {
    arguments_t args = {0};
    parse_arguments(argc, argv, &args);

    if (DS_STRCMP(args.subcmd, SUBCMD_GAME) == 0) {
        return game(args);
    } else if (DS_STRCMP(args.subcmd, SUBCMD_COUNT_POSITIONS) == 0) {
        return count_positions(args);
    } else if (DS_STRCMP(args.subcmd, SUBCMD_MOVE) == 0) {
        return move(args);
    }

    return 1;
}
