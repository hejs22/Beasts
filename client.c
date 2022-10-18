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
    char data[5];
    char request[2];
    int connected;
    pthread_t server_pid;
    enum PLAYERTYPE playertype;

    int pos_row;
    int pos_col;
    int coins_saved;
    int coins_carried;
    int deaths;
    int round;
    int camp_x;
    int camp_y;
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

    clear();
    mvprintw(0, 0, "Connection estabilished. ");
    this_client.server_pid = atoi(server_response);
}

void leave_game() {
    close(this_client.network_socket);
    this_client.connected = 0;
    clear();
    attron(A_BOLD);
    int score = this_client.coins_saved;
    mvprintw(2, 2, "Game over, your score: %d. Press any key to continue...", score);
    attroff(A_BOLD);
    getch();
}



// USER GRAPHICAL INTERFACE ///////////////////////////////////////////////////////////////////////////////////////////

void print_tile_client(char c, int row, int col) {
    switch (c) {
        case '#':
            attron(COLOR_PAIR(12));
            mvprintw(1 + row, 3 + col, "#");
            attroff(COLOR_PAIR(12));
            break;
        case 'X':
            attron(COLOR_PAIR(11));
            mvprintw(1 + row, 3 + col, "X");
            attroff(COLOR_PAIR(11));
            break;
        case 'A':
            attron(COLOR_PAIR(14));
            attron(A_BOLD);
            mvprintw(1 + row, 3 + col, "A");
            attroff(A_BOLD);
            attroff(COLOR_PAIR(14));
            break;
        case '1':
        case '2':
        case '3':
        case '4':
            attron(COLOR_PAIR(c - '0'));
            attron(A_BOLD);
            mvprintw(1 + row, 3 + col, "%c", c);
            attroff(A_BOLD);
            attroff(COLOR_PAIR(c - '0'));
            break;
        case 'c':
            attron(COLOR_PAIR(13));
            mvprintw(1 + row, 3 + col, "c");
            attroff(COLOR_PAIR(13));
            break;
        case 't':
            attron(COLOR_PAIR(13));
            mvprintw(1 + row, 3 + col, "t");
            attroff(COLOR_PAIR(13));
            break;
        case 'T':
            attron(COLOR_PAIR(13));
            mvprintw(1 + row, 3 + col, "T");
            attroff(COLOR_PAIR(13));
            break;
        case 'D':
            attron(COLOR_PAIR(13));
            mvprintw(1 + row, 3 + col, "D");
            attroff(COLOR_PAIR(13));
            break;
        case ' ':
            attron(COLOR_PAIR(15));
            mvprintw(1 + row, 3 + col, " ");
            attroff(COLOR_PAIR(15));
        default:
            break;
    }
}

void print_legend() {
    mvprintw(2, CLIENT_INFO_POS_X + PLAYER_POV + 10, "Server's PID:");
    mvprintw(3, CLIENT_INFO_POS_X + PLAYER_POV + 10, "Campsite's X/Y: ");
    mvprintw(4, CLIENT_INFO_POS_X + PLAYER_POV + 10, "Round number: ");

    mvprintw(6, CLIENT_INFO_POS_X + PLAYER_POV + 10, "Player's X/Y: ");
    mvprintw(7, CLIENT_INFO_POS_X + PLAYER_POV + 10, "Coins carried: ");
    mvprintw(8, CLIENT_INFO_POS_X + PLAYER_POV + 10, "Coins saved: ");
    mvprintw(9, CLIENT_INFO_POS_X + PLAYER_POV + 10, "Deaths: ");


    mvprintw(CLIENT_INFO_POS_Y + 1, CLIENT_INFO_POS_X, "Legend: ");

    mvprintw(CLIENT_INFO_POS_Y + 2, CLIENT_INFO_POS_X, "Players - ");
    attron(A_BOLD);
    attron(COLOR_PAIR(1));
    mvprintw(CLIENT_INFO_POS_Y + 2, INFO_POS_X + 10, "1");
    attroff(COLOR_PAIR(1));
    attron(COLOR_PAIR(2));
    mvprintw(CLIENT_INFO_POS_Y + 2, INFO_POS_X + 12, "2");
    attroff(COLOR_PAIR(2));
    attron(COLOR_PAIR(3));
    mvprintw(CLIENT_INFO_POS_Y + 2, INFO_POS_X + 14, "3");
    attroff(COLOR_PAIR(3));
    attron(COLOR_PAIR(4));
    mvprintw(CLIENT_INFO_POS_Y + 2, INFO_POS_X + 16, "4");
    attroff(COLOR_PAIR(4));
    attroff(A_BOLD);

    mvprintw(CLIENT_INFO_POS_Y + 3, CLIENT_INFO_POS_X, "1 coin - ");
    mvprintw(CLIENT_INFO_POS_Y + 3, CLIENT_INFO_POS_X + 13, "10 coins - ");
    mvprintw(CLIENT_INFO_POS_Y + 3, CLIENT_INFO_POS_X + 28, "50 coins - ");

    attron(COLOR_PAIR(13));
    mvprintw(CLIENT_INFO_POS_Y + 3, CLIENT_INFO_POS_X + 9, "c");
    mvprintw(CLIENT_INFO_POS_Y + 3, CLIENT_INFO_POS_X + 24, "t");
    mvprintw(CLIENT_INFO_POS_Y + 3, CLIENT_INFO_POS_X + 39, "T");
    attroff(COLOR_PAIR(13));

    mvprintw(CLIENT_INFO_POS_Y + 4, CLIENT_INFO_POS_X, "Bush - ");
    mvprintw(CLIENT_INFO_POS_Y + 4, CLIENT_INFO_POS_X + 13, "Campfire - ");
    mvprintw(CLIENT_INFO_POS_Y + 4, CLIENT_INFO_POS_X + 28, "Beast - ");

    attron(COLOR_PAIR(12));
    mvprintw(CLIENT_INFO_POS_Y + 4, CLIENT_INFO_POS_X + 7, "#");
    attroff(COLOR_PAIR(12));
    attron(COLOR_PAIR(14));
    attron(A_BOLD);
    mvprintw(CLIENT_INFO_POS_Y + 4, CLIENT_INFO_POS_X + 24, "A");
    mvprintw(CLIENT_INFO_POS_Y + 4, CLIENT_INFO_POS_X + 36, "*");
    attroff(A_BOLD);
    attroff(COLOR_PAIR(14));

    refresh();
}

void print_map_client() {
    for (int row = 0; row < PLAYER_POV; row++) {
        for (int col = 0; col < PLAYER_POV; col++) {
            print_tile_client(this_client.map[row][col], row + 1, col + 3);
        }
    }
    move(0, 0);
}

void print_info_client() {
    mvprintw(2, CLIENT_INFO_POS_X + PLAYER_POV + 25, "%d    ", this_client.server_pid);
    mvprintw(3, CLIENT_INFO_POS_X + PLAYER_POV + 25, "%d/%d     ", this_client.camp_x, this_client.camp_y);
    mvprintw(4, CLIENT_INFO_POS_X + PLAYER_POV + 25, "%d     ", this_client.round);

    mvprintw(6, CLIENT_INFO_POS_X + PLAYER_POV + 25, "%d/%d    ", this_client.pos_row, this_client.pos_col);
    mvprintw(7, CLIENT_INFO_POS_X + PLAYER_POV + 25, "%d      ", this_client.coins_carried);
    mvprintw(8, CLIENT_INFO_POS_X + PLAYER_POV + 25, "%d      ", this_client.coins_saved);
    mvprintw(9, CLIENT_INFO_POS_X + PLAYER_POV + 25, "%d      ", this_client.deaths);

}

void init_ui_client() {
    initscr();
    noecho();

    printw("TYPE c FOR CPU, h FOR HUMAN: ");
    int c = getch();
    if (c == 'c') this_client.playertype = CPU;
    if (c == 'h') this_client.playertype = HUMAN;

    if (has_colors() == TRUE) {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_BLUE); // For players
        init_pair(2, COLOR_WHITE, COLOR_CYAN);
        init_pair(3, COLOR_WHITE, COLOR_MAGENTA);
        init_pair(4, COLOR_WHITE, COLOR_RED);

        init_pair(11, COLOR_WHITE, COLOR_WHITE); // White for walls
        init_pair(12, COLOR_GREEN, COLOR_BLACK); // Green for bushes
        init_pair(13, COLOR_YELLOW, COLOR_BLACK); // Yellow for treasures
        init_pair(14, COLOR_RED, COLOR_BLACK); // Red for beasts and campsites
        init_pair(15, COLOR_BLACK, COLOR_BLACK); // Black for empty spaces
    }

    clear();
    print_legend();
}



// DATA TRANSFER /////////////////////////////////////////////////////////////////////////////////////////////////////

void get_map() {
    long bytes_received = recv(this_client.network_socket, this_client.map, sizeof(this_client.map), 0);
    if (bytes_received <= 0) {
        leave_game();
    } else {
        print_map_client();
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
        this_client.deaths = data.deaths;
        this_client.coins_carried = data.coins_carried;
        this_client.coins_saved = data.coins_saved;
        this_client.round = data.round;
        this_client.camp_x = data.camp_x;
        this_client.camp_y = data.camp_y;

        print_info_client();
    }

}

// GAME LOGIC //////////////////////////////////////////////////////////////////////////////////////////////////////////

void *key_listener(void *arg) {
    cbreak();
    keypad(stdscr, NULL);

    while (this_client.connected) {
        int key = getch();

        switch (key) {
            case 'Q':
            case 'q':
                this_client.connected = 0;
                leave_game();
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


void ai_client() {
    this_client.request[1] = rand() % 4;
    send(this_client.network_socket, this_client.request, sizeof(this_client.request), 0);
    get_map();
    get_info();
    refresh();
    usleep(TURN_TIME);
}

void game_client() {
    print_legend();
    if (this_client.playertype == CPU) {
        this_client.request[0] = MOVE;
        srand(time(0));
        while (this_client.connected) {
            ai_client();
        }

    } else if (this_client.playertype == HUMAN) {
        pthread_t keyboardListener;
        pthread_create(&keyboardListener, NULL, key_listener, NULL);
        while (this_client.connected) {
            send(this_client.network_socket, this_client.request, sizeof(this_client.request), 0);
            get_map();
            get_info();
            refresh();

            this_client.request[0] = WAIT;
            usleep(TURN_TIME);
        }

        pthread_join(keyboardListener, NULL);
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

    init_ui_client();
    game_client();
    return 0;
}