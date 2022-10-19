#ifndef CLIENT_PLAYER_H
#define CLIENT_PLAYER_H

enum DIRECTION {UP, RIGHT, DOWN, LEFT, STOP};
enum PLAYERTYPE {CPU, BEAST, HUMAN};
enum COMMAND {MOVE, QUIT, JOIN, WAIT, GET_MAP, SPAWN_BEAST, SPAWN_CPU, SPAWN_SMALL_TREASURE, SPAWN_MEDIUM_TREASURE, SPAWN_BIG_TREASURE};

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
};


#endif //CLIENT_PLAYER_H
