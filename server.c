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
#include "server.h"

pthread_t listeningThread;

void init_server() {
    // set server parameters
    server.up = 0;
    server.number_of_clients = MAX_CLIENTS;
    strcpy(server.message, "Connection estabilished.");

    // create the server socket
    server.socket = socket(AF_INET, SOCK_STREAM, 0);
    for (int i = 0; i < MAX_CLIENTS; i++) server.clients[i] = -1;

    // define server address
    server.address.sin_port = htons(9002);
    server.address.sin_family = AF_INET;
    server.address.sin_addr.s_addr = INADDR_ANY;

    // bind socket to specified IP and PORT
    bind(server.socket, (struct sockaddr *) &(server.address), sizeof(server.address));

    // allow MAX_CLIENTS number of clients
    if (listen(server.socket, MAX_CLIENTS) == 0) printf("Listening...\n");
    else printf("Error while setting up listen().\n");

    server.up = 1;
};

int is_open(int socket) {
    int res = recv(socket,NULL,1, MSG_PEEK | MSG_DONTWAIT);
    if (res != 0) return 1;
    return 0;
}

void disconnect_socket(int socket) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i] == socket) {
            printf("Client at socket %d disconnected. \n", socket);
            server.clients[i] = -1;
            close(socket);
        }
    }
}

void *client_server_connection_handler(void *arg) {
    char client_buffer[1024];
    int socket = *(int *) arg;
    int connected = 1;

    while (connected) {
        if (!is_open(socket)) {
            disconnect_socket(socket);
            connected = 0;
        }

        recv(socket, client_buffer, sizeof(client_buffer), 0);
        //if (res > 0) printf("Client %d: %s\n", socket, client_buffer);
        //usleep(100000);
        //printf("%d\n", *(int *) arg);
    }
    pthread_exit(NULL);
}

void *listen_for_clients(void *arg) {
    int client_socket;
    struct sockaddr_in client;
    socklen_t client_size = sizeof(client);

    client_socket = accept(server.socket, NULL, NULL);
    send(client_socket, server.message, sizeof(server.message), 0);

    recv(client_socket, server.buffer, sizeof(server.buffer), 0);
    printf("Player %s joined the server.\n", server.buffer);

    int i = 0;
    while (server.clients[i] != -1) i++;
    server.clients[i] = client_socket;

    // create a handler thread for each connection and remember client's socket
    pthread_t handler;
    pthread_create(&handler, NULL, client_server_connection_handler, (void *) &server.clients[i]);
    //pthread_join(handler, NULL);
    pthread_exit(NULL);
}

void init_ui() {
    initscr();
    keypad(stdscr, TRUE);
    noecho();

    if (has_colors() == TRUE) {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_WHITE); // White for walls
        init_pair(2, COLOR_GREEN, COLOR_BLACK); // Green for bushes
        init_pair(3, COLOR_YELLOW, COLOR_BLACK); // Yellow for treasures
        init_pair(4, COLOR_RED, COLOR_BLACK); // Red for beasts and campsites
        init_pair(5, COLOR_BLUE, COLOR_BLACK); // Blue for players
        init_pair(5, COLOR_BLACK, COLOR_BLACK); // Black for empty spaces
    }

    // Printing server's view
    print_map();
    print_info();

    refresh();
    while(1) {
        getch();
        refresh();
    }
    endwin(); //koniec
}

int main() { // server application

    init_server();
    //load_map();
    //init_ui();

    while (server.up) {
        pthread_create(&listeningThread, NULL, listen_for_clients, NULL);
        pthread_join(listeningThread, NULL);

    }

    return 0;
}