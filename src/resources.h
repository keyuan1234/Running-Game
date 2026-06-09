#ifndef RESOURCES_H
#define RESOURCES_H

#include <windows.h>
#include "types.h"

void assets_load(Assets *assets);
void assets_free(Assets *assets);
void sprite_draw(HDC hdc, const Sprite *sprite, int x, int y, int width, int height);

#endif
