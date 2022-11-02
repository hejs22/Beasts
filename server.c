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


/*
 * @ brief sets up server parameters
 * @ return -
 */
void initServer() {
    server.up = 0;
    server.number_of_clients = MAX_CLIENTS;

    server.socket = socket(AF_INET, SOCK_STREAM, 0);
    for (int i = 0; i < MAX_CLIENTS; i++) server.clients[i] = -1;
    server.beast_client = -1;

    server.address.sin_port = htons(9002);
    server.address.sin_family = AF_INET;
    server.address.sin_addr.s_addr = INADDR_ANY;

    bind(server.socket, (struct sockaddr *) &(server.address), sizeof(server.address));

    if (listen(server.socket, MAX_CLIENTS) == 0) printf("Listening...\n");
    else printf("Error while setting up listen().\n");

    server.pid = getpid();
    server.round = 0;
    world.active_players = 0;
    world.active_beasts = 0;
    server.up = 1;
};

/*
 * @ brief inits screen and colors, prints starting info
 * @ return -
 */
void initUi() {
    initscr();

    // create color pairs used in game
    if (has_colors() == TRUE) {
        start_color();
        init_pair(FIRST_PLAYER, COLOR_WHITE, COLOR_BLUE); // Blue for player 1
        init_pair(SECOND_PLAYER, COLOR_WHITE, COLOR_CYAN); // Cyan for player 2
        init_pair(THIRD_PLAYER, COLOR_WHITE, COLOR_MAGENTA); // Magenta for player 3
        init_pair(FOURTH_PLAYER, COLOR_WHITE, COLOR_RED); // Red for player 4

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

    printMap();
    printInitialObjects();
    printInfo();
    refresh();
}

/*
 * @ brief checks if socket is still open
 * @ param socket that needs to be checked
 * @ return 1 if connection closed, 0 if socket is active
 */
int isOpen(int socket) {
    // check if socket is still empty
    char c;
    long res = recv(socket, &c, 1, MSG_PEEK | MSG_DONTWAIT);
    if (res != 0) return 1;
    return 0;
}

/*
 * @ brief deletes socket from sockets array and closes it
 * @ param socket that needs to be deleted
 * @ return -
 */
void disconnectSocket(int socket) {
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

/*
 * @ brief creates new player and saves it to array
 * @ param socket that player is connected to
 * @ param process id of client
 * @ return pointer to player struct or NULL if all slots are taken
 */
struct Player *addPlayerToList(int socket, unsigned long pid) {
    struct Player *player = NULL;

    if (isOpen(socket)) {
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

/*
 * @ brief disconnects socket of player and deletes it's structure
 * @ param socket that needs to be disonnected
 * @ return -
 */
void handleDisconnection(int socket) {
    // check which socket belongs to player, delete his structure and disconnect him
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if ((world.players[i] != NULL) && (world.players[i]->socket == socket)) {
            disconnectSocket(socket);
            deletePlayer(world.players[i]);
            break;
        }
    }
}

/*
 * @ brief handles server-client connection, sends map and receives requests
 * @ param address of structure with player's socket, process id
 * @ return NULL on exit
 */
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

    if (!player) {
        pthread_mutex_lock(&serverLock);
        disconnectSocket(socket);
        pthread_mutex_unlock(&serverLock);
        pthread_exit(NULL);
    }

    while (connected) {
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
            connected = 0;
            pthread_mutex_unlock(&serverLock);
            pthread_exit(NULL);
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
                pthread_mutex_unlock(&serverLock);
                pthread_exit(NULL);
                break;
            default:
                break;
        }

        // if player steps in bushes, he is immobilized by next turn
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

/*
 * @ brief sends map of beast's surroundings
 * @ param pointer to beast's structure
 * @ return -
 */
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

/*
 * @ brief handles server-beasts connection, sends map and receives requests
 * @ param beasts manager socket
 * @ return NULL on exit
 */
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

/*
 * @ brief handles newly connected client and proceeds to create new thread handling its connection
 * @ param pointer to struct that consists of process id, socket and client's type
 * @ return NULL
 */
void *listenForClients(void *arg) {
    struct type_and_pid pid = *(struct type_and_pid *) arg;

    if (pid.type == '1') {

        int flag = 1;
        pthread_mutex_lock(&serverLock);
        // check for empty client slots
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
            disconnectSocket(pid.socket);
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
            disconnectSocket(pid.socket);
        }
        pthread_exit(NULL);
    }
}

/*
 * @ brief sends pack of player's info - surroundings, coordinates, coins etc.
 * @ param pointer to player's struct
 * @ return -
 */
void sendData(const struct Player *player) {
    struct player_data_transfer data;
    for (int row = 0; row < PLAYER_POV; row++) {
        for (int col = 0; col < PLAYER_POV; col++) {
            if (isPositionValid(row - (PLAYER_POV / 2) + player->pos_row, col - (PLAYER_POV / 2) + player->pos_col)) {
                data.map[row][col] = world.map[row - (PLAYER_POV / 2) + player->pos_row][col - (PLAYER_POV / 2) + player->pos_col];
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

/*
 * @ brief checks player's queued request and handles it
 * @ param pointer to player's struct
 * @ return -
 */
void runOrders(struct Player *player) {
    // if socket is still open, handles queued request from clients
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

/*
 * @ brief checks beast's queued request and handles it
 * @ param pointer to beast's struct
 * @ return -
 */
void runOrdersBeast(struct Beast *beast) {
    if (beast->command == MOVE) {
        moveBeast(beast, beast->argument);
    }
}

/*
 * @ brief checks if desired position is within map bounds
 * @ param row of map array
 * @ param col of map array
 * @ return 0 if position is invalid, 1 if valid
 */
int isPositionValid(int row, int col) {
    // checks map boundaries
    if ((row < 0) || (col < 0) || (row >= MAP_HEIGHT) || (col >= MAP_WIDTH)) return 0;
    return 1;
}

/*
 * @ brief deallocates all resources, closes sockets and exits
 * @ return -
 */
void endGame() {
    // delete all connected players and close their sockets
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (world.players[i] != NULL) {
            deletePlayer(world.players[i]);
        }
    }

    // close remaining sockets and clean up
    server.up = 0;
    close(server.beast_client);
    close(server.socket);
    clear();

    mvprintw(2, 2, "Server closed, all players disconnected. Press any key to continue...");
    refresh();
    getch();
    endwin();
    exit(0);
}

/*
 * @ brief creates new beast structure and signals beast_manager that new beast should be taken care of
 * @ return -
 */
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

/*
 * @ brief listens for input and handles it
 * @ return NULL
 */
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

/*
 * @ brief main game loop, runs all connected entities requests and updates screen
 * @ return NULL
 */
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
        // accept new connection and wait for message
        // when it arrives, create new thread that handles it
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
