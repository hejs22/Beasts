#ifndef CLIENT_SERVER_H
#define CLIENT_SERVER_H

#include <pthread.h>

struct ServerInfo {
    int socket;
    pthread_t pid;
    struct sockaddr_in address;
    int clients[MAX_CLIENTS];
    int beast_client;
    int number_of_clients;
    int up;
    int beasts_pid;
    int round;
    char message[256];
    char buffer[1024];
} server;

struct player_data_transfer {
    char map[PLAYER_POV][PLAYER_POV];
    int pos_X;
    int pos_Y;
    int coins_saved;
    int coins_carried;
    int deaths;
    int round;
    int camp_x;
    int camp_y;
};

void send_map(struct Player *player);

void sendData(const struct Player *player);

int isPositionValid(int row, int col);

void disconnectSocket(int);

void initUi();

void runOrders(struct Player *player);

#endif //CLIENT_SERVER_H
