#ifndef RESEAU_H
#define RESEAU_H

#include <netinet/in.h>


typedef enum {
    MSG_JOIN,      
    MSG_WELCOME,   
    MSG_MOVE,      
    MSG_CHAT,     
    MSG_MATCH_FOUND = 4,
    MSG_SPECTATE = 10,
    MSG_LIST_ROOMS = 11,
    MSG_ROOM_LIST = 12,
    MSG_BOARD_STATE = 13,
    MSG_GAME_OVER = 14,
    MSG_STATS = 15,
    MSG_HISTORY = 16,
    MSG_TOURNAMENT_JOIN = 17,
    MSG_TOURNAMENT_INFO = 18,
    MSG_RESULT_REPORT = 19
} type_msg_t;


typedef struct {
    type_msg_t type;          
    char pseudo[32];          
    char data[256];           
} paquet_t;

#endif 
