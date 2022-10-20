#ifndef CLIENT_WORLD_H
#define CLIENT_WORLD_H

#include "config.h"

struct World {
    struct Player *players[MAX_CLIENTS];
    int active_players;
    struct Beast *beasts[MAX_BEASTS];
    int active_beasts;
    char map[MAP_HEIGHT][MAP_WIDTH];
    int treasure_map[MAP_HEIGHT][MAP_WIDTH];
    int campfire_row;
    int campfire_col;
} world;

void load_map();
void print_map();
void print_info();
void update_info();
void refresh_screen();
void print_tile(enum TILE, int, int);

void create_object(enum TILE);
void print_initial_objects();

int find_treasure_at(int row, int col);
enum TILE get_tile_at(int row, int col);

#endif //CLIENT_WORLD_H
