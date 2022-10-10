#ifndef CLIENT_WORLD_H
#define CLIENT_WORLD_H

#include "player.h"

struct World {
    struct Player *players[MAX_CLIENTS];
    int active_players;
    struct Beast *beasts[MAX_BEASTS];
    char map[MAP_HEIGHT][MAP_WIDTH];
} world;

void load_map();
void print_map();
void print_info();

void create_treasure(enum COMMAND);

struct Player *create_player(int);
void deletePlayer(struct Player *);

#endif //CLIENT_WORLD_H
