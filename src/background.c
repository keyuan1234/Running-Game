#include "background.h"

static void draw_sky(HDC hdc)
{
    int y;
    for (y = 0; y < RUNWAY_BOTTOM_Y; ++y) {
        int r = 118 + y / 13;
        int g = 178 + y / 28;
        int b = 226 - y / 24;
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(r, g, b));
        HGDIOBJ old_pen = SelectObject(hdc, pen);
        MoveToEx(hdc, 0, y, NULL);
        LineTo(hdc, WINDOW_WIDTH, y);
        SelectObject(hdc, old_pen);
        DeleteObject(pen);
    }
}

void background_init(Background *background)
{
    background->rail_offset = 0.0f;
    background->sleeper_offset = 0.0f;
    background->building_offset = 0.0f;
}

void background_update(Background *background, float dt, float speed)
{
    background->rail_offset += speed * 0.32f * dt;
    background->sleeper_offset += speed * 0.22f * dt;
    background->building_offset += speed * 0.20f * dt;

    if (background->rail_offset > 120.0f) {
        background->rail_offset -= 120.0f;
    }
    if (background->sleeper_offset > 100.0f) {
        background->sleeper_offset -= 100.0f;
    }
    if (background->building_offset > 160.0f) {
        background->building_offset -= 160.0f;
    }
}

static float perspective_from_t(float t)
{
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }
    return t * t * (1.18f - 0.18f * t);
}

static void draw_city(HDC hdc, const Background *background)
{
    int i;
    for (i = -1; i < 8; ++i) {
        int x = i * 160 - (int)background->building_offset;
        int h = 95 + (i % 3) * 34;
        HBRUSH brush = CreateSolidBrush(RGB(84, 104, 130));
        HBRUSH lit = CreateSolidBrush(RGB(244, 215, 118));
        HGDIOBJ old_brush = SelectObject(hdc, brush);
        Rectangle(hdc, x, HORIZON_Y - h + 35, x + 70, HORIZON_Y + 70);
        Rectangle(hdc, WINDOW_WIDTH - x - 72, HORIZON_Y - h + 18,
                  WINDOW_WIDTH - x - 4, HORIZON_Y + 70);
        SelectObject(hdc, lit);
        Rectangle(hdc, x + 14, HORIZON_Y - h + 58, x + 28, HORIZON_Y - h + 72);
        Rectangle(hdc, x + 42, HORIZON_Y - h + 82, x + 56, HORIZON_Y - h + 96);
        Rectangle(hdc, WINDOW_WIDTH - x - 52, HORIZON_Y - h + 48,
                  WINDOW_WIDTH - x - 38, HORIZON_Y - h + 62);
        SelectObject(hdc, old_brush);
        DeleteObject(brush);
        DeleteObject(lit);
    }
}

static void draw_runway(HDC hdc, const Background *background)
{
    int i;
    POINT ground[4] = {
        { 0, WINDOW_HEIGHT },
        { WINDOW_WIDTH, WINDOW_HEIGHT },
        { WINDOW_WIDTH / 2 + 105, HORIZON_Y },
        { WINDOW_WIDTH / 2 - 105, HORIZON_Y }
    };
    HBRUSH ground_brush = CreateSolidBrush(RGB(68, 78, 88));
    HPEN rail_pen = CreatePen(PS_SOLID, 5, RGB(235, 220, 163));
    HPEN lane_pen = CreatePen(PS_SOLID, 2, RGB(190, 202, 210));
    HGDIOBJ old_brush = SelectObject(hdc, ground_brush);
    HGDIOBJ old_pen = SelectObject(hdc, rail_pen);

    Polygon(hdc, ground, 4);

    for (i = -2; i <= 2; ++i) {
        int bottom_x = WINDOW_WIDTH / 2 + i * 90;
        int top_x = WINDOW_WIDTH / 2 + i * 20;
        SelectObject(hdc, i == -1 || i == 1 ? lane_pen : rail_pen);
        MoveToEx(hdc, top_x, HORIZON_Y, NULL);
        LineTo(hdc, bottom_x, WINDOW_HEIGHT);
    }

    SelectObject(hdc, rail_pen);
    for (i = 0; i < 18; ++i) {
        float phase = background->sleeper_offset / 100.0f;
        float t = ((float)i + phase) / 17.0f;
        float p = perspective_from_t(t);
        int y = HORIZON_Y + (int)((RUNWAY_BOTTOM_Y - HORIZON_Y) * p);
        int width = 34 + (int)(520 * p);
        MoveToEx(hdc, WINDOW_WIDTH / 2 - width / 2, y, NULL);
        LineTo(hdc, WINDOW_WIDTH / 2 + width / 2, y);
    }

    SelectObject(hdc, lane_pen);
    for (i = 0; i < 12; ++i) {
        float phase = background->rail_offset / 120.0f;
        float t = ((float)i + phase) / 11.0f;
        float p = perspective_from_t(t);
        int y = HORIZON_Y + (int)((RUNWAY_BOTTOM_Y - HORIZON_Y) * p);
        int dash = 8 + (int)(38 * p);
        MoveToEx(hdc, WINDOW_WIDTH / 2, y - dash / 2, NULL);
        LineTo(hdc, WINDOW_WIDTH / 2, y + dash / 2);
    }

    SelectObject(hdc, old_pen);
    SelectObject(hdc, old_brush);
    DeleteObject(rail_pen);
    DeleteObject(lane_pen);
    DeleteObject(ground_brush);
}

static void draw_side_props(HDC hdc, const Background *background)
{
    int i;
    HPEN pole_pen = CreatePen(PS_SOLID, 3, RGB(245, 225, 145));
    HBRUSH sign_brush = CreateSolidBrush(RGB(218, 75, 91));
    HBRUSH light_brush = CreateSolidBrush(RGB(255, 238, 146));
    HGDIOBJ old_pen = SelectObject(hdc, pole_pen);
    HGDIOBJ old_brush = SelectObject(hdc, sign_brush);

    for (i = 0; i < 9; ++i) {
        float phase = background->building_offset / 160.0f;
        float t = ((float)i + phase) / 8.0f;
        float p = perspective_from_t(t);
        int y = HORIZON_Y + (int)((RUNWAY_BOTTOM_Y - HORIZON_Y) * p);
        int side_span = 140 + (int)(410 * p);
        int pole_height = 24 + (int)(98 * p);
        int sign_w = 16 + (int)(58 * p);
        int sign_h = 8 + (int)(30 * p);
        int left_x = WINDOW_WIDTH / 2 - side_span;
        int right_x = WINDOW_WIDTH / 2 + side_span;

        MoveToEx(hdc, left_x, y, NULL);
        LineTo(hdc, left_x, y - pole_height);
        MoveToEx(hdc, right_x, y, NULL);
        LineTo(hdc, right_x, y - pole_height);

        SelectObject(hdc, light_brush);
        Ellipse(hdc, left_x - sign_h / 2, y - pole_height - sign_h,
                left_x + sign_h / 2, y - pole_height);
        Ellipse(hdc, right_x - sign_h / 2, y - pole_height - sign_h,
                right_x + sign_h / 2, y - pole_height);

        SelectObject(hdc, sign_brush);
        Rectangle(hdc, left_x - sign_w - 8, y - pole_height / 2 - sign_h / 2,
                  left_x - 8, y - pole_height / 2 + sign_h / 2);
        Rectangle(hdc, right_x + 8, y - pole_height / 2 - sign_h / 2,
                  right_x + sign_w + 8, y - pole_height / 2 + sign_h / 2);
    }

    SelectObject(hdc, old_brush);
    SelectObject(hdc, old_pen);
    DeleteObject(pole_pen);
    DeleteObject(sign_brush);
    DeleteObject(light_brush);
}

void background_draw(HDC hdc, const Background *background)
{
    draw_sky(hdc);
    draw_city(hdc, background);
    draw_runway(hdc, background);
    draw_side_props(hdc, background);
}
