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
    char request[3*MAX_BEASTS];
    int connected;
    int amount_of_beasts;
    pthread_t server_pid;

    char maps[MAX_BEASTS][BEAST_POV][BEAST_POV];
} this_client;

pthread_mutex_t lock;

// CONNECTION MANAGEMENT //////////////////////////////////////////////////////////////////////////////////////////////

void client_configure() {
    // create a socket
    this_client.network_socket = socket(AF_INET, SOCK_STREAM, 0);  // socket is created

    // create address structure, specify address for the socket
    this_client.server_address.sin_family = AF_INET;
    this_client.server_address.sin_port = htons(9002);
    this_client.server_address.sin_addr.s_addr = INADDR_ANY;
}

void estabilish_connection() {
    int i = 0;
    while (i < 10) {
        int connection_status = connect(this_client.network_socket, (struct sockaddr *) &this_client.server_address, sizeof(this_client.server_address));
        if (connection_status >= 0) break;
        printf("Waiting for server initialization...\n");
        usleep(TURN_TIME * 5);
        i++;
    }

    this_client.connected = 1;
    char server_response[256];

    struct type_and_pid pid;
    pid.pid = getpid();
    pid.type = '0';

    recv(this_client.network_socket, &server_response, sizeof(server_response), 0);
    send(this_client.network_socket, &pid, sizeof(pid), 0);

    clear();
    printf("Connection estabilished. ");
    this_client.server_pid = atoi(server_response);
    this_client.amount_of_beasts = 0;
}

void leave_game() {
    close(this_client.network_socket);
    this_client.connected = 0;
    printf("\n----Game ended.----\n");
    getch();
}

// DATA TRANSFER /////////////////////////////////////////////////////////////////////////////////////////////////////

void get_info() {
    long bytes_received = recv(this_client.network_socket, this_client.maps, sizeof(this_client.maps), 0);

    if (bytes_received <= 0) {
        leave_game();
    }
}

// GAME LOGIC //////////////////////////////////////////////////////////////////////////////////////////////////////////

int is_not_obstacle(char c) {
    if ((c == 'A') || (c == 'X') || (c == '*')) {
        return 0;
    }
    return 1;
}

int is_player(char c) {
    if ((c >= '1') && (c <= '4')) return 1;
    return 0;
}

struct point scan_area(char map[BEAST_POV][BEAST_POV]) {
    // returns location of nearest player

    int row = BEAST_POV / 2;
    int col = BEAST_POV / 2;

    struct point p;
    p.x = 0; p.y = 0;
    int min = 999;
    for (int x = -(BEAST_POV / 2); x < BEAST_POV / 2; x++) {
        for (int y = -(BEAST_POV / 2); y < BEAST_POV / 2; y++) {
            if (is_player(map[row + x][col + y])) {
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

enum DIRECTION find_path(char map[BEAST_POV][BEAST_POV]) {
    // finds best direction in which this beast should go

    struct point player_location = scan_area(map);

    enum DIRECTION horizontal_dir = STOP, vertical_dir = STOP;

    if ((player_location.x > 0) && (is_not_obstacle(map[BEAST_POV/2 + 1][BEAST_POV/2]))) {
        vertical_dir = DOWN;
    } else if ((player_location.x < 0) && (is_not_obstacle(map[BEAST_POV/2 - 1][BEAST_POV/2]))) {
        vertical_dir = UP;
    }

    if ((player_location.y > 0) && (is_not_obstacle(map[BEAST_POV/2][BEAST_POV/2 + 1]))) {
        horizontal_dir = RIGHT;
    } else if ((player_location.y < 0) && (is_not_obstacle(map[BEAST_POV/2][BEAST_POV/2 - 1]))) {
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

    return rand() % 4;
}

void *handle_beast(void *arg) {
    // reads map and decides where to go
    int beast_id = *(int *) arg;
    char request[3];
    while (this_client.connected) {
        pthread_mutex_lock(&lock);

        request[0] = beast_id;
        request[1] = MOVE;

        char map[BEAST_POV][BEAST_POV];
        memcpy(map, this_client.maps[beast_id], sizeof(this_client.maps[beast_id]));

        request[2] = find_path(map);
        send(this_client.network_socket, request, sizeof(request), 0);
        pthread_mutex_unlock(&lock);

        usleep(TURN_TIME);
    }
}

void handleSpawn () {
    // creates new thread for new beast
    if (this_client.amount_of_beasts < MAX_BEASTS) {
        pthread_t new;
        int beast_id = this_client.amount_of_beasts;
        pthread_create(&new, NULL, handle_beast, &beast_id);
        this_client.amount_of_beasts++;
    }
}

void beast_manager() {
    while (this_client.connected) {
        // gets map every TURN_TIME microseconds
        get_info();
        usleep(TURN_TIME);
    }
    leave_game();
}

int main() { // client application
    client_configure();
    estabilish_connection();
    srand(time(0));

    signal(SIGUSR1, handleSpawn);
    beast_manager();
    return 0;
}