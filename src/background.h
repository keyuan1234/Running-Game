#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <windows.h>
#include "types.h"

void background_init(Background *background);
void background_update(Background *background, float dt, float speed);
void background_draw(HDC hdc, const Background *background);

#endif
