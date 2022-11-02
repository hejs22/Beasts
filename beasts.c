#include <stdlib.h>
#include <locale.h>

#include "config.h"
#include "world.h"
#include "player.h"
#include "beasts.h"

/*
 * @ brief checks if move is forbidden for beasts
 * @ param row of desired location
 * @ param col of desired location
 * @ return 0 if forbidden, 1 if valid
 */
int validMoveBeasts(int row, int col) {
    if ((row < 0) || (col < 0) || (col >= MAP_WIDTH) || (row >= MAP_HEIGHT)) return 0;
    if ((world.map[row][col] == WALL) || (world.map[row][col] == CAMPFIRE) || (world.map[row][col] == BEAST_TILE)) return 0;
    return 1;
}

/*
 * @ brief moves beast in desired location, handles it's collision and updates data
 * @ param pointer to beast's struct
 * @ param desired direction
 * @ return -
 */
void moveBeast(struct Beast *beast, enum DIRECTION dir) {
    if (beast == NULL) return;

    printTile(beast->standing_at, beast->pos_row, beast->pos_col);
    switch (dir) {
        case UP:
            if (validMoveBeasts(beast->pos_row - 1, beast->pos_col)) {
                handleCollisionBeast(beast, beast->pos_row - 1, beast->pos_col);
                beast->pos_row -= 1;
            }
            break;
        case DOWN:
            if (validMoveBeasts(beast->pos_row + 1, beast->pos_col)) {
                handleCollisionBeast(beast, beast->pos_row + 1, beast->pos_col);
                beast->pos_row += 1;
            }
            break;
        case LEFT:
            if (validMoveBeasts(beast->pos_row, beast->pos_col - 1)) {
                handleCollisionBeast(beast, beast->pos_row, beast->pos_col - 1);
                beast->pos_col -= 1;
            }
            break;
        case RIGHT:
            if (validMoveBeasts(beast->pos_row, beast->pos_col + 1)) {
                handleCollisionBeast(beast, beast->pos_row, beast->pos_col + 1);
                beast->pos_col += 1;
            }
            break;
        case STOP:
            break;
        default:
            break;
    }

    beast->standing_at = getTileAt(beast->pos_row, beast->pos_col);
    printTile(BEAST_TILE, beast->pos_row, beast->pos_col);
}

/*
 * @ brief creates new beast and sets it's parameters
 * @ return pointer to beast's structure
 */
struct Beast *createBeast() {
    struct Beast *new = malloc(sizeof(struct Beast));
    if (new == NULL) return NULL;

    int flag = 1;
    int rand_col;
    int rand_row;

    while (flag) {
        rand_col = rand() % MAP_WIDTH;
        rand_row = rand() % MAP_HEIGHT;
        if (world.map[rand_row][rand_col] == EMPTY) {
            printTile(BEAST_TILE, rand_row, rand_col);
            flag = 0;
        }
    }

    new->pos_col = rand_col;
    new->pos_row = rand_row;
    new->bush = 0;
    new->standing_at = EMPTY;

    return new;
}

/*
 * @ brief safely deletes beast
 * @ param pointer to beast's structure
 * @ return -
 */
void killBeast(struct Beast *beast) {
    if (beast != NULL) {
        printTile(beast->standing_at, beast->pos_row, beast->pos_col);
        world.active_beasts--;
        free(beast);
        beast = NULL;
    }
}

/*
 * @ brief checks multiple collision events and handles them
 * @ param pointer to beast's structure
 * @ param row of desired location
 * @ param col of desired location
 * @ return -
 */
void handleCollisionBeast(struct Beast *beast, int row, int col) {
    if ((world.map[row][col] >= FIRST_PLAYER) && (world.map[row][col] <= FOURTH_PLAYER)) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if ((world.players[i] != NULL) && (world.players[i]->pos_row == row) && (world.players[i]->pos_col == col)) {
                    killPlayer(world.players[i]);
            }
        }
    } else if (world.map[row][col] == BUSH) {
        beast->bush = 1;
    }
}