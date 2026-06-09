#include "resources.h"
#include <gdiplus/gdiplus.h>
#include <string.h>

static ULONG_PTR g_gdiplus_token = 0;

static void executable_dir(char *buffer, int buffer_size)
{
    int i;
    DWORD len = GetModuleFileNameA(NULL, buffer, (DWORD)buffer_size);
    if (len == 0 || len >= (DWORD)buffer_size) {
        buffer[0] = '\0';
        return;
    }
    for (i = (int)len - 1; i >= 0; --i) {
        if (buffer[i] == '\\' || buffer[i] == '/') {
            buffer[i + 1] = '\0';
            return;
        }
    }
    buffer[0] = '\0';
}

static void make_exe_relative_path(const char *relative_path, char *out, int out_size)
{
    executable_dir(out, out_size);
    if (out[0] == '\0') {
        lstrcpynA(out, relative_path, out_size);
        return;
    }
    lstrcatA(out, relative_path);
}

static void ansi_to_wide(const char *src, WCHAR *dst, int dst_count)
{
    MultiByteToWideChar(CP_ACP, 0, src, -1, dst, dst_count);
}

int resources_gdiplus_startup(void)
{
    GdiplusStartupInput input;
    memset(&input, 0, sizeof(input));
    input.GdiplusVersion = 1;
    return GdiplusStartup(&g_gdiplus_token, &input, NULL) == Ok;
}

void resources_gdiplus_shutdown(void)
{
    if (g_gdiplus_token != 0) {
        GdiplusShutdown(g_gdiplus_token);
        g_gdiplus_token = 0;
    }
}

static int sprite_load_gdiplus(Sprite *sprite, const WCHAR *path)
{
    GpBitmap *bitmap = NULL;
    HBITMAP hbitmap = NULL;
    BITMAP info;

    if (g_gdiplus_token == 0) {
        return 0;
    }

    if (GdipCreateBitmapFromFile(path, &bitmap) != Ok || !bitmap) {
        return 0;
    }

    if (GdipCreateHBITMAPFromBitmap(bitmap, &hbitmap, 0x00000000) == Ok && hbitmap) {
        sprite->bitmap = hbitmap;
        GetObjectA(sprite->bitmap, sizeof(info), &info);
        sprite->width = info.bmWidth;
        sprite->height = info.bmHeight;
        sprite->loaded = 1;
    }

    GdipDisposeImage((GpImage *)bitmap);
    return sprite->loaded;
}

static void sprite_load(Sprite *sprite, const char *path)
{
    BITMAP info;
    char exe_path[MAX_PATH];
    WCHAR wide_path[MAX_PATH];
    memset(sprite, 0, sizeof(*sprite));

    make_exe_relative_path(path, exe_path, MAX_PATH);
    sprite->bitmap = (HBITMAP)LoadImageA(NULL, exe_path, IMAGE_BITMAP, 0, 0,
                                         LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (!sprite->bitmap) {
        sprite->bitmap = (HBITMAP)LoadImageA(NULL, path, IMAGE_BITMAP, 0, 0,
                                             LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    }
    if (sprite->bitmap) {
        GetObjectA(sprite->bitmap, sizeof(info), &info);
        sprite->width = info.bmWidth;
        sprite->height = info.bmHeight;
        sprite->loaded = 1;
        return;
    }

    ansi_to_wide(exe_path, wide_path, MAX_PATH);
    if (!sprite_load_gdiplus(sprite, wide_path)) {
        ansi_to_wide(path, wide_path, MAX_PATH);
        sprite_load_gdiplus(sprite, wide_path);
    }
}

static void sprite_load_png(Sprite *sprite, const WCHAR *path)
{
    char exe_path_a[MAX_PATH];
    WCHAR exe_path_w[MAX_PATH];

    memset(sprite, 0, sizeof(*sprite));
    make_exe_relative_path("Pay.png", exe_path_a, MAX_PATH);
    ansi_to_wide(exe_path_a, exe_path_w, MAX_PATH);

    if (!sprite_load_gdiplus(sprite, exe_path_w)) {
        sprite_load_gdiplus(sprite, path);
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
    sprite_load(&assets->obstacle_train, "assets/obstacle_train.bmp");
    sprite_load_png(&assets->pay_image, L"Pay.png");
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
    if (assets->obstacle_train.bitmap) {
        DeleteObject(assets->obstacle_train.bitmap);
    }
    if (assets->pay_image.bitmap) {
        DeleteObject(assets->pay_image.bitmap);
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

