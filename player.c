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

void printPlayer(const struct Player *player, int row, int col) {
    attron(COLOR_PAIR(player->avatar - '0'));
    attron(A_BOLD);
    if (world.map[row][col] != '#') {
        mvprintw(1 + row, 3 + col, "%c", player->avatar);
        world.map[row][col] = player->avatar;
    }
    attroff(A_BOLD);
    attroff(COLOR_PAIR(player->avatar - '0'));
}

void handleCollisionPlayer(struct Player *player, int row, int col) {
    // checks multiple collision events and handles them

    if (world.map[row][col] == 'c') player->coins_carried += 1;
    else if (world.map[row][col] == 't') player->coins_carried += 10;
    else if (world.map[row][col] == 'T') player->coins_carried += 50;
    else if (world.map[row][col] == 'D') {
        player->coins_carried += findTreasureAt(row, col);
    } else if ((world.map[row + 1][col] == 'A') || (world.map[row - 1][col] == 'A') ||
               (world.map[row][col + 1] == 'A') || (world.map[row][col - 1] == 'A') ||
               (world.map[row + 1][col + 1] == 'A') || (world.map[row - 1][col - 1] == 'A') ||
               (world.map[row - 1][col + 1] == 'A') || (world.map[row + 1][col - 1] == 'A')) {
        player->coins_saved += player->coins_carried;
        player->coins_carried = 0;
    } else if (world.map[row][col] == '*') {
        killPlayer(player);
    } else if ((world.map[row][col] == '1') || (world.map[row][col] == '2') || (world.map[row][col] == '3') ||
               (world.map[row][col] == '4')) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if ((world.players[i] != NULL) && (world.players[i]->pos_row == row) && (world.players[i]->pos_col == col)) {
                killPlayer(world.players[i]);
            }
        }
        killPlayer(player);
    } else if (world.map[row][col] == '#') {
        player->bush = 1;
    }
}


int validMove(int row, int col) {
    if ((row < 0) || (col < 0) || (col >= MAP_WIDTH) || (row >= MAP_HEIGHT)) return 0;
    if ((world.map[row][col] == 'X') || (world.map[row][col] == 'A')) return 0;
    return 1;
}

void movePlayer(struct Player *player, enum DIRECTION dir) {
    // checks if player can move in desired direction, if so, changes his coordinates
    if (player == NULL) return;

    switch (dir) {
        case UP:
            if (validMove(player->pos_row - 1, player->pos_col)) {
                handleCollisionPlayer(player, player->pos_row - 1, player->pos_col);
                printTile(EMPTY, player->pos_row, player->pos_col);
                player->pos_row -= 1;
            }
            break;
        case DOWN:
            if (validMove(player->pos_row + 1, player->pos_col)) {
                handleCollisionPlayer(player, player->pos_row + 1, player->pos_col);
                printTile(EMPTY, player->pos_row, player->pos_col);
                player->pos_row += 1;
            }
            break;
        case LEFT:
            if (validMove(player->pos_row, player->pos_col - 1)) {
                handleCollisionPlayer(player, player->pos_row, player->pos_col - 1);
                printTile(EMPTY, player->pos_row, player->pos_col);
                player->pos_col -= 1;
            }
            break;
        case RIGHT:
            if (validMove(player->pos_row, player->pos_col + 1)) {
                handleCollisionPlayer(player, player->pos_row, player->pos_col + 1);
                printTile(EMPTY, player->pos_row, player->pos_col);
                player->pos_col += 1;
            }
            break;
        default:
            break;
    }

    printPlayer(player, player->pos_row, player->pos_col);
}

struct Player *create_player(int socket) {
    struct Player *new = malloc(sizeof(struct Player));
    if (new == NULL) return NULL;

    int flag = 1;
    int rand_col;
    int rand_row;
    new->avatar = 0;

    while (flag) {
        rand_col = rand() % MAP_WIDTH;
        rand_row = rand() % MAP_HEIGHT;
        if (world.map[rand_row][rand_col] == ' ') {
            printPlayer(new, rand_row, rand_col);
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
    new->socket = socket;

    return new;
}

void killPlayer(struct Player *player) {
    if (player->bush) {
        world.map[player->pos_row][player->pos_col] = '#';
        printTile(BUSH, player->pos_row, player->pos_col);
    } else {
        world.map[player->pos_row][player->pos_col] = ' ';
        printTile(EMPTY, player->pos_row, player->pos_col);
    }

    if (player->coins_carried > 0) {
        dropTreasure(player);
    }

    player->pos_col = player->spawn_col;
    player->pos_row = player->spawn_row;
    player->coins_carried = 0;
    player->deaths++;
}

void deletePlayer(struct Player *player) {
    // cleans up after player
    killPlayer(player);
    close(player->socket);
    free(player);
    player = NULL;
    world.active_players--;
}

void dropTreasure(const struct Player *player) {
    // drops coins of player killed
    world.treasure_map[player->pos_row][player->pos_col] += player->coins_carried;
    printTile(DROPPED_TREASURE, player->pos_row, player->pos_col);
};