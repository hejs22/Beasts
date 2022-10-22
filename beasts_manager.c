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

    char map[MAP_HEIGHT][MAP_WIDTH];
    int pos_row[MAX_BEASTS];
    int pos_col[MAX_BEASTS];
} this_client;

struct beasts_data_transfer {
    char map[MAP_HEIGHT][MAP_WIDTH];
    int pos_X[MAX_BEASTS];
    int pos_Y[MAX_BEASTS];
    enum COMMAND command;
};

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

struct player_data_transfer {
    char map[MAP_HEIGHT][MAP_WIDTH];
    int pos_X[MAX_BEASTS];
    int pos_Y[MAX_BEASTS];
};

void get_info() {
    struct player_data_transfer data;
    long bytes_received = recv(this_client.network_socket, (void *) &data, sizeof(data), 0);

    if (bytes_received <= 0) {
        leave_game();
    }

    memcpy(this_client.map, data.map, sizeof(data.map));
    for (int i = 0; i < this_client.amount_of_beasts; i++) {
        this_client.pos_row[i] = data.pos_X[i];
        this_client.pos_col[i] = data.pos_Y[i];
    }
}

// GAME LOGIC //////////////////////////////////////////////////////////////////////////////////////////////////////////

void *handle_beast(void *arg) {
    int beast_id = *(int *) arg;
    this_client.request[0 + beast_id * 3] = beast_id;
    this_client.request[1 + beast_id * 3] = MOVE;
    while (this_client.connected) {
        printf("%d\t", beast_id);
        this_client.request[2 + beast_id * 3] = rand() % 4;
        usleep(TURN_TIME);
    }
}

void handleSIGUSR1 () {
    if (this_client.amount_of_beasts < MAX_BEASTS) {
        pthread_t new;
        int beast_id = this_client.amount_of_beasts;
        pthread_create(&new, NULL, handle_beast, &beast_id);
        this_client.amount_of_beasts++;
    }
}

void beast_manager() {
    this_client.request[0] = MOVE;
    srand(time(0));
    while (this_client.connected) {
        struct beasts_data_transfer data;
        long bytes_received = recv(this_client.network_socket, &data, sizeof(data), 0);
        if (bytes_received > 0) {
            usleep(TURN_TIME);
            send(this_client.network_socket, this_client.request, sizeof(this_client.request), 0);
            printf("Orders sent\n");
        }
    }
    leave_game();
}

int main() { // client application
    client_configure();
    estabilish_connection();

    // send data to server
    int pid = getpid();
    sprintf(this_client.buffer, "%d", pid);
    send(this_client.network_socket, this_client.buffer, sizeof(this_client.buffer), 0);

    signal(SIGUSR1, handleSIGUSR1);
    beast_manager();

    usleep(1000000);
    return 0;
}