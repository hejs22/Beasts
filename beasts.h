#ifndef BEASTS_BEASTS_H
#define BEASTS_BEASTS_H

#include "config.h"
#include "world.h"

struct Beast *create_beast();
void moveBeast(struct Beast *, enum DIRECTION);
int validMoveBeasts(int row, int col);
void moveBeast(struct Beast *beast, enum DIRECTION dir);
void handle_collision_beast(struct Beast *beast, int row, int col);

#endif //BEASTS_BEASTS_H
