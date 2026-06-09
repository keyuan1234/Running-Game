#include "resources.h"
#include <string.h>

static void sprite_load(Sprite *sprite, const char *path)
{
    BITMAP info;
    memset(sprite, 0, sizeof(*sprite));
    sprite->bitmap = (HBITMAP)LoadImageA(NULL, path, IMAGE_BITMAP, 0, 0,
                                         LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (sprite->bitmap) {
        GetObjectA(sprite->bitmap, sizeof(info), &info);
        sprite->width = info.bmWidth;
        sprite->height = info.bmHeight;
        sprite->loaded = 1;
    }
}

void assets_load(Assets *assets)
{
    memset(assets, 0, sizeof(*assets));
    sprite_load(&assets->player_run[0], "assets/player_run_0.bmp");
    sprite_load(&assets->player_run[1], "assets/player_run_1.bmp");
    sprite_load(&assets->player_jump, "assets/player_jump.bmp");
    sprite_load(&assets->player_crouch, "assets/player_crouch.bmp");
    sprite_load(&assets->obstacle_ground, "assets/obstacle_ground.bmp");
    sprite_load(&assets->obstacle_air, "assets/obstacle_air.bmp");
    assets->hit_sound_available = GetFileAttributesA("assets/hit.wav") != INVALID_FILE_ATTRIBUTES;
}

void assets_free(Assets *assets)
{
    int i;
    for (i = 0; i < 2; ++i) {
        if (assets->player_run[i].bitmap) {
            DeleteObject(assets->player_run[i].bitmap);
        }
    }
    if (assets->player_jump.bitmap) {
        DeleteObject(assets->player_jump.bitmap);
    }
    if (assets->player_crouch.bitmap) {
        DeleteObject(assets->player_crouch.bitmap);
    }
    if (assets->obstacle_ground.bitmap) {
        DeleteObject(assets->obstacle_ground.bitmap);
    }
    if (assets->obstacle_air.bitmap) {
        DeleteObject(assets->obstacle_air.bitmap);
    }
    memset(assets, 0, sizeof(*assets));
}

void sprite_draw(HDC hdc, const Sprite *sprite, int x, int y, int width, int height)
{
    HDC memory_dc;
    HGDIOBJ old_bitmap;

    if (!sprite || !sprite->loaded || !sprite->bitmap) {
        return;
    }

    memory_dc = CreateCompatibleDC(hdc);
    old_bitmap = SelectObject(memory_dc, sprite->bitmap);
    StretchBlt(hdc, x, y, width, height, memory_dc, 0, 0,
               sprite->width, sprite->height, SRCCOPY);
    SelectObject(memory_dc, old_bitmap);
    DeleteDC(memory_dc);
}

