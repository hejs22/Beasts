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
    printw("Entered into key listener");
    while (this_client.connected) {
        int key = getch();

        switch (key) {
            case 'Q':
            case 'q':
                this_client.connected = 0;
                // tell server to kill player
                break;
            case 'w':
                this_client.request[0] = MOVE;
                this_client.request[1] = UP;
                // tell server to go up
                break;
            case 'a':
                this_client.request[0] = MOVE;
                this_client.request[1] = LEFT;
                // tell server to go left
                break;
            case 's':
                this_client.request[0] = MOVE;
                this_client.request[1] = DOWN;
                // etc
                break;
            case 'd':
                this_client.request[0] = MOVE;
                this_client.request[1] = RIGHT;
                // etc
                break;
            default:
                break;
        }
    }

    pthread_exit(NULL);
}

int main() { // client application

    client_configure();
    estabilish_connection();

    initscr();
    noecho();

    printw("TYPE c FOR CPU, h FOR HUMAN: ");
    int c = getch();
    if (c == 'c') this_client.playertype = CPU;
    if (c == 'h') this_client.playertype = HUMAN;

    // send data to server
    send(this_client.network_socket, this_client.buffer, sizeof(this_client.buffer), 0);

    time_t rawtime;
    struct tm *timeinfo;


    if (this_client.playertype == CPU) {
        this_client.request[0] = MOVE;
        srand(time(0));
        for (int i = 0; i < 1000; i++) {
            this_client.request[1] = rand() % 4;
            send(this_client.network_socket, this_client.request, sizeof(this_client.request), 0);
            recv(this_client.network_socket, this_client.buffer, sizeof(this_client.buffer), 0);

            time(&rawtime);
            timeinfo = localtime (&rawtime);

            printw("%s - %s\n", this_client.buffer, asctime(timeinfo));
            usleep(TURN_TIME);
        }
    } else if (this_client.playertype == HUMAN) {
        pthread_t keyboardListener;
        pthread_create(&keyboardListener, NULL, key_listener, NULL);

        for (int i = 0; i < 1000; i++) {
            send(this_client.network_socket, this_client.request, sizeof(this_client.request), 0);
            recv(this_client.network_socket, this_client.buffer, sizeof(this_client.buffer), 0);

            time(&rawtime);
            timeinfo = localtime (&rawtime);

            printw("%s - %s\n", this_client.buffer, asctime(timeinfo));
            this_client.request[0] = WAIT;
            usleep(TURN_TIME);
        }
    }

    close(this_client.network_socket);
    return 0;
}