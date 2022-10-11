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
#include "player.h"

pthread_t listeningThread, playingThread, keyListenerThread;

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
           // printf("Client at socket %d disconnected. \n", socket);
            server.clients[i] = -1;
            close(socket);
        }
    }
}

void *client_server_connection_handler(void *arg) {
    char client_buffer[1024];
    struct Player *player;
    int socket = *(int *) arg;
    int connected = 1;
    char buffer[1024];

    int flag = 0;
    if (is_open(socket)) {
        // check for empty slots, if any is found create player
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server.clients[i] == -1) {
                world.players[world.active_players] = create_player(socket);
                player = world.players[world.active_players];
                world.active_players++;
                flag = 1;
                break;
            }
        }
    }
    // if all slots are taken disconnect and exit
    if (flag == 0) {
        disconnect_socket(socket);
        pthread_exit(NULL);
    }

    while (connected) {
        // if client isn't connected, disconnect his socket and free his Player structure
        if (!is_open(socket)) {
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (world.players[i]->socket == socket) {
                    disconnect_socket(socket);
                    deletePlayer(world.players[i]);
                    break;
                }
            }
            connected = 0;
        }

        // Handle client's requests TODO
        long bytes_received = recv(socket, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            enum COMMAND request = (enum COMMAND) buffer[0];
            int parameter = (int) buffer[1];

            switch (request) {
                case MOVE:
                    switch (parameter) {
                        case UP:
                            send(socket, "UP", 2, 0);
                            movePlayer(player, UP);
                            break;
                        case DOWN:
                            send(socket, "DOWN", 4, 0);
                            movePlayer(player, DOWN);
                            break;
                        case LEFT:
                            send(socket, "LEFT", 4, 0);
                            movePlayer(player, LEFT);
                            break;
                        case RIGHT:
                            send(socket, "RIGHT", 5, 0);
                            movePlayer(player, RIGHT);
                            break;
                        default:
                            break;
                    }
                    break;
                case WAIT:
                    send(socket, "WAIT", 4, 0);
                    break;
                case QUIT:
                    send(socket, "QUIT", 4, 0);
                    connected = 0;
                    break;
                case GET_MAP:
                    send(socket, "GET_MAP", 7, 0);
                    break;
                default:
                    break;
            }
            refresh();
        }

    }
    pthread_exit(NULL);
}

void *listen_for_clients(void *arg) {
    int client_socket;
    struct sockaddr_in client;
    //socklen_t client_size = sizeof(client);

    client_socket = accept(server.socket, NULL, NULL);
    send(client_socket, server.message, sizeof(server.message), 0);

    recv(client_socket, server.buffer, sizeof(server.buffer), 0);
    //printf("Player %s joined the server.\n", server.buffer);

    // check for empty client slots
    int flag = 1, i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i] == -1) {
            server.clients[i] = client_socket;
            flag = 0;
            break;
        }
    }
    // if all slots are taken, disconnect socket and exit
    if (flag) {
        disconnect_socket(client_socket);
        pthread_exit(NULL);
    }

    // create a handler thread for each connection and remember client's socket
    pthread_t handler;
    pthread_create(&handler, NULL, client_server_connection_handler, (void *) &server.clients[i]);
    //pthread_join(handler, NULL);
    pthread_exit(NULL);
}

void *key_listener(void *arg) {
    noecho();
    //printw("Entered into key listener");
    while (server.up) {
        unsigned char key = getch();

        switch (key) {
            case 'Q':
            case 'q':
                key = '0';
                printw("q");
                //endGame();
                break;
            case 'c':
                key = '0';
                create_object(SMALL_TREASURE);
                break;
            case 't':
                key = '0';
                create_object(MEDIUM_TREASURE);
                break;
            case 'T':
                key = '0';
                create_object(BIG_TREASURE);
                break;
            case 'B':
            case 'b':
                key = '0';
                // add beasts TODO

                break;
            default:
                key = '0';
        }
    }

    pthread_exit(NULL);
}

void init_ui() {
    initscr();

    if (has_colors() == TRUE) {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_WHITE); // White for walls
        init_pair(2, COLOR_GREEN, COLOR_BLACK); // Green for bushes
        init_pair(3, COLOR_YELLOW, COLOR_BLACK); // Yellow for treasures
        init_pair(4, COLOR_RED, COLOR_BLACK); // Red for beasts and campsites
        init_pair(5, COLOR_CYAN, COLOR_BLACK); // Blue for players
        init_pair(6, COLOR_BLACK, COLOR_BLACK); // Black for empty spaces
    }

    // Printing server's view
    print_map();
    print_info();

    refresh();
}

void *game(void *arg) {
    init_ui();
    noecho();
    pthread_create(&keyListenerThread, NULL, key_listener, NULL);

    // game logic TODO

    pthread_exit(NULL);
}

int main() { // server application
    srand(time(NULL));
    init_server();
    load_map();

    pthread_create(&playingThread, NULL, game, NULL);

    while (server.up) {
        pthread_create(&listeningThread, NULL, listen_for_clients, NULL);
        pthread_join(listeningThread, NULL);
    }

    return 0;
}