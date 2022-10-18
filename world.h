#ifndef CLIENT_WORLD_H
#define CLIENT_WORLD_H

#include "player.h"

enum TILE {BUSH, SMALL_TREASURE, MEDIUM_TREASURE, BIG_TREASURE, WALL, EMPTY, PLAYER, CAMPFIRE, DROPPED_TREASURE};

struct World {
    struct Player *players[MAX_CLIENTS];
    int active_players;
    struct Beast *beasts[MAX_BEASTS];
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

struct Player *create_player(int);
void deletePlayer(struct Player *);
void movePlayer(struct Player *, enum DIRECTION);
int validMove(int, int);

void print_player(struct Player *player, int row, int col);
void handle_collision(struct Player *player, int row, int col);

void dropTreasure(struct Player *player);
void killPlayer(struct Player *player);

#endif //CLIENT_WORLD_H
