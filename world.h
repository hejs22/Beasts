#ifndef CLIENT_WORLD_H
#define CLIENT_WORLD_H

#include "player.h"
#include "config.h"

struct World {
    struct Player *players[MAX_CLIENTS];
    struct Beast *beasts[MAX_BEASTS];
    char map[MAP_HEIGHT][MAP_WIDTH];
} world;

void load_map();
void print_map();
void print_info();

#endif //CLIENT_WORLD_H
