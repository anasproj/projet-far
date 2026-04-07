#ifndef RESEAU_EXT_H
#define RESEAU_EXT_H

#include "reseau.h"








#define MSG_MATCH_FOUND   4   
#define MSG_SPECTATE      10  
#define MSG_LIST_ROOMS    11  
#define MSG_ROOM_LIST     12  
#define MSG_BOARD_STATE   13  
#define MSG_GAME_OVER     14  
#define MSG_STATS         15  
#define MSG_HISTORY       16  
#define MSG_TOURNAMENT_JOIN 17 
#define MSG_TOURNAMENT_INFO 18 
#define MSG_RESULT_REPORT   19 


typedef struct {
    int id;               
    pid_t pid;            
    char joueur1[32];     
    char joueur2[32];     
    int spectateurs;      
    int actif;            
} salon_info_t;

#define MAX_SALONS 50

#endif 
