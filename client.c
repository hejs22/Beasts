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
    char map[7][7];
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
}

void estabilish_connection() {
    while (1) {
        // spinlock TODO
        int connection_status = connect(this_client.network_socket, (struct sockaddr *) &this_client.server_address, sizeof(this_client.server_address));
        if (connection_status >= 0) break;
        printf("Waiting for server initialization...\n");
        usleep(TURN_TIME);
    }

    this_client.connected = 1;
    char server_response[256];
    recv(this_client.network_socket, &server_response, sizeof(server_response), 0);

    printw("%s\n", server_response);
}


void *key_listener(void *arg) {
    cbreak();
    keypad(stdscr, NULL);

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

void print_tile_client(char c, int row, int col) {
    switch (c) {
        case '#':
            attron(COLOR_PAIR(2));
            mvprintw(1 + row, 3 + col, "#");
            attroff(COLOR_PAIR(2));
            break;
        case 'X':
            attron(COLOR_PAIR(1));
            mvprintw(1 + row, 3 + col, "X");
            attroff(COLOR_PAIR(1));
            break;
        case 'O':
            attron(COLOR_PAIR(4));
            attron(A_BOLD);
            mvprintw(1 + row, 3 + col, "A");
            attroff(A_BOLD);
            attroff(COLOR_PAIR(4));
            break;
        case '1':
        case '2':
        case '3':
        case '4':
            attron(COLOR_PAIR(5));
            attron(A_BOLD);
            mvprintw(1 + row, 3 + col, "%c", c);
            attroff(A_BOLD);
            attroff(COLOR_PAIR(5));
            break;
        case 'c':
            attron(COLOR_PAIR(3));
            mvprintw(1 + row, 3 + col, "c");
            attroff(COLOR_PAIR(3));
            break;
        case 't':
            attron(COLOR_PAIR(3));
            mvprintw(1 + row, 3 + col, "t");
            attroff(COLOR_PAIR(3));
            break;
        case 'T':
            attron(COLOR_PAIR(3));
            mvprintw(1 + row, 3 + col, "T");
            attroff(COLOR_PAIR(3));
            break;
        case ' ':
            attron(COLOR_PAIR(6));
            mvprintw(1 + row, 3 + col, " ");
            attroff(COLOR_PAIR(6));
        default:
            break;
    }
}

void print_map_client() {
    clear();
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 7; col++) {
            print_tile_client(this_client.map[row][col], row + 1, col + 3);
        }
    }
    move(0, 0);
    refresh();
}

int main() { // client application

    initscr();
    noecho();

    if (has_colors() == TRUE) {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_WHITE); // White for walls
        init_pair(2, COLOR_GREEN, COLOR_BLACK); // Green for bushes
        init_pair(3, COLOR_YELLOW, COLOR_BLACK); // Yellow for treasures
        init_pair(4, COLOR_RED, COLOR_BLACK); // Red for beasts and campsites
        init_pair(5, COLOR_CYAN, COLOR_BLUE); // Blue for players
        init_pair(6, COLOR_BLACK, COLOR_BLACK); // Black for empty spaces
    }

    printw("TYPE c FOR CPU, h FOR HUMAN: ");
    int c = getch();
    if (c == 'c') this_client.playertype = CPU;
    if (c == 'h') this_client.playertype = HUMAN;

    client_configure();
    estabilish_connection();



    // send data to server
    send(this_client.network_socket, this_client.buffer, sizeof(this_client.buffer), 0);


    if (this_client.playertype == CPU) {
        this_client.request[0] = MOVE;
        srand(time(0));
        for (int i = 0; i < 1000; i++) {
            this_client.request[1] = rand() % 4;
            send(this_client.network_socket, this_client.request, sizeof(this_client.request), 0);
            recv(this_client.network_socket, this_client.map, sizeof(this_client.map), 0);
            print_map_client();
            usleep(TURN_TIME);
        }
    } else if (this_client.playertype == HUMAN) {
        pthread_t keyboardListener;
        pthread_create(&keyboardListener, NULL, key_listener, NULL);

        for (int i = 0; i < 1000; i++) {
            send(this_client.network_socket, this_client.request, sizeof(this_client.request), 0);
            recv(this_client.network_socket, this_client.map, sizeof(this_client.map), 0);
            print_map_client();
            this_client.request[0] = WAIT;
            usleep(TURN_TIME);
        }
    }

    close(this_client.network_socket);
    return 0;
}