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
        //printf("Couldn't load a map. Aborting...\n");
        return;
    }

    char c;
    int row = 0, col = 0;

    while (!feof(mapfile)) {
        fscanf(mapfile, "%c", &c);
        if (c == 'X' || c == '#' || c == ' ' || c == 'A') {
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
            } else if (world.map[row][col] == 'A') {
                print_tile(CAMPFIRE, row, col);
                world.campfire_row = row;
                world.campfire_col = col;
            }
        }
    }
}

void print_initial_objects() {
    for (int i = 0; i < TREASURES_AMOUNT; i++) create_object(rand() % 3 + 1);
    for (int i = 0; i < BUSHES_AMOUNT; i++) create_object(BUSH);
}

void update_info() {

    mvprintw(INFO_POS_Y, INFO_POS_X + 80, "%d    ", server.round);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        struct Player *player = world.players[i];
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

void print_info() {    // prints all additional info on console
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
    attron(COLOR_PAIR(1));
    mvprintw(INFO_POS_Y + 13, INFO_POS_X + 10, "1");
    attroff(COLOR_PAIR(1));
    attron(COLOR_PAIR(2));
    mvprintw(INFO_POS_Y + 13, INFO_POS_X + 12, "2");
    attroff(COLOR_PAIR(2));
    attron(COLOR_PAIR(3));
    mvprintw(INFO_POS_Y + 13, INFO_POS_X + 14, "3");
    attroff(COLOR_PAIR(3));
    attron(COLOR_PAIR(4));
    mvprintw(INFO_POS_Y + 13, INFO_POS_X + 16, "4");
    attroff(COLOR_PAIR(4));
    attroff(A_BOLD);

    mvprintw(INFO_POS_Y + 14, INFO_POS_X, "1 coin - ");
    mvprintw(INFO_POS_Y + 14, INFO_POS_X + 13, "10 coins - ");
    mvprintw(INFO_POS_Y + 14, INFO_POS_X + 28, "50 coins - ");

    attron(COLOR_PAIR(13));
    mvprintw(INFO_POS_Y + 14, INFO_POS_X + 9, "c");
    mvprintw(INFO_POS_Y + 14, INFO_POS_X + 24, "t");
    mvprintw(INFO_POS_Y + 14, INFO_POS_X + 39, "T");
    attroff(COLOR_PAIR(13));

    mvprintw(INFO_POS_Y + 15, INFO_POS_X, "Bush - ");
    mvprintw(INFO_POS_Y + 15, INFO_POS_X + 13, "Campfire - ");
    mvprintw(INFO_POS_Y + 15, INFO_POS_X + 28, "Beast - ");

    attron(COLOR_PAIR(12));
    mvprintw(INFO_POS_Y + 15, INFO_POS_X + 7, "#");
    attroff(COLOR_PAIR(12));
    attron(COLOR_PAIR(14));
    attron(A_BOLD);
    mvprintw(INFO_POS_Y + 15, INFO_POS_X + 24, "A");
    mvprintw(INFO_POS_Y + 15, INFO_POS_X + 36, "*");
    attroff(A_BOLD);
    attroff(COLOR_PAIR(14));

    update_info();
}

void print_tile(enum TILE TYPE, int row, int col) {
    if ((row < 0) || (col < 0) || (row >= MAP_HEIGHT) || (col >= MAP_WIDTH)) return;
    if (TYPE == BUSH) {
        attron(COLOR_PAIR(12));
        world.map[row][col] = '#';
        mvprintw(1 + row, 3 + col, "#");
        attroff(COLOR_PAIR(12));
    } else if (TYPE == SMALL_TREASURE) {
        attron(COLOR_PAIR(13));
        world.map[row][col] = 'c';
        mvprintw(1 + row, 3 + col, "c");
        attroff(COLOR_PAIR(13));
    } else if (TYPE == MEDIUM_TREASURE) {
        attron(COLOR_PAIR(13));
        world.map[row][col] = 't';
        mvprintw(1 + row, 3 + col, "t");
        attroff(COLOR_PAIR(13));
    } else if (TYPE == BIG_TREASURE) {
        attron(COLOR_PAIR(13));
        world.map[row][col] = 'T';
        mvprintw(1 + row, 3 + col, "T");
        attroff(COLOR_PAIR(13));
    } else if (TYPE == DROPPED_TREASURE) {
        attron(COLOR_PAIR(13));
        world.map[row][col] = 'D';
        mvprintw(1 + row, 3 + col, "D");
        attroff(COLOR_PAIR(13));
    } else if (TYPE == WALL) {
        attron(COLOR_PAIR(11));
        world.map[row][col] = 'X';
        mvprintw(1 + row, 3 + col, "X");
        attroff(COLOR_PAIR(11));
    } else if (TYPE == EMPTY) {
        if (world.map[row][col] != '#') {
            attron(COLOR_PAIR(15));
            world.map[row][col] = ' ';
            mvprintw(1 + row, 3 + col, " ");
            attroff(COLOR_PAIR(15));
        }
    }   else if (TYPE == CAMPFIRE) {
        attron(COLOR_PAIR(14));
        attron(A_BOLD);
        world.map[row][col] = 'A';
        mvprintw(1 + row, 3 + col, "A");
        attroff(A_BOLD);
        attroff(COLOR_PAIR(14));
    }   else if (TYPE == BEAST_TILE) {
        attron(COLOR_PAIR(14));
        attron(A_BOLD);
        if (world.map[row][col] != '#') {
            world.map[row][col] = '*';
            mvprintw(1 + row, 3 + col, "*");
        }
        attroff(A_BOLD);
        attroff(COLOR_PAIR(14));
    }

}

void print_player(struct Player *player, int row, int col) {
        attron(COLOR_PAIR(player->avatar - '0'));
        attron(A_BOLD);
        if (world.map[row][col] != '#') {
            mvprintw(1 + row, 3 + col, "%c", player->avatar);
            world.map[row][col] = player->avatar;
        }
        attroff(A_BOLD);
        attroff(COLOR_PAIR(player->avatar - '0'));
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

int find_treasure_at(int row, int col) {
    int coins = world.treasure_map[row][col];
    world.treasure_map[row][col] = 0;
    return coins;
}

void handle_collision_player(struct Player *player, int row, int col) {
    if (world.map[row][col] == 'c') player->coins_carried += 1;
    else if (world.map[row][col] == 't') player->coins_carried += 10;
    else if (world.map[row][col] == 'T') player->coins_carried += 50;
    else if (world.map[row][col] == 'D') {
        player->coins_carried += find_treasure_at(row, col);
    }
    else if ((world.map[row + 1][col] == 'A') || (world.map[row - 1][col] == 'A') || (world.map[row][col + 1] == 'A') || (world.map[row][col - 1] == 'A') || (world.map[row + 1][col + 1] == 'A') || (world.map[row - 1][col - 1] == 'A') || (world.map[row - 1][col + 1] == 'A') || (world.map[row + 1][col - 1] == 'A')) {
        player->coins_saved += player->coins_carried;
        player->coins_carried = 0;
    }
    else if (world.map[row][col] == '*') {
        killPlayer(player);
    }
    else if ((world.map[row][col] == '1') || (world.map[row][col] == '2') || (world.map[row][col] == '3') || (world.map[row][col] == '4')) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (world.players[i] != NULL) {
                if ((world.players[i]->pos_row == row) && (world.players[i]->pos_col == col)) {
                    killPlayer(world.players[i]);
                }
            }
        }
       killPlayer(player);
    }
    else if (world.map[row][col] == '#') {
        player->bush = 1;
    }
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

int validMove(int row, int col) {
    if ((row < 0) || (col < 0) || (col >= MAP_WIDTH) || (row >= MAP_HEIGHT)) return 0;
    if ((world.map[row][col] == 'X') || (world.map[row][col] == 'A')) return 0;
    return 1;
}

void movePlayer(struct Player *player, enum DIRECTION dir) {
    if (player == NULL) return;

    switch (dir) {
        case UP:
           if (validMove(player->pos_row - 1, player->pos_col)) {
               handle_collision_player(player, player->pos_row - 1, player->pos_col);
               print_tile(EMPTY, player->pos_row, player->pos_col);
               player->pos_row -= 1;
           }
            break;
        case DOWN:
            if (validMove(player->pos_row + 1, player->pos_col)) {
                handle_collision_player(player, player->pos_row + 1, player->pos_col);
                print_tile(EMPTY, player->pos_row, player->pos_col);
                player->pos_row += 1;
            }
            break;
        case LEFT:
            if (validMove(player->pos_row, player->pos_col - 1)) {
                handle_collision_player(player, player->pos_row, player->pos_col - 1);
                print_tile(EMPTY, player->pos_row, player->pos_col);
                player->pos_col -= 1;
            }
            break;
        case RIGHT:
            if (validMove(player->pos_row, player->pos_col + 1)) {
                handle_collision_player(player, player->pos_row, player->pos_col + 1);
                print_tile(EMPTY, player->pos_row, player->pos_col);
                player->pos_col += 1;
            }
            break;
        case STOP:
            break;
        default:
            break;
    }

    print_player(player, player->pos_row, player->pos_col);
}

void moveBeast(struct Beast *beast, enum DIRECTION dir) {
    if (beast == NULL) return;

    switch (dir) {
        case UP:
            if (validMove(beast->pos_row - 1, beast->pos_col)) {
                handle_collision_beast(beast, beast->pos_row - 1, beast->pos_col);
                print_tile(EMPTY, beast->pos_row, beast->pos_col);
                beast->pos_row -= 1;
            }
            break;
        case DOWN:
            if (validMove(beast->pos_row + 1, beast->pos_col)) {
                handle_collision_beast(beast, beast->pos_row - 1, beast->pos_col);
                print_tile(EMPTY, beast->pos_row, beast->pos_col);
                beast->pos_row += 1;
            }
            break;
        case LEFT:
            if (validMove(beast->pos_row, beast->pos_col - 1)) {
                handle_collision_beast(beast, beast->pos_row - 1, beast->pos_col);
                print_tile(EMPTY, beast->pos_row, beast->pos_col);
                beast->pos_col -= 1;
            }
            break;
        case RIGHT:
            if (validMove(beast->pos_row, beast->pos_col + 1)) {
                handle_collision_beast(beast, beast->pos_row - 1, beast->pos_col);
                print_tile(EMPTY, beast->pos_row, beast->pos_col);
                beast->pos_col += 1;
            }
            break;
        case STOP:
            break;
        default:
            break;
    }


    print_tile(BEAST_TILE, beast->pos_row, beast->pos_col);
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
    new->socket = socket;

    return new;
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

    return new;
}

void dropTreasure(struct Player *player) {
    world.treasure_map[player->pos_row][player->pos_col] += player->coins_carried;
    print_tile(DROPPED_TREASURE, player->pos_row, player->pos_col);
};

void killPlayer(struct Player *player) {
    if (player->bush) {
        world.map[player->pos_row][player->pos_col] = '#';
        print_tile(BUSH, player->pos_row, player->pos_col);
    } else {
        world.map[player->pos_row][player->pos_col] = ' ';
        print_tile(EMPTY, player->pos_row, player->pos_col);
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
    killPlayer(player);
    free(player);
    player = NULL;
    world.active_players--;
}

void refresh_screen() {
    clear();
    print_map();
    print_info();
    update_info();
    for (int row = 0; row < MAP_HEIGHT; row++) {
        for (int col = 0; col < MAP_WIDTH; col++) {
            if (world.map[row][col] == 'c') {
                print_tile(SMALL_TREASURE, row, col);
            } else if (world.map[row][col] == 't') {
                print_tile(MEDIUM_TREASURE, row, col);
            } else if (world.map[row][col] == 'T') {
                print_tile(BIG_TREASURE, row, col);
            } else if (world.map[row][col] == 'D') {
                print_tile(DROPPED_TREASURE, row, col);
            }
        }
    }
}