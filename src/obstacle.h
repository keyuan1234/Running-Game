#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <windows.h>
#include "types.h"

void obstacles_init(Obstacle obstacles[], int count);
void obstacles_update(Game *game, float dt);
void obstacles_draw(HDC hdc, const Obstacle obstacles[], int count, const Assets *assets);
RECT obstacle_get_rect(const Obstacle *obstacle);

#endif
