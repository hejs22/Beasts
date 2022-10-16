#ifndef CLIENT_SERVER_H
#define CLIENT_SERVER_H

#include <pthread.h>

struct ServerInfo {
    int socket;
    pthread_t pid;
    struct sockaddr_in address;
    int clients[MAX_CLIENTS];
    int number_of_clients;
    int up;
    int round;
    char message[256];
    char buffer[1024];
} server;

struct data_transfer {
    int pos_X;
    int pos_Y;
    int coins_saved;
    int coins_carried;
    int deaths;
    int round;
};

void disconnect_socket(int);
void init_ui();

#endif //CLIENT_SERVER_H
