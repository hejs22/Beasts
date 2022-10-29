#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <locale.h>

#include "config.h"
#include "world.h"
#include "player.h"

void printPlayer(const struct Player *player, int row, int col) {
    // prints players avatars on map and updates world.map[][]
    attron(COLOR_PAIR(player->avatar));
    attron(A_BOLD);
    if (world.map[row][col] != BUSH) {
        mvprintw(1 + row, 3 + col, "%c", player->avatar);
        world.map[row][col] = player->avatar;
    }
    attroff(A_BOLD);
    attroff(COLOR_PAIR(player->avatar));
}

void handleCollisionPlayer(struct Player *player, int row, int col) {
    // checks multiple collision events and handles them

    // if stepped on treasures, collect them
    if (world.map[row][col] == SMALL_TREASURE) player->coins_carried += 1;
    else if (world.map[row][col] == MEDIUM_TREASURE) player->coins_carried += 10;
    else if (world.map[row][col] == BIG_TREASURE) player->coins_carried += 50;
    else if (world.map[row][col] == DROPPED_TREASURE) {
        player->coins_carried += findTreasureAt(row, col);
    } else if ((world.map[row + 1][col] == CAMPFIRE) || (world.map[row - 1][col] == CAMPFIRE) ||
               (world.map[row][col + 1] == CAMPFIRE) || (world.map[row][col - 1] == CAMPFIRE) ||
               (world.map[row + 1][col + 1] == CAMPFIRE) || (world.map[row - 1][col - 1] == CAMPFIRE) ||
               (world.map[row - 1][col + 1] == CAMPFIRE) || (world.map[row + 1][col - 1] == CAMPFIRE)) {
        // if campfire is nearby, save player's coins
        player->coins_saved += player->coins_carried;
        player->coins_carried = 0;
    } else if (world.map[row][col] == BEAST_TILE) {
        // if stepped on beast, die
        killPlayer(player);
    } else if ((world.map[row][col] >= FIRST_PLAYER) && (world.map[row][col] <= FOURTH_PLAYER)) {
        // if stepped on another player, both dies
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if ((world.players[i] != NULL) && (world.players[i]->pos_row == row) && (world.players[i]->pos_col == col)) {
                killPlayer(world.players[i]);
            }
        }
        killPlayer(player);
    } else if (world.map[row][col] == BUSH) {
        // if stepped on bush, wait 1 turn
        player->bush = 1;
    }
}


int validMove(int row, int col) {
    if ((row < 0) || (col < 0) || (col >= MAP_WIDTH) || (row >= MAP_HEIGHT)) return 0;
    if ((world.map[row][col] == WALL) || (world.map[row][col] == CAMPFIRE)) return 0;
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

    // choose random spawn point
    while (flag) {
        rand_col = rand() % MAP_WIDTH;
        rand_row = rand() % MAP_HEIGHT;
        if (world.map[rand_row][rand_col] == EMPTY) {
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
    // kill player, if he was hiding in bush, print bush in place of his deaths, else print empty space
    if (player->bush) {
        world.map[player->pos_row][player->pos_col] = BUSH;
        printTile(BUSH, player->pos_row, player->pos_col);
    } else {
        world.map[player->pos_row][player->pos_col] = EMPTY;
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