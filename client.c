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

#include "config.h"

pthread_mutex_t lock;

// DATA STRUCTURES //////////////////////////////////////////////////////////////////////////////////////////////////

struct client_socket {
    int network_socket;
    struct sockaddr_in server_address;
    char buffer[1024];
    char map[PLAYER_POV][PLAYER_POV];
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
    int campfire_found;
} this_client;

struct player_data_transfer {
    char map[PLAYER_POV][PLAYER_POV];
    int pos_X;
    int pos_Y;
    int coins_saved;
    int coins_carried;
    int deaths;
    int round;
};

// CONNECTION MANAGEMENT //////////////////////////////////////////////////////////////////////////////////////////////

void clientConfigure() {
    // create a socket
    this_client.network_socket = socket(AF_INET, SOCK_STREAM, 0);  // socket is created

    // create address structure, specify address for the socket
    this_client.server_address.sin_family = AF_INET;
    this_client.server_address.sin_port = htons(9002);
    this_client.server_address.sin_addr.s_addr = INADDR_ANY;

    this_client.round = -1;
    this_client.campfire_found = 0;
    this_client.camp_x = 0;
    this_client.camp_y = 0;
}

void leaveGame() {
    close(this_client.network_socket);
    this_client.connected = 0;
    clear();
    attron(A_BOLD);
    if (this_client.round == -1) {
        mvprintw(2, 2, "Server is full. Press any key to continue...");
    } else {
        int score = this_client.coins_saved;
        mvprintw(2, 2, "Game over, your score: %d. Press any key to continue...", score);
    }
    attroff(A_BOLD);
    getch();
    exit(0);
}

void estabilishConnection() {
    int i = 0;
    while (i < 10) {
        int connection_status = connect(this_client.network_socket, (struct sockaddr *) &this_client.server_address, sizeof(this_client.server_address));
        if (connection_status >= 0) break;
        printf("Waiting for server initialization...\n");
        usleep(TURN_TIME * 5);
        i++;
    }

    this_client.connected = 1;

    struct type_and_pid pid;
    pid.pid = getpid();
    pid.type = '1';

    long res = recv(this_client.network_socket, &this_client.server_pid, sizeof(this_client.server_pid), 0);
    if (res <= 0) {
        leaveGame();
    }

    send(this_client.network_socket, &pid, sizeof(pid), 0);
    res = recv(this_client.network_socket, this_client.buffer, sizeof(this_client.buffer), 0);
    if ((res <= 0) || (this_client.buffer[0] == 'E')) {
        leaveGame();
    }

    clear();
}

// USER GRAPHICAL INTERFACE ///////////////////////////////////////////////////////////////////////////////////////////

void rememberCampfire(int row, int col) {
    // remember coordinates of campfire if seen for the first time
    this_client.camp_x = row - (PLAYER_POV/2) + this_client.pos_row;
    this_client.camp_y = col - (PLAYER_POV/2) + this_client.pos_col;
    this_client.campfire_found = 1;
}

void printTileClient(enum TILE TYPE, int row, int col) {
    int pair = TYPE;
    int bold = 0;

    if ((TYPE == CAMPFIRE) && (this_client.campfire_found == 0)) {
            rememberCampfire(row, col);
    }

    if ((TYPE == CAMPFIRE) || (TYPE == BEAST_TILE) || ((TYPE >= FIRST_PLAYER) && (TYPE <= FOURTH_PLAYER))) {
        bold = 1;
    }

    if (bold) attron(A_BOLD);
    attron(COLOR_PAIR(pair));
    mvprintw(1 + row, 3 + col, "%c", TYPE);
    attroff(COLOR_PAIR(pair));
    if (bold) attroff(A_BOLD);
}

void printLegend() {
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
    attron(COLOR_PAIR(FIRST_PLAYER));
    mvprintw(CLIENT_INFO_POS_Y + 2, INFO_POS_X + 10, "%c", FIRST_PLAYER);
    attroff(COLOR_PAIR(FIRST_PLAYER));
    attron(COLOR_PAIR(SECOND_PLAYER));
    mvprintw(CLIENT_INFO_POS_Y + 2, INFO_POS_X + 12, "%c", SECOND_PLAYER);
    attroff(COLOR_PAIR(SECOND_PLAYER));
    attron(COLOR_PAIR(THIRD_PLAYER));
    mvprintw(CLIENT_INFO_POS_Y + 2, INFO_POS_X + 14, "%c", THIRD_PLAYER);
    attroff(COLOR_PAIR(THIRD_PLAYER));
    attron(COLOR_PAIR(FOURTH_PLAYER));
    mvprintw(CLIENT_INFO_POS_Y + 2, INFO_POS_X + 16, "%c", FOURTH_PLAYER);
    attroff(COLOR_PAIR(FOURTH_PLAYER));
    attroff(A_BOLD);

    mvprintw(CLIENT_INFO_POS_Y + 3, CLIENT_INFO_POS_X, "1 coin - ");
    mvprintw(CLIENT_INFO_POS_Y + 3, CLIENT_INFO_POS_X + 13, "10 coins - ");
    mvprintw(CLIENT_INFO_POS_Y + 3, CLIENT_INFO_POS_X + 28, "50 coins - ");

    attron(COLOR_PAIR(SMALL_TREASURE));
    mvprintw(CLIENT_INFO_POS_Y + 3, CLIENT_INFO_POS_X + 9, "%c", SMALL_TREASURE);
    mvprintw(CLIENT_INFO_POS_Y + 3, CLIENT_INFO_POS_X + 24, "%c", MEDIUM_TREASURE);
    mvprintw(CLIENT_INFO_POS_Y + 3, CLIENT_INFO_POS_X + 39, "%c", BIG_TREASURE);
    attroff(COLOR_PAIR(SMALL_TREASURE));

    mvprintw(CLIENT_INFO_POS_Y + 4, CLIENT_INFO_POS_X, "Bush - ");
    mvprintw(CLIENT_INFO_POS_Y + 4, CLIENT_INFO_POS_X + 13, "Campfire - ");
    mvprintw(CLIENT_INFO_POS_Y + 4, CLIENT_INFO_POS_X + 28, "Beast - ");

    attron(COLOR_PAIR(BUSH));
    mvprintw(CLIENT_INFO_POS_Y + 4, CLIENT_INFO_POS_X + 7, "%c", BUSH);
    attroff(COLOR_PAIR(BUSH));
    attron(COLOR_PAIR(BEAST_TILE));
    attron(A_BOLD);
    mvprintw(CLIENT_INFO_POS_Y + 4, CLIENT_INFO_POS_X + 24, "%c", CAMPFIRE);
    mvprintw(CLIENT_INFO_POS_Y + 4, CLIENT_INFO_POS_X + 36, "%c", BEAST_TILE);
    attroff(A_BOLD);
    attroff(COLOR_PAIR(BEAST_TILE));
}

void printMapClient() {
    for (int row = 0; row < PLAYER_POV; row++) {
        for (int col = 0; col < PLAYER_POV; col++) {
            printTileClient(this_client.map[row][col], row + 1, col + 3);
        }
    }
    move(0, 0);
}

void printInfoClient() {
    mvprintw(2, CLIENT_INFO_POS_X + PLAYER_POV + 25, "%d    ", this_client.server_pid);
    mvprintw(3, CLIENT_INFO_POS_X + PLAYER_POV + 26, "%d/%d     ", this_client.camp_x, this_client.camp_y);
    mvprintw(4, CLIENT_INFO_POS_X + PLAYER_POV + 25, "%d     ", this_client.round);

    mvprintw(6, CLIENT_INFO_POS_X + PLAYER_POV + 25, "%d/%d    ", this_client.pos_row, this_client.pos_col);
    mvprintw(7, CLIENT_INFO_POS_X + PLAYER_POV + 25, "%d      ", this_client.coins_carried);
    mvprintw(8, CLIENT_INFO_POS_X + PLAYER_POV + 25, "%d      ", this_client.coins_saved);
    mvprintw(9, CLIENT_INFO_POS_X + PLAYER_POV + 25, "%d      ", this_client.deaths);
}

void initUiClient() {
    initscr();
    noecho();

    printw("TYPE c FOR CPU, h FOR HUMAN: ");
    int c = getch();
    if (c == 'c') this_client.playertype = CPU;
    if (c == 'h') this_client.playertype = HUMAN;

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

    clear();
    printLegend();
}

// DATA TRANSFER /////////////////////////////////////////////////////////////////////////////////////////////////////

void getInfo() {
    struct player_data_transfer data;
    long bytes_received = recv(this_client.network_socket, (void *) &data, sizeof(data), 0);

    if (bytes_received <= 0) {
        leaveGame();
    } else {
        memcpy(this_client.map, data.map, sizeof(data.map));
        this_client.pos_row = data.pos_X;
        this_client.pos_col = data.pos_Y;
        this_client.deaths = data.deaths;
        this_client.coins_carried = data.coins_carried;
        this_client.coins_saved = data.coins_saved;
        this_client.round = data.round;

        printMapClient();
        printInfoClient();
    }
    refresh();
}

// GAME LOGIC //////////////////////////////////////////////////////////////////////////////////////////////////////////

void *keyListener(void *arg) {
    enum PLAYERTYPE TYPE = *(enum PLAYERTYPE *) arg;
    cbreak();
    keypad(stdscr, TRUE);

    if (TYPE == CPU) {
        // CPU client can disconnect with q
        while (this_client.connected) {
            int key = getch();
            pthread_mutex_lock(&lock);
            switch (key) {
                case 'Q':
                case 'q':
                    leaveGame();
                    break;
                default:
                    break;
            }
            pthread_mutex_unlock(&lock);
        }
    } else if (TYPE == HUMAN) {
        // HUMAN client can move with arrows, wsad, or disconnect with q
        while (this_client.connected) {
            int key = getch();
            pthread_mutex_lock(&lock);
            switch (key) {
                case 'Q':
                case 'q':
                    leaveGame();
                    break;
                case 'w':
                case KEY_UP:
                    this_client.request[0] = MOVE;
                    this_client.request[1] = UP;
                    break;
                case 'a':
                case KEY_LEFT:
                    this_client.request[0] = MOVE;
                    this_client.request[1] = LEFT;
                    break;
                case 's':
                case KEY_DOWN:
                    this_client.request[0] = MOVE;
                    this_client.request[1] = DOWN;
                    break;
                case 'd':
                case KEY_RIGHT:
                    this_client.request[0] = MOVE;
                    this_client.request[1] = RIGHT;
                    break;
                default:
                    break;
            }
            pthread_mutex_unlock(&lock);
        }
    }

    pthread_exit(NULL);
}


int isNotObstacle(int row, int col) {
    row += (PLAYER_POV / 2);
    col += (PLAYER_POV / 2);
    if ((this_client.map[row][col] == CAMPFIRE) || (this_client.map[row][col] == WALL) || (this_client.map[row][col] == BEAST_TILE)) {
        return 0;
    }
    return 1;
}

int isCollectible(int row, int col) {
    row += (PLAYER_POV / 2);
    col += (PLAYER_POV / 2);
    if ((this_client.map[row][col] == DROPPED_TREASURE) || (this_client.map[row][col] == BIG_TREASURE) ||
        (this_client.map[row][col] == MEDIUM_TREASURE) || (this_client.map[row][col] == SMALL_TREASURE)) {
        return 1;
    }
    return 0;
}

enum DIRECTION scanArea() {
    // find relatively safe direction, if there are no dangers nearby, orders to collect coins
    enum DIRECTION dir = STOP;

    if (isCollectible(1, 0)) {
        dir = DOWN;
    } else if (isCollectible(-1, 0)) {
        dir = UP;
    } else if (isCollectible(0, 1)) {
        dir = RIGHT;
    } else if (isCollectible(0, -1)) {
        dir = LEFT;
    }

    if (dir == STOP) {
        if (isNotObstacle(1, 0) && isCollectible(2, 0)) {
            dir = DOWN;
        } else if (isNotObstacle(-1, 0) && isCollectible(-2, 0)) {
            dir = UP;
        } else if (isNotObstacle(0, 1) && isCollectible(0, 2)) {
            dir = RIGHT;
        } else if (isNotObstacle(0, -1) && isCollectible(0, -2)) {
            dir = LEFT;
        }
    }

    if (dir == STOP) {
        dir = rand() % 4;
    }

    return dir;
}

void aiClient() {
    // gets map, chooses direction, sends request
    getInfo();
    this_client.request[1] = scanArea();
    send(this_client.network_socket, this_client.request, sizeof(this_client.request), 0);
    getInfo();
    usleep(TURN_TIME);
    send(this_client.network_socket, this_client.request, sizeof(this_client.request), 0);
}

void humanClient() {
    getInfo();
    send(this_client.network_socket, this_client.request, sizeof(this_client.request), 0);
    this_client.request[0] = WAIT;
    getInfo();
    usleep(TURN_TIME);
    send(this_client.network_socket, this_client.request, sizeof(this_client.request), 0);
}

void gameClient() {
    pthread_t keyboardListener;
    pthread_create(&keyboardListener, NULL, keyListener, &this_client.playertype);

    if (this_client.playertype == CPU) {
        this_client.request[0] = MOVE;
        srand(time(0));
        while (this_client.connected) {
            aiClient();
        }

    } else if (this_client.playertype == HUMAN) {
        while (this_client.connected) {
            humanClient();
        }
        pthread_join(keyboardListener, NULL);
    }
    leaveGame();
}

int main() { // client application

    clientConfigure();
    estabilishConnection();

    initUiClient();
    printLegend();
    gameClient();
    return 0;
}