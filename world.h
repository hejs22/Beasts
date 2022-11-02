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

int loadMap();
void printMap();
void printInfo();
void updateInfo();
void refreshScreen();
void printTile(enum TILE TYPE, int row, int col);

void createObject(enum TILE TYPE);
void printInitialObjects();

int findTreasureAt(int row, int col);
enum TILE getTileAt(int row, int col);

#endif //CLIENT_WORLD_H
