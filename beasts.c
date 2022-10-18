#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include "player.h"
#include "config.h"
#include <time.h>


// DATA STRUCTURES //////////////////////////////////////////////////////////////////////////////////////////////////

struct client_socket {
    int network_socket;
    struct sockaddr_in server_address;
    char buffer[1024];
    char map[PLAYER_POV][PLAYER_POV];
    char request[2];
    int connected;
    pthread_t server_pid;

    int pos_row;
    int pos_col;

} this_client;


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
    recv(this_client.network_socket, &server_response, sizeof(server_response), 0);
    send(this_client.network_socket, "0", sizeof("0"), 0);

    clear();
    printf("Connection estabilished. ");
    this_client.server_pid = atoi(server_response);
}

void leave_game() {
    close(this_client.network_socket);
    this_client.connected = 0;
    getch();
}

// DATA TRANSFER /////////////////////////////////////////////////////////////////////////////////////////////////////

void get_map() {
    long bytes_received = recv(this_client.network_socket, this_client.map, sizeof(this_client.map), 0);
    if (bytes_received <= 0) {
        leave_game();
    }
}

struct data_transfer {
    int pos_X;
    int pos_Y;
    int coins_saved;
    int coins_carried;
    int deaths;
    int round;
    int camp_x;
    int camp_y;
};

void get_info() {
    struct data_transfer data;
    long bytes_received = recv(this_client.network_socket, (void *) &data, sizeof(data), 0);

    if (bytes_received <= 0) {
        leave_game();
    } else {
        this_client.pos_row = data.pos_X;
        this_client.pos_col = data.pos_Y;
    }

}

// GAME LOGIC //////////////////////////////////////////////////////////////////////////////////////////////////////////


void ai_client() {
    this_client.request[1] = rand() % 4;
    send(this_client.network_socket, this_client.request, sizeof(this_client.request), 0);
    get_map();
    get_info();
    usleep(TURN_TIME);
}

void game_client() {
    this_client.request[0] = MOVE;
    srand(time(0));
    while (this_client.connected) {
        ai_client();
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

    game_client();
    return 0;
}