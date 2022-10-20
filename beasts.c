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
#include "world.h"
#include "server.h"
#include "player.h"
#include "beasts.h"


int validMoveBeasts(int row, int col) {
    if ((row < 0) || (col < 0) || (col >= MAP_WIDTH) || (row >= MAP_HEIGHT)) return 0;
    if ((world.map[row][col] == 'X') || (world.map[row][col] == 'A') || (world.map[row][col] == '*')) return 0;
    return 1;
}

void moveBeast(struct Beast *beast, enum DIRECTION dir) {
    if (beast == NULL) return;

    print_tile(beast->standing_at, beast->pos_row, beast->pos_col);
    switch (dir) {
        case UP:
            if (validMoveBeasts(beast->pos_row - 1, beast->pos_col)) {
                handle_collision_beast(beast, beast->pos_row - 1, beast->pos_col);
                beast->pos_row -= 1;
            }
            break;
        case DOWN:
            if (validMoveBeasts(beast->pos_row + 1, beast->pos_col)) {
                handle_collision_beast(beast, beast->pos_row + 1, beast->pos_col);
                beast->pos_row += 1;
            }
            break;
        case LEFT:
            if (validMoveBeasts(beast->pos_row, beast->pos_col - 1)) {
                handle_collision_beast(beast, beast->pos_row, beast->pos_col - 1);
                beast->pos_col -= 1;
            }
            break;
        case RIGHT:
            if (validMoveBeasts(beast->pos_row, beast->pos_col + 1)) {
                handle_collision_beast(beast, beast->pos_row, beast->pos_col + 1);
                beast->pos_col += 1;
            }
            break;
        case STOP:
            break;
        default:
            break;
    }

    beast->standing_at = get_tile_at(beast->pos_row, beast->pos_col);
    print_tile(BEAST_TILE, beast->pos_row, beast->pos_col);
}

struct Beast *create_beast() {
    struct Beast *new = malloc(sizeof(struct Beast));
    if (new == NULL) return NULL;

    int flag = 1, rand_col, rand_row;

    while (flag) {
        rand_col = rand() % MAP_WIDTH;
        rand_row = rand() % MAP_HEIGHT;
        if (world.map[rand_row][rand_col] == ' ') {
            print_tile(BEAST_TILE, rand_row, rand_col);
            flag = 0;
        }
    }

    new->pos_col = rand_col;
    new->pos_row = rand_row;
    new->bush = 0;
    new->standing_at = EMPTY;

    return new;
}

void handle_collision_beast(struct Beast *beast, int row, int col) {
    if ((world.map[row][col] == '1') || (world.map[row][col] == '2') || (world.map[row][col] == '3') || (world.map[row][col] == '4')) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (world.players[i] != NULL) {
                if ((world.players[i]->pos_row == row) && (world.players[i]->pos_col == col)) {
                    killPlayer(world.players[i]);
                }
            }
        }
    }
    else if (world.map[row][col] == '#') {
        beast->bush = 1;
    }
}