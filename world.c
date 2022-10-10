#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include "config.h"
#include "world.h"
#include "server.h"
#include "player.h"


void load_map() {  // loads map from txt file into global variable
    FILE *mapfile = fopen(MAP_FILENAME, "r");
    if (mapfile == NULL) {
        printf("Couldn't load a map. Aborting...\n");
        return;
    }

    char c;
    int row = 0, col = 0;

    while (!feof(mapfile)) {
        fscanf(mapfile, "%c", &c);
        if (c == 'X' || c == '#' || c == ' ' || c == 'O') {
            world.map[row][col] = c;
            col++;
        } else if (c == '\n') {
            row++;
            col = 0;
        } else if (c == EOF) {
            break;
        }
    }

    fclose(mapfile);
}

void print_map() {  // prints map on console based on world.map[][]
    for (int i = 0; i < MAP_HEIGHT; i++) {
        move(1 + i, 3);
        for (int j = 0; j < MAP_WIDTH; j++) {
            if (world.map[i][j] == 'X') {
                attron(COLOR_PAIR(1));
                printw("X");
                attroff(COLOR_PAIR(1));
            } else if (world.map[i][j] == '#') {
                attron(COLOR_PAIR(2));
                printw("#");
                attroff(COLOR_PAIR(2));
            } else if (world.map[i][j] == ' ') {
                attron(COLOR_PAIR(6));
                printw(" ");
                attroff(COLOR_PAIR(6));
            } else if (world.map[i][j] == 'O') {
                attron(COLOR_PAIR(4));
                printw("A");
                attroff(COLOR_PAIR(4));
            }
        }
    }
}

void print_info() {    // prints all additional info on console
    mvprintw(INFO_POS_Y, INFO_POS_X, "Server's PID: ");
    mvprintw(INFO_POS_Y, INFO_POS_X + 20, "Campsite's X/Y: ");
    mvprintw(INFO_POS_Y, INFO_POS_X + 45, "Round number: ");

    mvprintw(INFO_POS_Y + 2, INFO_POS_X, "Parameter: ");
    mvprintw(INFO_POS_Y + 3, INFO_POS_X, "PID: ");
    mvprintw(INFO_POS_Y + 4, INFO_POS_X, "Type: ");
    mvprintw(INFO_POS_Y + 5, INFO_POS_X, "X/Y: ");
    mvprintw(INFO_POS_Y + 6, INFO_POS_X, "Deaths: ");
    mvprintw(INFO_POS_Y + 7, INFO_POS_X, "Coins: ");
    mvprintw(INFO_POS_Y + 8, INFO_POS_X + 1, "-carried: ");
    mvprintw(INFO_POS_Y + 9, INFO_POS_X + 1, "-brought: ");

    mvprintw(INFO_POS_Y + 2, INFO_POS_X + 15, "Player1");
    mvprintw(INFO_POS_Y + 2, INFO_POS_X + 30, "Player2");
    mvprintw(INFO_POS_Y + 2, INFO_POS_X + 45, "Player3");
    mvprintw(INFO_POS_Y + 2, INFO_POS_X + 60, "Player4");

    mvprintw(INFO_POS_Y + 11, INFO_POS_X, "Legend: ");
}

void create_treasure(enum COMMAND TYPE) {
    int flag = 1;
    while (flag) {
        int rand_y = rand() % MAP_WIDTH;
        int rand_x = rand() % MAP_HEIGHT;
        if (world.map[rand_x][rand_y] == ' ') {
            attron(COLOR_PAIR(3));
            if (TYPE == 1) {
                world.map[rand_x][rand_y] = 'c';
                mvprintw(1 + rand_x, 3 + rand_y, "c");
            } else if (TYPE == 2) {
                world.map[rand_x][rand_y] = 't';
                mvprintw(1 + rand_x, 3 + rand_y, "t");
            } else if (TYPE == 3) {
                world.map[rand_x][rand_y] = 'T';
                mvprintw(1 + rand_x, 3 + rand_y, "T");
            }
            attroff(COLOR_PAIR(3));
            flag = 0;
        }
    }
}

struct Player *create_player(int socket) {
    struct Player *new = malloc(sizeof(struct Player));
    if (new == NULL) return NULL;

    int flag = 1, rand_y = 0, rand_x = 0;

    while (flag) {
        rand_y = rand() % MAP_WIDTH;
        rand_x = rand() % MAP_HEIGHT;
        if (world.map[rand_x][rand_y] == ' ') {
            attron(COLOR_PAIR(5));
            attron(A_BOLD);
            world.map[rand_x][rand_y] = '@';
            mvprintw(1 + rand_x, 3 + rand_y, "@");
            attroff(A_BOLD);
            attroff(COLOR_PAIR(5));
            flag = 0;
        }
    }

    new->pos_y = rand_y;
    new->pos_x = rand_x;
    new->spawn_x = new->pos_x;
    new->spawn_y = new->pos_y;
    new->bush = 0;
    new->coins_carried = 0;
    new->coins_saved = 0;
    new->deaths = 0;
    new->dir = STOP;
    new->socket = socket;
    //printf("Player created\n");
    refresh();

    return new;
}

void deletePlayer(struct Player *p) {
    world.active_players--;
    free(p);
}