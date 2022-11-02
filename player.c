#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <locale.h>

#include "config.h"
#include "world.h"
#include "player.h"

/*
 * @ brief prints player's avatar on screen and updates world.map
 * @ param pointer to player's structure
 * @ param row of map
 * @ param column of map
 * @ return -
 */
void printPlayer(const struct Player *player, int row, int col) {
    attron(COLOR_PAIR(player->avatar));
    attron(A_BOLD);
    if (world.map[row][col] != BUSH) {
        mvprintw(1 + row, 3 + col, "%c", player->avatar);
        world.map[row][col] = player->avatar;
    }
    attroff(A_BOLD);
    attroff(COLOR_PAIR(player->avatar));
}

/*
 * @ brief checks player's collision and handles it
 * @ param pointer to player's structure
 * @ param row of map
 * @ param column of map
 * @ return -
 */
void handleCollisionPlayer(struct Player *player, int row, int col) {
    if (world.map[row][col] == SMALL_TREASURE) player->coins_carried += 1;
    else if (world.map[row][col] == MEDIUM_TREASURE) player->coins_carried += 10;
    else if (world.map[row][col] == BIG_TREASURE) player->coins_carried += 50;
    else if (world.map[row][col] == DROPPED_TREASURE) {
        player->coins_carried += findTreasureAt(row, col);
    } else if ((world.map[row + 1][col] == CAMPFIRE) || (world.map[row - 1][col] == CAMPFIRE) ||
               (world.map[row][col + 1] == CAMPFIRE) || (world.map[row][col - 1] == CAMPFIRE) ||
               (world.map[row + 1][col + 1] == CAMPFIRE) || (world.map[row - 1][col - 1] == CAMPFIRE) ||
               (world.map[row - 1][col + 1] == CAMPFIRE) || (world.map[row + 1][col - 1] == CAMPFIRE)) {
        player->coins_saved += player->coins_carried;
        player->coins_carried = 0;
    } else if (world.map[row][col] == BEAST_TILE) {
        killPlayer(player);
    } else if ((world.map[row][col] >= FIRST_PLAYER) && (world.map[row][col] <= FOURTH_PLAYER)) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if ((world.players[i] != NULL) && (world.players[i]->pos_row == row) && (world.players[i]->pos_col == col)) {
                killPlayer(world.players[i]);
            }
        }
        killPlayer(player);
    } else if (world.map[row][col] == BUSH) {
        player->bush = 1;
    }
}

/*
 * @ brief validates desired move
 * @ param row of map
 * @ param column of map
 * @ return 0 if move is forbidden, 1 if move is possible
 */
int validMove(int row, int col) {
    if ((row < 0) || (col < 0) || (col >= MAP_WIDTH) || (row >= MAP_HEIGHT)) return 0;
    if ((world.map[row][col] == WALL) || (world.map[row][col] == CAMPFIRE)) return 0;
    return 1;
}

/*
 * @ brief checks if player can move in desired direction, moves him and handles collision
 * @ param pointer to player's structure
 * @ param player's desired direction
 * @ return -
 */
void movePlayer(struct Player *player, enum DIRECTION dir) {
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

/*
 * @ brief creates new player and sets its starting parameters
 * @ param socket that player is connected to
 * @ return pointer to player's struct or NULL on failure
 */
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

/*
 * @ brief kills player, spawning him at his respawn point and drop his carried coins
 * @ param pointer to player's struct
 * @ return -
 */
void killPlayer(struct Player *player) {
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

/*
 * @ brief deletes player's structure and cleans up
 * @ param pointer to player's structure
 * @ return -
 */
void deletePlayer(struct Player *player) {
    killPlayer(player);
    close(player->socket);
    free(player);
    player = NULL;
    world.active_players--;
}

/*
 * @ brief drops coins of killed player
 * @ param pointer to player's structure
 * @ return -
 */
void dropTreasure(const struct Player *player) {
    world.treasure_map[player->pos_row][player->pos_col] += player->coins_carried;
    printTile(DROPPED_TREASURE, player->pos_row, player->pos_col);
};