#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H

enum DIRECTION {
    UP, RIGHT, DOWN, LEFT, STOP
};
enum PLAYERTYPE {
    CPU, BEAST, HUMAN
};
enum COMMAND {
    MOVE,
    QUIT,
    JOIN,
    WAIT,
    GET_MAP,
    SPAWN_BEAST,
    SPAWN_CPU,
    SPAWN_SMALL_TREASURE,
    SPAWN_MEDIUM_TREASURE,
    SPAWN_BIG_TREASURE
};
enum TILE {
    BUSH, SMALL_TREASURE, MEDIUM_TREASURE, BIG_TREASURE, WALL, EMPTY, PLAYER, CAMPFIRE, DROPPED_TREASURE, BEAST_TILE
};

struct type_and_pid {
    int pid;
    char type;
    int socket;
};

struct point {
    int x;
    int y;
};

#define MAX_CLIENTS 4
#define MAX_BEASTS 8
#define TREASURES_AMOUNT 50
#define BUSHES_AMOUNT 120
#define MAP_HEIGHT 28
#define MAP_WIDTH 86
#define TURN_TIME 350000
#define PLAYER_POV 5
#define BEAST_POV 5
#define INFO_POS_Y (MAP_HEIGHT + 2)
#define INFO_POS_X 5
#define CLIENT_INFO_POS_Y (PLAYER_POV + 5)
#define CLIENT_INFO_POS_X 5
#define MAP_FILENAME "map_small.txt"

#endif //CLIENT_CONFIG_H
