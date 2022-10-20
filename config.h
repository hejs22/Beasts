#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ncurses.h>
#include <pthread.h>
#include <time.h>
#include <locale.h>

enum DIRECTION {UP, RIGHT, DOWN, LEFT, STOP};
enum PLAYERTYPE {CPU, BEAST, HUMAN};
enum COMMAND {MOVE, QUIT, WAIT, SPAWN_BEAST};
enum TILE {BUSH, SMALL_TREASURE, MEDIUM_TREASURE, BIG_TREASURE, WALL, EMPTY, PLAYER, CAMPFIRE, DROPPED_TREASURE, BEAST_TILE};

#define MAX_CLIENTS 4
#define MAX_BEASTS 10
#define TREASURES_AMOUNT 50
#define BUSHES_AMOUNT 120
#define MAP_HEIGHT 28
#define MAP_WIDTH 86
#define TURN_TIME 350000
#define PLAYER_POV 11
#define INFO_POS_Y (MAP_HEIGHT + 2)
#define INFO_POS_X 5
#define CLIENT_INFO_POS_Y (PLAYER_POV + 5)
#define CLIENT_INFO_POS_X 5
#define MAP_FILENAME "map_small.txt"

#endif //CLIENT_CONFIG_H
