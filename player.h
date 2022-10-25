#ifndef CLIENT_PLAYER_H
#define CLIENT_PLAYER_H

#include "config.h"
#include "world.h"

struct Player {
    int pos_row;
    int pos_col;
    int spawn_row;
    int spawn_col;
    int deaths;
    int coins_carried;
    int coins_saved;
    int bush;
    int socket;

    char avatar;
    int human;
    enum COMMAND command;
    int argument;

    pthread_t pid;
};

struct Beast {
    int pos_row;
    int pos_col;
    int bush;
    enum COMMAND command;
    int argument;
    enum TILE standing_at;
};


void printPlayer(const struct Player *player, int row, int col);
void handleCollisionPlayer(struct Player *player, int row, int col);

void dropTreasure(const struct Player *player);
void killPlayer(struct Player *player);

struct Player *create_player(int);
void deletePlayer(struct Player *);
void movePlayer(struct Player *, enum DIRECTION dir);
int validMove(int, int);

#endif //CLIENT_PLAYER_H
