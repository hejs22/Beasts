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

struct client_socket {
    int network_socket;
    struct sockaddr_in server_address;
    char buffer[1024];
    char map[22]; // information about area around player 3 + 5 + 5 + 5 + 3 + 1
    char request[2];
    int connected;
    enum PLAYERTYPE playertype;
} this_client;

void client_configure() {
    // create a socket
    this_client.network_socket = socket(AF_INET, SOCK_STREAM, 0);  // socket is created

    // create address structure, specify address for the socket
    this_client.server_address.sin_family = AF_INET;
    this_client.server_address.sin_port = htons(9002);
    this_client.server_address.sin_addr.s_addr = INADDR_ANY;

    this_client.playertype = CPU; // for testing set it to CPU TODO
}

void estabilish_connection() {
    while (1) {
        int connection_status = connect(this_client.network_socket, (struct sockaddr *) &this_client.server_address, sizeof(this_client.server_address));
        if (connection_status >= 0) break;
        printf("Waiting for server initialization...\n");
        usleep(TURN_TIME);
    }

    this_client.connected = 1;
    char server_response[256];
    recv(this_client.network_socket, &server_response, sizeof(server_response), 0);

    printf("%s\n", server_response);
}


void *key_listener(void *arg) {
    noecho();
    //printw("Entered into key listener");
    while (this_client.connected) {
        unsigned char key = getch();

        switch (key) {
            case 'Q':
            case 'q':
                this_client.connected = 0;
                // tell server to kill player
                break;
            case KEY_UP:
                // tell server to go up
                break;
            case KEY_LEFT:
                // tell server to go left
                break;
            case KEY_DOWN:
                // etc
                break;
            case KEY_RIGHT:
                // etc
                break;
        }
    }

    pthread_exit(NULL);
}

int main() { // client application

    client_configure();
    estabilish_connection();
    if (this_client.playertype == HUMAN) {
        // run keyboard listener
    }


    // send data to server
    send(this_client.network_socket, this_client.buffer, sizeof(this_client.buffer), 0);

    time_t rawtime;
    struct tm *timeinfo;

    this_client.request[0] = MOVE;
    srand(time(0));
    for (int i = 0; i < 1000; i++) {
        this_client.request[1] = rand() % 4;
        send(this_client.network_socket, this_client.request, sizeof(this_client.request), 0);
        recv(this_client.network_socket, this_client.buffer, sizeof(this_client.buffer), 0);

        time(&rawtime);
        timeinfo = localtime (&rawtime);

        printf("%s - %s\n", this_client.buffer, asctime(timeinfo));
        usleep(TURN_TIME);
    }

    close(this_client.network_socket);
    return 0;
}