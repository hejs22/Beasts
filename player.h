#include <pthread.h>

#ifndef CLIENT_PLAYER_H
#define CLIENT_PLAYER_H

enum DIRECTION {UP, RIGHT, DOWN, LEFT};
enum PLAYERTYPE {CPU, BEAST, HUMAN};

struct Player {
    int pox_x;
    int pos_y;
    int spawn_x;
    int spawn_y;
    enum DIRECTION dir;
    int deaths;
    int coins_carried;
    int coins_saved;
    int bush;
    int port;
    pthread_t pid;
};

struct Beast {
    int pos_x;
    int pos_y;
    int bush;
    int port;
    pthread_t pid;
};

#endif //CLIENT_PLAYER_H
