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
#include <time.h>
#include <locale.h>

pthread_t listeningThread, playingThread, keyListenerThread;

// SERVER INITIALIZATION ///////////////////////////////////////////////////////////////////////////////////////////////

void init_server() {
    // set server parameters
    server.up = 0;
    server.number_of_clients = MAX_CLIENTS;


    // create the server socket
    server.socket = socket(AF_INET, SOCK_STREAM, 0);
    for (int i = 0; i < MAX_CLIENTS; i++) server.clients[i] = -1;
    server.beast_client = -1;

    // define server address
    server.address.sin_port = htons(9002);
    server.address.sin_family = AF_INET;
    server.address.sin_addr.s_addr = INADDR_ANY;

    // bind socket to specified IP and PORT
    bind(server.socket, (struct sockaddr *) &(server.address), sizeof(server.address));

    // allow MAX_CLIENTS number of clients
    if (listen(server.socket, MAX_CLIENTS) == 0) printf("Listening...\n");
    else printf("Error while setting up listen().\n");

    server.pid = getpid();
    sprintf(server.message, "%d", server.pid);
    server.round = 0;
    server.up = 1;
};

void init_ui() {
    initscr();

    if (has_colors() == TRUE) {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_BLUE); // Blue for players
        init_pair(2, COLOR_WHITE, COLOR_CYAN); // Blue for players
        init_pair(3, COLOR_WHITE, COLOR_MAGENTA); // Blue for players
        init_pair(4, COLOR_WHITE, COLOR_RED); // Blue for players

        init_pair(11, COLOR_WHITE, COLOR_WHITE); // White for walls
        init_pair(12, COLOR_GREEN, COLOR_BLACK); // Green for bushes
        init_pair(13, COLOR_YELLOW, COLOR_BLACK); // Yellow for treasures
        init_pair(14, COLOR_RED, COLOR_BLACK); // Red for beasts and campsites
        init_pair(15, COLOR_BLACK, COLOR_BLACK); // Black for empty spaces
    }

    // Printing server's view
    print_map();
    print_initial_objects();
    print_info();
    refresh();
}

// CONNECTION MANAGEMENT ///////////////////////////////////////////////////////////////////////////////////////////////


int is_open(int socket) {
    int res = recv(socket,NULL,1, MSG_PEEK | MSG_DONTWAIT);
    if (res != 0) return 1;
    return 0;
}

void disconnect_socket(int socket) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i] == socket) {
            server.clients[i] = -1;
            close(socket);
        }
    }
}

void *client_server_connection_handler(void *arg) {
    struct Player *player;
    int socket = *(int *) arg;
    int connected = 1;
    char buffer[1024];

    int flag = 0;
    if (is_open(socket)) {
        // check for empty slots, if any is found create player
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server.clients[i] == socket) {
                player = create_player(socket);
                recv(socket, buffer, sizeof(buffer), 0);
                player->pid = atoi(buffer);
                player->avatar = i + '1';
                world.players[i] = player;
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
                if (world.players[i] != NULL) {
                    if (world.players[i]->socket == socket) {
                        disconnect_socket(socket);
                        deletePlayer(world.players[i]);
                        break;
                    }
                }
            }
            connected = 0;
        }

        long bytes_received = recv(socket, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            enum COMMAND request = (enum COMMAND) buffer[0];
            int parameter = (int) buffer[1];

            switch (request) {
                case MOVE:
                    switch (parameter) {
                        case UP:
                            player->command = MOVE;
                            player->argument = UP;
                            break;
                        case DOWN:
                            player->command = MOVE;
                            player->argument = DOWN;
                            break;
                        case LEFT:
                            player->command = MOVE;
                            player->argument = LEFT;
                            break;
                        case RIGHT:
                            player->command = MOVE;
                            player->argument = RIGHT;
                            break;
                        default:
                            break;
                    }
                    break;
                case WAIT:
                    break;
                case QUIT:
                    disconnect_socket(player->socket);
                    deletePlayer(player);
                    connected = 0;
                    break;
                default:
                    break;
            }
            send_map(player);
            send_data(player);
            buffer[0] = WAIT;
            refresh();
        }

    }
    pthread_exit(NULL);
}

void *beasts_connection_handler(void *arg) {
    int socket = *(int *) arg;
    int connected = 1;
    char buffer[1024];

    pthread_exit(NULL);
}

void *listen_for_clients(void *arg) {
    int client_socket = accept(server.socket, NULL, NULL);

    send(client_socket, server.message, sizeof(server.message), 0);
    char c;
    recv(client_socket, &c, sizeof(c), 0);
    if (c == '1') { // this is player's client

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
        pthread_exit(NULL);

    }
    else { // this is beast client
        // TODO handle connection from beasts manager
        if (server.beast_client == -1) {
            server.beast_client = client_socket;

            pthread_t handler;
            pthread_create(&handler, NULL, beasts_connection_handler, (void *) &server.beast_client);
            pthread_exit(NULL);
        } else {
            disconnect_socket(client_socket);
            pthread_exit(NULL);
        }
    }

}

// DATA TRANSFER ///////////////////////////////////////////////////////////////////////////////////////////////////////


void send_map(struct Player *player) {
    char map[PLAYER_POV][PLAYER_POV];
    for (int row = 0; row < PLAYER_POV; row++) {
        for (int col = 0; col < PLAYER_POV; col++) {
            if (is_position_valid(row - (PLAYER_POV / 2) + player->pos_row, col - (PLAYER_POV / 2) + player->pos_col)) {
                map[row][col] = world.map[row - (PLAYER_POV / 2) + player->pos_row][col - (PLAYER_POV / 2) + player->pos_col];
            }
            else {
                map[row][col] = 'X';
            }
        }
    }
    if (server.up) send(player->socket, map, sizeof(map), 0);
}

void send_data(struct Player *player) {
    struct data_transfer data;

    data.pos_X = player->pos_row;
    data.pos_Y = player->pos_col;
    data.coins_carried = player->coins_carried;
    data.coins_saved = player->coins_saved;
    data.deaths = player->deaths;
    data.round = server.round;
    data.camp_x = world.campfire_row;
    data.camp_y = world.campfire_col;

    if (server.up) send(player->socket, (void *) &data, sizeof(data), 0);
}


// GAME LOGIC //////////////////////////////////////////////////////////////////////////////////////////////////////////

void run_orders(struct Player *player) {
    if (is_open(player->socket)) {
        switch (player->command) {
            case MOVE:
                movePlayer(player, player->argument);
                break;
            case QUIT:
                disconnect_socket(player->socket);
                deletePlayer(player);
            default:
                break;
        }
    }
    player->command = WAIT;
}

int is_position_valid(int row, int col) {
    if ((row < 0) || (col < 0) || (row >= MAP_HEIGHT) || (col >= MAP_WIDTH)) return 0;
    return 1;
}

void end_game() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (world.players[i] != NULL) {
            deletePlayer(world.players[i]);
        }
    }
    server.up = 0;
}

void *key_listener(void *arg) {
    noecho();

    while (server.up) {
        unsigned char key = getch();
        switch (key) {
            case 'r':
            case 'R':
                refresh_screen();
                break;
            case 'Q':
            case 'q':
                key = '0';
                server.up = 0;
                end_game();
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
                // TODO add beasts

                break;
            default:
                key = '0';
        }
    }

    pthread_exit(NULL);
}

void *game(void *arg) {
    init_ui();
    noecho();

    //int beast_handle = system("./beasts");
    pthread_create(&keyListenerThread, NULL, key_listener, NULL);

    while (server.up) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (world.players[i] != NULL) {
                run_orders(world.players[i]);
            }
        }
        update_info();
        refresh();
        usleep(TURN_TIME);
        server.round++;
    }

    pthread_exit(NULL);
}

int main() { // server application
    srand(time(0));
    init_server();
    load_map();

    pthread_create(&playingThread, NULL, game, NULL);

    while (server.up) {
        usleep(TURN_TIME);
        pthread_create(&listeningThread, NULL, listen_for_clients, NULL);
        pthread_join(listeningThread, NULL);
    }
    return 0;
}
