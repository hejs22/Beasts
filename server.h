#ifndef CLIENT_SERVER_H
#define CLIENT_SERVER_H

#include <pthread.h>

struct ServerInfo {
    int socket;
    struct sockaddr_in address;
    int clients[MAX_CLIENTS];
    int number_of_clients;
    int up;
    char message[256];
    char buffer[1024];
} server;

void disconnect_socket(int);

#endif //CLIENT_SERVER_H
