#ifndef CLIENT_SERVER_H
#define CLIENT_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include "config.h"
#include "world.h"

struct ServerInfo {
    // connection parameters
    int socket;
    struct sockaddr_in address;
    int clients[MAX_CLIENTS];

    // additional parameters
    int number_of_clients;
    int up;
    char message[256];
    char buffer[1024]
} server;

struct ClientInfo {
    int socket;
    int port;
    pthread_t thread;
};

#endif //CLIENT_SERVER_H
