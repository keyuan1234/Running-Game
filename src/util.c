#include "util.h"
#include "config.h"
#include <stdlib.h>

int rects_overlap(RECT a, RECT b)
{
    return a.left < b.right &&
           a.right > b.left &&
           a.top < b.bottom &&
           a.bottom > b.top;
}

float rand_float(float min_value, float max_value)
{
    float t = (float)rand() / (float)RAND_MAX;
    return min_value + (max_value - min_value) * t;
}

void draw_panel(HDC hdc, RECT rect, COLORREF bg_color, COLORREF border_color, int radius)
{
    HBRUSH bg_brush = CreateSolidBrush(bg_color);
    HPEN border_pen = CreatePen(PS_SOLID, 2, border_color);
    HGDIOBJ old_brush = SelectObject(hdc, bg_brush);
    HGDIOBJ old_pen = SelectObject(hdc, border_pen);

    if (radius > 0) {
        RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    } else {
        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
    }

    SelectObject(hdc, old_pen);
    SelectObject(hdc, old_brush);
    DeleteObject(border_pen);
    DeleteObject(bg_brush);
}

void draw_overlay(HDC hdc)
{
    RECT overlay;
    HBRUSH dim_brush;
    BLENDFUNCTION blend;
    HDC overlay_dc;
    HBITMAP overlay_bmp;
    HGDIOBJ old_bmp;

    overlay.left = 0;
    overlay.top = 0;
    overlay.right = WINDOW_WIDTH;
    overlay.bottom = WINDOW_HEIGHT;

    overlay_dc = CreateCompatibleDC(hdc);
    overlay_bmp = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
    old_bmp = SelectObject(overlay_dc, overlay_bmp);

    dim_brush = CreateSolidBrush(RGB(2, 4, 10));
    FillRect(overlay_dc, &overlay, dim_brush);
    DeleteObject(dim_brush);

    memset(&blend, 0, sizeof(blend));
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 140;
    blend.AlphaFormat = 0;

    AlphaBlend(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
               overlay_dc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, blend);

    SelectObject(overlay_dc, old_bmp);
    DeleteObject(overlay_bmp);
    DeleteDC(overlay_dc);
}

