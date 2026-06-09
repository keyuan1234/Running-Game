#ifndef UTIL_H
#define UTIL_H

#include <windows.h>

int rects_overlap(RECT a, RECT b);
float rand_float(float min_value, float max_value);
void draw_panel(HDC hdc, RECT rect, COLORREF bg_color, COLORREF border_color, int radius);
void draw_overlay(HDC hdc);

#endif
