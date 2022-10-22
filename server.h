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

struct map {
    char map[BEAST_POV][BEAST_POV];
};

void send_map(struct Player *player);
void send_data(struct Player *player);
int is_position_valid(int row, int col);
void disconnect_socket(int);
void init_ui();
void run_orders(struct Player *);

#endif //CLIENT_SERVER_H
