#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ncurses.h>
#include <locale.h>

#include "config.h"
#include "world.h"
#include "server.h"
#include "player.h"

/*
 * @ brief loads map from MAP_FILENAME to world.map
 * @ return 0 on success, 1 on failure
 */
int loadMap() {
    FILE *mapfile = fopen(MAP_FILENAME, "r");
    if (mapfile == NULL) {
        return 1;
    }

    char c;
    int row = 0;
    int col = 0;

    while (!feof(mapfile)) {
        fscanf(mapfile, "%c", &c);
        if ((c == WALL) || (c == BUSH) || (c == EMPTY) || (c == CAMPFIRE)) {
            world.map[row][col] = c;
            world.treasure_map[row][col] = 0;
            col++;
        } else if (c == '\n') {
            row++;
            col = 0;
        } else if (c == EOF) {
            break;
        }
    }

    fclose(mapfile);
    return 0;
}

/*
 * @ brief prints map on user's screen
 * @ return 0
 */
void printMap() {
    for (int row = 0; row < MAP_HEIGHT; row++) {
        for (int col = 0; col < MAP_WIDTH; col++) {
            if (world.map[row][col] == WALL) {
                printTile(WALL, row, col);
            } else if (world.map[row][col] == BUSH) {
                printTile(BUSH, row, col);
            } else if (world.map[row][col] == EMPTY) {
                printTile(EMPTY, row, col);
            } else if (world.map[row][col] == CAMPFIRE) {
                printTile(CAMPFIRE, row, col);
                world.campfire_row = row;
                world.campfire_col = col;
            }
        }
    }
}

/*
 * @ brief generates random starting objects
 * @ return -
 */
void printInitialObjects() {
    for (int i = 0; i < TREASURES_AMOUNT / 2; i++) createObject(SMALL_TREASURE);
    for (int i = 0; i < TREASURES_AMOUNT / 4; i++) createObject(MEDIUM_TREASURE);
    for (int i = 0; i < TREASURES_AMOUNT / 4; i++) createObject(BIG_TREASURE);
    for (int i = 0; i < BUSHES_AMOUNT; i++) createObject(BUSH);
}

/*
 * @ brief updates all player's and server info, such as coordinates, round, pids etc
 * @ return -
 */
void updateInfo() {
    mvprintw(INFO_POS_Y, INFO_POS_X + 80, "%d    ", server.round);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        const struct Player *player = world.players[i];
        if (player != NULL) {
            mvprintw(INFO_POS_Y + 3, INFO_POS_X + (i + 1) * 15, "%d      ", player->pid);
            if (player->human) mvprintw(INFO_POS_Y + 4, INFO_POS_X + (i + 1) * 15, "HUMAN  ");
            else mvprintw(INFO_POS_Y + 4, INFO_POS_X + (i + 1) * 15, "CPU     ");
            mvprintw(INFO_POS_Y + 5, INFO_POS_X + (i + 1) * 15, "%d %d    ", player->pos_row, player->pos_col);
            mvprintw(INFO_POS_Y + 6, INFO_POS_X + (i + 1) * 15, "%d     ", player->deaths);
            mvprintw(INFO_POS_Y + 7, INFO_POS_X + (i + 1) * 15, "%d     ", player->coins_saved + player->coins_carried);
            mvprintw(INFO_POS_Y + 8, INFO_POS_X + (i + 1) * 15, "%d     ", player->coins_carried);
            mvprintw(INFO_POS_Y + 9, INFO_POS_X + (i + 1) * 15, "%d     ", player->coins_saved);
        } else {
            mvprintw(INFO_POS_Y + 3, INFO_POS_X + (i + 1) * 15, "?        ");
            mvprintw(INFO_POS_Y + 4, INFO_POS_X + (i + 1) * 15, "?        ");
            mvprintw(INFO_POS_Y + 5, INFO_POS_X + (i + 1) * 15, "?/?      ");
            mvprintw(INFO_POS_Y + 6, INFO_POS_X + (i + 1) * 15, "?        ");
            mvprintw(INFO_POS_Y + 7, INFO_POS_X + (i + 1) * 15, "?        ");
            mvprintw(INFO_POS_Y + 8, INFO_POS_X + (i + 1) * 15, "?        ");
            mvprintw(INFO_POS_Y + 9, INFO_POS_X + (i + 1) * 15, "?        ");
        }
    }
}

/*
 * @ brief prints data that doesn't need to be updated every turn
 * @ return -
 */
void printInfo() {
    mvprintw(INFO_POS_Y, INFO_POS_X, "Server's PID: %d", server.pid);
    mvprintw(INFO_POS_Y, INFO_POS_X + 30, "Campsite's X/Y: %d/%d    ", world.campfire_row, world.campfire_col);
    mvprintw(INFO_POS_Y, INFO_POS_X + 65, "Round number: ");

    mvprintw(INFO_POS_Y + 2, INFO_POS_X, "Parameter: ");
    mvprintw(INFO_POS_Y + 3, INFO_POS_X, "PID: ");
    mvprintw(INFO_POS_Y + 4, INFO_POS_X, "Type: ");
    mvprintw(INFO_POS_Y + 5, INFO_POS_X, "X/Y: ");
    mvprintw(INFO_POS_Y + 6, INFO_POS_X, "Deaths: ");
    mvprintw(INFO_POS_Y + 7, INFO_POS_X, "Coins: ");
    mvprintw(INFO_POS_Y + 8, INFO_POS_X + 1, "-carried: ");
    mvprintw(INFO_POS_Y + 9, INFO_POS_X + 1, "-saved: ");

    mvprintw(INFO_POS_Y + 2, INFO_POS_X + 15, "Player1");
    mvprintw(INFO_POS_Y + 2, INFO_POS_X + 30, "Player2");
    mvprintw(INFO_POS_Y + 2, INFO_POS_X + 45, "Player3");
    mvprintw(INFO_POS_Y + 2, INFO_POS_X + 60, "Player4");

    mvprintw(INFO_POS_Y + 11, INFO_POS_X, "Legend: ");

    mvprintw(INFO_POS_Y + 13, INFO_POS_X, "Players - ");
    attron(A_BOLD);
    attron(COLOR_PAIR(FIRST_PLAYER));
    mvprintw(INFO_POS_Y + 13, INFO_POS_X + 10, "%c", FIRST_PLAYER);
    attroff(COLOR_PAIR(FIRST_PLAYER));
    attron(COLOR_PAIR(SECOND_PLAYER));
    mvprintw(INFO_POS_Y + 13, INFO_POS_X + 12, "%c", SECOND_PLAYER);
    attroff(COLOR_PAIR(SECOND_PLAYER));
    attron(COLOR_PAIR(THIRD_PLAYER));
    mvprintw(INFO_POS_Y + 13, INFO_POS_X + 14, "%c", THIRD_PLAYER);
    attroff(COLOR_PAIR(THIRD_PLAYER));
    attron(COLOR_PAIR(FOURTH_PLAYER));
    mvprintw(INFO_POS_Y + 13, INFO_POS_X + 16, "%c", FOURTH_PLAYER);
    attroff(COLOR_PAIR(FOURTH_PLAYER));
    attroff(A_BOLD);

    mvprintw(INFO_POS_Y + 14, INFO_POS_X, "1 coin - ");
    mvprintw(INFO_POS_Y + 14, INFO_POS_X + 13, "10 coins - ");
    mvprintw(INFO_POS_Y + 14, INFO_POS_X + 28, "50 coins - ");

    attron(COLOR_PAIR(BIG_TREASURE));
    mvprintw(INFO_POS_Y + 14, INFO_POS_X + 9, "%c", SMALL_TREASURE);
    mvprintw(INFO_POS_Y + 14, INFO_POS_X + 24, "%c", MEDIUM_TREASURE);
    mvprintw(INFO_POS_Y + 14, INFO_POS_X + 39, "%c", BIG_TREASURE);
    attroff(COLOR_PAIR(BIG_TREASURE));

    mvprintw(INFO_POS_Y + 15, INFO_POS_X, "Bush - ");
    mvprintw(INFO_POS_Y + 15, INFO_POS_X + 13, "Campfire - ");
    mvprintw(INFO_POS_Y + 15, INFO_POS_X + 28, "Beast - ");

    attron(COLOR_PAIR(BUSH));
    mvprintw(INFO_POS_Y + 15, INFO_POS_X + 7, "%c", BUSH);
    attroff(COLOR_PAIR(BUSH));
    attron(COLOR_PAIR(CAMPFIRE));
    attron(A_BOLD);
    mvprintw(INFO_POS_Y + 15, INFO_POS_X + 24, "%c", CAMPFIRE);
    mvprintw(INFO_POS_Y + 15, INFO_POS_X + 36, "%c", BEAST_TILE);
    attroff(A_BOLD);
    attroff(COLOR_PAIR(CAMPFIRE));

    updateInfo();
}

/*
 * @ brief prints tile at desired location
 * @ param type of tile to print
 * @ param row of map
 * @ param colum of map
 * @ return -
 */
void printTile(enum TILE TYPE, int row, int col) {
    if ((row < 0) || (col < 0) || (row >= MAP_HEIGHT) || (col >= MAP_WIDTH)) return;
    int pair = TYPE;
    int bold = 0;
    int hidden = 0;

    if (TYPE == EMPTY) {
        hidden = 1;
        if (world.map[row][col] != BUSH) {
            hidden = 0;
        }
    }   else if (TYPE == CAMPFIRE) {
        bold = 1;
    }   else if (TYPE == BEAST_TILE) {
        hidden = 1;
        if (world.map[row][col] != BUSH) {
            hidden = 0;
            bold = 1;
        }
    }

    if (!hidden) {
        attron(COLOR_PAIR(pair));
        if (bold) attron(A_BOLD);
        world.map[row][col] = TYPE;
        mvprintw(1 + row, 3 + col, "%c", TYPE);
        if (bold) attroff(A_BOLD);
        attroff(COLOR_PAIR(pair));
    }
}

/*
 * @ brief creates object at random location
 * @ param type of tile to create
 * @ return -
 */
void createObject(enum TILE TYPE) {
    int flag = 1;
    while (flag) {
        int rand_col = rand() % MAP_WIDTH;
        int rand_row = rand() % MAP_HEIGHT;
        if (world.map[rand_row][rand_col] == EMPTY) {
            printTile(TYPE, rand_row, rand_col);
            flag = 0;
        }
    }
}

/*
 * @ brief checks treasure value and set's it to 0
 * @ param row of map
 * @ param column of map
 * @ return value of treasure
 */
int findTreasureAt(int row, int col) {
    int coins = world.treasure_map[row][col];
    world.treasure_map[row][col] = 0;
    return coins;
}

/*
 * @ brief checks type of tile at given coordinates
 * @ param row of map
 * @ param column of map
 * @ return type of found tile on success, -1 if tile isn't needed or parameters are invalid
 */
enum TILE getTileAt(int row, int col) {
    if ((row < 0) || (col < 0) || (row >= MAP_HEIGHT) || (col >= MAP_WIDTH)) return -1;
    // only checking these tiles, because if beast collides with a player, it kills it. Collision with another beast or wall is forbidden.
    if ((world.map[row][col] == BUSH) || (world.map[row][col] == EMPTY) || (world.map[row][col] == SMALL_TREASURE) || (world.map[row][col] == MEDIUM_TREASURE) || (world.map[row][col] == BIG_TREASURE) || (world.map[row][col] == DROPPED_TREASURE)) {
        return world.map[row][col];
    }
    return -1;
}

/*
 * @ brief refreshes all printed info
 * @ return -
 */
void refreshScreen() {
    clear();
    printMap();
    printInfo();
    updateInfo();
    for (int row = 0; row < MAP_HEIGHT; row++) {
        for (int col = 0; col < MAP_WIDTH; col++) {
            printTile(world.map[row][col], row, col);
        }
    }
}