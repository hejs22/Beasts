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


// DATA STRUCTURES //////////////////////////////////////////////////////////////////////////////////////////////////

struct client_socket {
    int network_socket;
    struct sockaddr_in server_address;
    char buffer[1024];
    char request[3 * MAX_BEASTS];
    int connected;
    int amount_of_beasts;
    pthread_t server_pid;

    char maps[MAX_BEASTS][BEAST_POV][BEAST_POV];
} this_client;

pthread_mutex_t lock;

// CONNECTION MANAGEMENT //////////////////////////////////////////////////////////////////////////////////////////////

void leaveGame() {
    close(this_client.network_socket);
    this_client.connected = 0;
    clear();
    attron(A_BOLD);
    printf("Connection ended.");
    attroff(A_BOLD);
    getch();
}

void clientConfigure() {
    // create a socket
    this_client.network_socket = socket(AF_INET, SOCK_STREAM, 0);  // socket is created

    // create address structure, specify address for the socket
    this_client.server_address.sin_family = AF_INET;
    this_client.server_address.sin_port = htons(9002);
    this_client.server_address.sin_addr.s_addr = INADDR_ANY;
}

void estabilishConnection() {
    int i = 0;
    while (i < 10) {
        int connection_status = connect(this_client.network_socket, (struct sockaddr *) &this_client.server_address,
                                        sizeof(this_client.server_address));
        if (connection_status >= 0) break;
        printf("Waiting for server initialization...\n");
        usleep(TURN_TIME * 5);
        i++;
    }

    this_client.connected = 1;

    struct type_and_pid pid;
    pid.pid = getpid();
    pid.type = '0';

    long res = recv(this_client.network_socket, &this_client.server_pid, sizeof(this_client.server_pid), 0);
    if (res <= 0) {
        leaveGame();
    }
    send(this_client.network_socket, &pid, sizeof(pid), 0);

    clear();
    printf("Connection estabilished. ");
    this_client.amount_of_beasts = 0;
}

// DATA TRANSFER /////////////////////////////////////////////////////////////////////////////////////////////////////

void getInfo() {
    long bytes_received = recv(this_client.network_socket, this_client.maps, sizeof(this_client.maps), 0);
    if (bytes_received <= 0) {
        leaveGame();
    }
}

// GAME LOGIC //////////////////////////////////////////////////////////////////////////////////////////////////////////

int isNotObstacle(char c) {
    if ((c == 'A') || (c == 'X') || (c == '*')) {
        return 0;
    }
    return 1;
}

int isPlayer(char c) {
    if ((c >= '1') && (c <= '4')) return 1;
    return 0;
}

struct point scanArea(char map[BEAST_POV][BEAST_POV]) {
    // returns location of nearest player

    int row = BEAST_POV / 2;
    int col = BEAST_POV / 2;

    struct point p;
    p.x = 0;
    p.y = 0;
    int min = 999;
    for (int x = -(BEAST_POV / 2); x < BEAST_POV / 2; x++) {
        for (int y = -(BEAST_POV / 2); y < BEAST_POV / 2; y++) {
            if (isPlayer(map[row + x][col + y])) {
                int distance = abs(x) + abs(y);
                if (distance < min) {
                    min = distance;
                    p.x = x;
                    p.y = y;
                }
            }
        }
    }

    return p;
}

enum DIRECTION findPath(char map[BEAST_POV][BEAST_POV]) {
    // finds best direction in which this beast should go

    struct point player_location = scanArea(map);

    enum DIRECTION horizontal_dir = STOP;
    enum DIRECTION vertical_dir = STOP;

    if ((player_location.x > 0) && (isNotObstacle(map[BEAST_POV / 2 + 1][BEAST_POV / 2]))) {
        vertical_dir = DOWN;
    } else if ((player_location.x < 0) && (isNotObstacle(map[BEAST_POV / 2 - 1][BEAST_POV / 2]))) {
        vertical_dir = UP;
    }

    if ((player_location.y > 0) && (isNotObstacle(map[BEAST_POV / 2][BEAST_POV / 2 + 1]))) {
        horizontal_dir = RIGHT;
    } else if ((player_location.y < 0) && (isNotObstacle(map[BEAST_POV / 2][BEAST_POV / 2 - 1]))) {
        horizontal_dir = LEFT;
    }


    if ((horizontal_dir == STOP) && (vertical_dir != STOP)) {
        return vertical_dir;
    }

    if ((horizontal_dir != STOP) && (vertical_dir == STOP)) {
        return horizontal_dir;
    }

    if ((horizontal_dir != STOP) && (vertical_dir != STOP)) {
        if ((rand() % 2) == 1) {
            return horizontal_dir;
        } else {
            return vertical_dir;
        }
    }

    return rand() % 5;
}

void *handleBeast(void *arg) {
    // reads map and decides where to go
    int beast_id = *(int *) arg;
    char request[3];
    while (this_client.connected) {
        pthread_mutex_lock(&lock);

        request[0] = beast_id;
        request[1] = MOVE;

        char map[BEAST_POV][BEAST_POV];
        memcpy(map, this_client.maps[beast_id], sizeof(this_client.maps[beast_id]));

        request[2] = findPath(map);
        send(this_client.network_socket, request, sizeof(request), 0);
        getInfo();

        printf("Beast %d is alive, direction: %d\n", beast_id, request[2]);
        pthread_mutex_unlock(&lock);

        usleep(TURN_TIME);
    }
}

void handleSpawn() {
    // creates new thread for new beast
    if (this_client.amount_of_beasts < MAX_BEASTS) {
        pthread_t new;
        int beast_id = this_client.amount_of_beasts;
        this_client.amount_of_beasts++;
        pthread_create(&new, NULL, handleBeast, &beast_id);
    }
}

void beastManager() {
    while (this_client.connected) {
        // gets map every TURN_TIME microseconds
        getInfo();
        usleep(TURN_TIME);
    }
    leaveGame();
}

int main() { // client application
    clientConfigure();
    estabilishConnection();
    srand(time(0));

    signal(SIGUSR1, &handleSpawn);
    beastManager();

    close(this_client.network_socket);
    return 0;
}