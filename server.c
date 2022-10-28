#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include <time.h>
#include <locale.h>
#include <signal.h>

#include "config.h"
#include "world.h"
#include "server.h"
#include "player.h"
#include "beasts.h"

pthread_t listeningThread;
pthread_t playingThread;
pthread_t keyListenerThread;
pthread_mutex_t serverLock = PTHREAD_MUTEX_INITIALIZER;

// SERVER INITIALIZATION ///////////////////////////////////////////////////////////////////////////////////////////////

void initServer() {
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

    if (listen(server.socket, MAX_CLIENTS) == 0) printf("Listening...\n");
    else printf("Error while setting up listen().\n");

    server.pid = getpid();
    server.round = 0;
    world.active_players = 0;
    world.active_beasts = 0;
    server.up = 1;
};

void initUi() {
    initscr();

    // create color pairs used in game
    if (has_colors() == TRUE) {
        start_color();
        init_pair(FIRST_PLAYER, COLOR_WHITE, COLOR_BLUE); // Blue for players
        init_pair(SECOND_PLAYER, COLOR_WHITE, COLOR_CYAN); // Blue for players
        init_pair(THIRD_PLAYER, COLOR_WHITE, COLOR_MAGENTA); // Blue for players
        init_pair(FOURTH_PLAYER, COLOR_WHITE, COLOR_RED); // Blue for players

        init_pair(WALL, COLOR_WHITE, COLOR_WHITE); // White for walls
        init_pair(BUSH, COLOR_GREEN, COLOR_BLACK); // Green for bushes
        init_pair(SMALL_TREASURE, COLOR_YELLOW, COLOR_BLACK); // Yellow for treasures
        init_pair(MEDIUM_TREASURE, COLOR_YELLOW, COLOR_BLACK); // Yellow for treasures
        init_pair(BIG_TREASURE, COLOR_YELLOW, COLOR_BLACK); // Yellow for treasures
        init_pair(DROPPED_TREASURE, COLOR_YELLOW, COLOR_BLACK); // Yellow for treasures

        init_pair(BEAST_TILE, COLOR_RED, COLOR_BLACK); // Red for beasts and campsites
        init_pair(CAMPFIRE, COLOR_RED, COLOR_BLACK); // Red for beasts and campsites
        init_pair(EMPTY, COLOR_BLACK, COLOR_BLACK); // Black for empty spaces
    }

    // Printing server's view
    printMap();
    printInitialObjects();
    printInfo();
    refresh();
}

// CONNECTION MANAGEMENT ///////////////////////////////////////////////////////////////////////////////////////////////


int isOpen(int socket) {
    char c;
    long res = recv(socket, &c, 1, MSG_PEEK | MSG_DONTWAIT);
    if (res != 0) return 1;
    return 0;
}

void disconnectSocket(int socket) {
    // delete socket from sockets array and close it
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i] == socket) {
            server.clients[i] = -1;
            close(socket);
        }
    }

    if (socket == server.beast_client) {
        server.beast_client = -1;
        close(socket);
    }
}

struct Player *addPlayerToList(int socket, unsigned long pid) {
    struct Player *player = NULL;

    if (isOpen(socket)) {
        // check for empty slots, if any is found create player
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server.clients[i] == socket) {
                player = create_player(socket);
                player->pid = pid;
                player->avatar = i + '1';
                world.players[i] = player;
                world.active_players++;
            }
        }
    }

    return player;
}

void handleDisconnection(int socket) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if ((world.players[i] != NULL) && (world.players[i]->socket == socket)) {
            disconnectSocket(socket);
            deletePlayer(world.players[i]);
            break;
        }
    }
}

void *clientServerConnectionHandler(void *arg) {
    struct Player *player = NULL;
    struct type_and_pid client_info = *(struct type_and_pid *) arg;
    int socket = client_info.socket;
    unsigned long pid = client_info.pid;
    int connected = 1;
    char buffer[1024];

    pthread_mutex_lock(&serverLock);
    player = addPlayerToList(socket, pid);
    pthread_mutex_unlock(&serverLock);

    // if all slots are taken disconnect and exit
    if (!player) {
        pthread_mutex_lock(&serverLock);
        disconnectSocket(socket);
        pthread_mutex_unlock(&serverLock);
        pthread_exit(NULL);
    }

    while (connected) {
        // if client isn't connected, disconnect his socket and free his Player structure

        pthread_mutex_lock(&serverLock);
        if (!isOpen(socket)) {
            handleDisconnection(socket);
            connected = 0;
            pthread_mutex_unlock(&serverLock);
            break;
        }

        sendData(player);
        pthread_mutex_unlock(&serverLock);

        long bytes_received = recv(socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            pthread_mutex_lock(&serverLock);
            handleDisconnection(socket);
            pthread_mutex_unlock(&serverLock);
        }

        enum COMMAND request = (enum COMMAND) buffer[0];
        int parameter = (int) buffer[1];

        pthread_mutex_lock(&serverLock);
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
            case QUIT:
                disconnectSocket(socket);
                deletePlayer(player);
                connected = 0;
                break;
            default:
                break;
        }

        if (player->bush) {
            player->command = WAIT;
            player->bush = 0;
        }

        sendData(player);
        pthread_mutex_unlock(&serverLock);
        recv(socket, buffer, sizeof(buffer), 0);

    }
    pthread_exit(NULL);
}

void sendBeastData(const struct Beast *beast) {
    char map[BEAST_POV][BEAST_POV];
    for (int row = 0; row < BEAST_POV; row++) {
        for (int col = 0; col < BEAST_POV; col++) {
            if (isPositionValid(row - (BEAST_POV / 2) + beast->pos_row,
                                col - (BEAST_POV / 2) + beast->pos_col)) {
                map[row][col] = world.map[row - (BEAST_POV / 2) + beast->pos_row][col - (BEAST_POV / 2) +
                                                                                            beast->pos_col];
            } else {
                map[row][col] = WALL;
            }
        }
    }
    send(server.beast_client, map, sizeof(map), 0);
}


void *beastsConnectionHandler(void *arg) {
    int socket = *(int *) arg;
    int connected = 1;
    char buffer[1024];

    while (connected) {

        pthread_mutex_lock(&serverLock);
        int number_of_beasts = world.active_beasts;
        for (int i = 0; i < number_of_beasts; i++) {
            sendBeastData(world.beasts[i]);
        }
        pthread_mutex_unlock(&serverLock);

        for (int i = 0; i < number_of_beasts; i++) {
            long bytes_received = recv(socket, buffer, sizeof(buffer), 0);

            if (bytes_received <= 0) {
                pthread_mutex_lock(&serverLock);
                disconnectSocket(server.beast_client);
                connected = 0;
                pthread_mutex_unlock(&serverLock);
                break;
            }

            enum COMMAND request = (enum COMMAND) buffer[1];
            int parameter = (int) buffer[2];

            pthread_mutex_lock(&serverLock);
            switch (request) {
                case MOVE:
                    world.beasts[buffer[0]]->command = MOVE;
                    switch (parameter) {
                        case UP:
                            world.beasts[buffer[0]]->argument = UP;
                            break;
                        case DOWN:
                            world.beasts[buffer[0]]->argument = DOWN;
                            break;
                        case LEFT:
                            world.beasts[buffer[0]]->argument = LEFT;
                            break;
                        case RIGHT:
                            world.beasts[buffer[0]]->argument = RIGHT;
                            break;
                        default:
                            world.beasts[buffer[0]]->argument = STOP;
                            break;
                    }
                    break;
                default:
                    break;
            }

            if (world.beasts[buffer[0]]->bush) {
                world.beasts[buffer[0]]->command = WAIT;
                world.beasts[buffer[0]]->bush = 0;
            }

            for (int j = 0; j < number_of_beasts; j++) {
                sendBeastData(world.beasts[j]);
            }
            pthread_mutex_unlock(&serverLock);
        }
    }

    pthread_exit(NULL);
}

void *listenForClients(void *arg) {
    struct type_and_pid pid = *(struct type_and_pid *) arg;

    if (pid.type == '1') { // this is player's client

        // check for empty client slots
        int flag = 1;
        pthread_mutex_lock(&serverLock);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server.clients[i] == -1) {
                server.clients[i] = pid.socket;
                flag = 0;
                break;
            }
        }
        pthread_mutex_unlock(&serverLock);

        // if all slots are taken, disconnect socket and exit
        if (flag) {
            send(pid.socket, "ER", 2, 0);
            pthread_mutex_lock(&serverLock);
            disconnectSocket(pid.socket);
            pthread_mutex_unlock(&serverLock);
            pthread_exit(NULL);
        }

        // create a handler thread for each connection and remember client's socket
        send(pid.socket, "OK", 2, 0);
        pthread_t handler;
        pthread_create(&handler, NULL, clientServerConnectionHandler, arg);
        pthread_exit(NULL);

    } else {
        // this is beast client
        pthread_mutex_lock(&serverLock);
        int socket = server.beast_client;
        pthread_mutex_unlock(&serverLock);

        if (socket == -1) {
            pthread_mutex_lock(&serverLock);
            server.beast_client = pid.socket;
            server.beasts_pid = pid.pid;
            pthread_mutex_unlock(&serverLock);
            pthread_t handler;
            pthread_create(&handler, NULL, beastsConnectionHandler, (void *) &server.beast_client);
        } else {
            pthread_mutex_lock(&serverLock);
            disconnectSocket(pid.socket);
            pthread_mutex_unlock(&serverLock);
        }
        pthread_exit(NULL);

    }

}

// DATA TRANSFER ///////////////////////////////////////////////////////////////////////////////////////////////////////


void sendData(const struct Player *player) {
    struct player_data_transfer data;

    for (int row = 0; row < PLAYER_POV; row++) {
        for (int col = 0; col < PLAYER_POV; col++) {
            if (isPositionValid(row - (PLAYER_POV / 2) + player->pos_row, col - (PLAYER_POV / 2) + player->pos_col)) {
                data.map[row][col] = world.map[row - (PLAYER_POV / 2) + player->pos_row][col - (PLAYER_POV / 2) +
                                                                                         player->pos_col];
            } else {
                data.map[row][col] = WALL;
            }
        }
    }

    data.pos_X = player->pos_row;
    data.pos_Y = player->pos_col;
    data.coins_carried = player->coins_carried;
    data.coins_saved = player->coins_saved;
    data.deaths = player->deaths;

    data.round = server.round;

    if (server.up) send(player->socket, (void *) &data, sizeof(data), 0);
}


// GAME LOGIC //////////////////////////////////////////////////////////////////////////////////////////////////////////

void runOrders(struct Player *player) {
    if (isOpen(player->socket)) {
        switch (player->command) {
            case MOVE:
                movePlayer(player, player->argument);
                break;
            case QUIT:
                disconnectSocket(player->socket);
                deletePlayer(player);
            default:
                break;
        }
    }
    player->command = WAIT;
}

void runOrdersBeast(struct Beast *beast) {
    if (beast->command == MOVE) {
        moveBeast(beast, beast->argument);
    }
}

int isPositionValid(int row, int col) {
    if ((row < 0) || (col < 0) || (row >= MAP_HEIGHT) || (col >= MAP_WIDTH)) return 0;
    return 1;
}

void endGame() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (world.players[i] != NULL) {
            deletePlayer(world.players[i]);
        }
    }
    server.up = 0;

    clear();
    close(server.beast_client);
    close(server.socket);

    mvprintw(2, 2, "Server closed, all players disconnected. Press any key to continue...");
    refresh();
    getch();
    endwin();
    exit(0);
}

void spawnBeast() {
    if ((world.active_beasts < MAX_BEASTS) && (server.beast_client != -1)) {
        kill(server.beasts_pid, SIGUSR1);
        for (int i = 0; i < MAX_BEASTS; i++) {
            if (world.beasts[i] == NULL) {
                world.beasts[i] = createBeast();
                break;
            }
        }
        world.active_beasts++;
    }
}

void *keyListener(void *arg) {
    noecho();
    int connected = 1;

    while (connected) {
        unsigned char key = getch();
        pthread_mutex_lock(&serverLock);
        switch (key) {
            case 'r':
            case 'R':
                refreshScreen();
                break;
            case 'Q':
            case 'q':
                server.up = 0;
                endGame();
                break;
            case 'c':
                createObject(SMALL_TREASURE);
                break;
            case 't':
                createObject(MEDIUM_TREASURE);
                break;
            case 'T':
                createObject(BIG_TREASURE);
                break;
            case 'B':
            case 'b':
                spawnBeast();
                break;
            default:
                break;
        }
        connected = server.up;
        pthread_mutex_unlock(&serverLock);
    }

    pthread_exit(NULL);
}

void *game(void *arg) {
    initUi();
    noecho();

    int connected = 1;
    pthread_create(&keyListenerThread, NULL, keyListener, NULL);

    while (connected) {
        usleep(TURN_TIME);
        pthread_mutex_lock(&serverLock);
        connected = server.up;

        server.round++;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (world.players[i] != NULL) {
                runOrders(world.players[i]);
            }
        }
        for (int i = 0; i < MAX_BEASTS; i++) {
            if (world.beasts[i] != NULL) {
                runOrdersBeast(world.beasts[i]);
            }
        }

        if (server.up) {
            updateInfo();
            refresh();
        } else {
            endGame();
        }

        pthread_mutex_unlock(&serverLock);
    }
}

int main() {
    srand(time(0));

    initServer();
    loadMap();
    pthread_create(&playingThread, NULL, game, NULL);

    pthread_mutex_lock(&serverLock);
    int socket = server.socket;
    int connected = server.up;
    unsigned long pid = server.pid;
    pthread_mutex_unlock(&serverLock);

    while (connected) {
        struct type_and_pid client_info;
        int client_socket = accept(socket, NULL, NULL);
        send(client_socket, &pid, sizeof(pid), 0);
        recv(client_socket, &client_info, sizeof(client_info), 0);
        client_info.socket = client_socket;

        pthread_create(&listeningThread, NULL, listenForClients, &client_info);

        pthread_mutex_lock(&serverLock);
        connected = server.up;
        pthread_mutex_unlock(&serverLock);
    }

    pthread_join(playingThread, NULL);
    endGame();
    return 0;
}
