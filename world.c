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
    for (int row = 0; row < MAP_HEIGHT; row++) {
        for (int col = 0; col < MAP_WIDTH; col++) {
            if (world.map[row][col] == 'X') {
                print_tile(WALL, row, col);
            } else if (world.map[row][col] == '#') {
                print_tile(BUSH, row, col);
            } else if (world.map[row][col] == ' ') {
                print_tile(EMPTY, row, col);
            } else if (world.map[row][col] == 'O') {
                print_tile(CAMPFIRE, row, col);
            }
        }
    }

    for (int i = 0; i < TREASURES_AMOUNT; i++) create_object(rand() % 3 + 1);
    for (int i = 0; i < BUSHES_AMOUNT; i++) create_object(BUSH);

}

void update_info() {
    for (int i = 1; i < MAX_CLIENTS; i++) {
        struct Player *player = world.players[i];
        if (player != NULL) {
            mvprintw(INFO_POS_Y + 3, INFO_POS_X + 3 +  i * 15, "x");
            if (player->human) mvprintw(INFO_POS_Y + 4, INFO_POS_X +  i * 15, "HUMAN  ");
            else mvprintw(INFO_POS_Y + 4, INFO_POS_X + 1 +  i * 15, "CPU     ");
            mvprintw(INFO_POS_Y + 5, INFO_POS_X + 2 +  i * 15, "%d %d    ", player->pos_row, player->pos_col);
            mvprintw(INFO_POS_Y + 6, INFO_POS_X + 3 +  i * 15, "%d     ", player->deaths);
            mvprintw(INFO_POS_Y + 7, INFO_POS_X + 3 + i * 15, "%d     ", player->coins_saved + player->coins_carried);
            mvprintw(INFO_POS_Y + 8, INFO_POS_X + 3 + i * 15, "%d     ", player->coins_carried);
            mvprintw(INFO_POS_Y + 9, INFO_POS_X + 3 + i * 15, "%d     ", player->coins_saved);
        } else {
            mvprintw(INFO_POS_Y + 3, INFO_POS_X + 3 + i * 15, "?        ");
            mvprintw(INFO_POS_Y + 4, INFO_POS_X + 3 + i * 15, "?        ");
            mvprintw(INFO_POS_Y + 5, INFO_POS_X + 2 + i * 15, "?/?      ");
            mvprintw(INFO_POS_Y + 6, INFO_POS_X + 3 + i * 15, "?        ");
            mvprintw(INFO_POS_Y + 7, INFO_POS_X + 3 + i * 15, "?        ");
            mvprintw(INFO_POS_Y + 8, INFO_POS_X + 3 + i * 15, "?        ");
            mvprintw(INFO_POS_Y + 9, INFO_POS_X + 3 + i * 15, "?        ");
        }
    }
    refresh();
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

        update_info();

        mvprintw(INFO_POS_Y + 11, INFO_POS_X, "Legend: ");
}

void print_tile(enum TILE TYPE, int row, int col) {
    if ((row < 0) || (col < 0) || (row >= MAP_HEIGHT) || (col >= MAP_WIDTH)) return;
    if (TYPE == BUSH) {
        attron(COLOR_PAIR(2));
        world.map[row][col] = '#';
        mvprintw(1 + row, 3 + col, "#");
        attroff(COLOR_PAIR(2));
    } else if (TYPE == SMALL_TREASURE) {
        attron(COLOR_PAIR(3));
        world.map[row][col] = 'c';
        mvprintw(1 + row, 3 + col, "c");
        attroff(COLOR_PAIR(3));
    } else if (TYPE == MEDIUM_TREASURE) {
        attron(COLOR_PAIR(3));
        world.map[row][col] = 't';
        mvprintw(1 + row, 3 + col, "t");
        attroff(COLOR_PAIR(3));
    } else if (TYPE == BIG_TREASURE) {
        attron(COLOR_PAIR(3));
        world.map[row][col] = 'T';
        mvprintw(1 + row, 3 + col, "T");
        attroff(COLOR_PAIR(3));
    } else if (TYPE == WALL) {
        attron(COLOR_PAIR(1));
        world.map[row][col] = 'X';
        mvprintw(1 + row, 3 + col, "X");
        attroff(COLOR_PAIR(1));
    } else if (TYPE == EMPTY) {
        attron(COLOR_PAIR(6));
        world.map[row][col] = ' ';
        mvprintw(1 + row, 3 + col, " ");
        attroff(COLOR_PAIR(6));
    }   else if (TYPE == CAMPFIRE) {
        attron(COLOR_PAIR(4));
        attron(A_BOLD);
        world.map[row][col] = 'A';
        mvprintw(1 + row, 3 + col, "A");
        attroff(A_BOLD);
        attroff(COLOR_PAIR(4));
    }
    move(0, 0);
    refresh();
}

void print_player(struct Player *player, int row, int col) {
        attron(COLOR_PAIR(5));
        attron(A_BOLD);
        world.map[row][col] = player->avatar;
        mvprintw(1 + row, 3 + col, "%c", player->avatar);
        attroff(A_BOLD);
        attroff(COLOR_PAIR(5));
}

void create_object(enum TILE TYPE) {
    int flag = 1;
    while (flag) {
        int rand_col = rand() % MAP_WIDTH;
        int rand_row = rand() % MAP_HEIGHT;
        if (world.map[rand_row][rand_col] == ' ') {
            print_tile(TYPE, rand_row, rand_col);
            flag = 0;
        }
    }
}

void handle_collision(struct Player *P, int row, int col) {
    if (world.map[row][col] == 'c') P->coins_carried += 1;
    else if (world.map[row][col] == 't') P->coins_carried += 10;
    else if (world.map[row][col] == 'T') P->coins_carried += 50;
    else if (world.map[row][col] == 'A') {
        P->coins_saved += P->coins_carried;
        P->coins_carried = 0;
        // TODO prevent despawning of campfire
    }
    else if (world.map[row][col] == '*') {
        // TODO kill player
    } else if (world.map[row][col] == '@') {
        // TODO kill both players
    }
    else if (world.map[row][col] == '#') {
        P->bush = 2;
    }
}

int validMove(int row, int col) {
    if ((row < 0) || (col < 0) || (col >= MAP_WIDTH) || (row >= MAP_HEIGHT)) return 0;
    if (world.map[row][col] == 'X') return 0;
    return 1;
}

void movePlayer(struct Player *player, enum DIRECTION dir) {
    if (player == NULL) return;
    if (player->bush == 2) {
        player->bush = 1;
        return;
    }
    switch (dir) {
        case UP:
           if (validMove(player->pos_row - 1, player->pos_col)) {
               handle_collision(player, player->pos_row - 1, player->pos_col);
               //if (player->bush == 1) print_tile(BUSH, player->pos_row, player->pos_col);
               if (player->bush == 0) print_tile(EMPTY, player->pos_row, player->pos_col);
               print_player(player, player->pos_row - 1, player->pos_col);
               player->pos_row -= 1;
            }
            break;
        case DOWN:
            if (validMove(player->pos_row + 1, player->pos_col)) {
                //if (player->bush == 1) print_tile(BUSH, player->pos_row, player->pos_col);
                if (player->bush == 0)  print_tile(EMPTY, player->pos_row, player->pos_col);
                print_player(player, player->pos_row + 1, player->pos_col);
                player->pos_row += 1;
            }
            break;
        case LEFT:
            if (validMove(player->pos_row, player->pos_col - 1)) {
                //if (player->bush == 1) print_tile(BUSH, player->pos_row, player->pos_col);
                if (player->bush == 0)  print_tile(EMPTY, player->pos_row, player->pos_col);
                print_player(player, player->pos_row, player->pos_col - 1);
                player->pos_col -= 1;
            }
            break;
        case RIGHT:
            if (validMove(player->pos_row, player->pos_col + 1)) {
                //if (player->bush == 1) print_tile(BUSH, player->pos_row, player->pos_col);
                if (player->bush == 0)  print_tile(EMPTY, player->pos_row, player->pos_col);
                print_player(player, player->pos_row, player->pos_col + 1);
                player->pos_col += 1;
            }
            break;
        case STOP:
            break;
        default:
            break;
    }
    player->bush = 0;
}

struct Player *create_player(int socket) {
    struct Player *new = malloc(sizeof(struct Player));
    if (new == NULL) return NULL;

    int flag = 1, rand_col, rand_row;
    new->avatar = 0;

    while (flag) {
        rand_col = rand() % MAP_WIDTH;
        rand_row = rand() % MAP_HEIGHT;
        if (world.map[rand_row][rand_col] == ' ') {
            print_player(new, rand_row, rand_col);
            flag = 0;
        }
    }

    new->pos_col = rand_col;
    new->pos_row = rand_row;
    new->spawn_row = new->pos_row;
    new->spawn_col = new->pos_col;
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
    p = NULL;
}