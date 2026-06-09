#include "obstacle.h"
#include "resources.h"
#include "util.h"
#include <stdlib.h>

static int lane_to_index(Lane lane)
{
    return (int)lane - 1;
}

static float clamp_float(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static POINT project_lane_point(Lane lane, float depth)
{
    POINT point;
    float t = clamp_float(1.0f - depth, 0.0f, 1.0f);
    float perspective = t * t * (1.18f - 0.18f * t);
    point.x = WINDOW_WIDTH / 2 + (int)(lane_to_index(lane) * LANE_BOTTOM_SPACING * (0.18f + 0.82f * perspective));
    point.y = HORIZON_Y + (int)((RUNWAY_BOTTOM_Y - HORIZON_Y) * perspective);
    return point;
}

static void obstacle_project(Obstacle *obstacle)
{
    POINT base = project_lane_point(obstacle->lane, obstacle->depth);
    float t = clamp_float(1.0f - obstacle->depth, 0.0f, 1.0f);
    int draw_width;
    int draw_height;
    int bottom_gap;

    {
        float perspective = t * t * (1.18f - 0.18f * t);
        obstacle->scale = 0.14f + 1.18f * perspective;
    }

    if (obstacle->kind == BARRIER_LOW) {
        draw_width = (int)(LOW_BARRIER_WIDTH * obstacle->scale);
        draw_height = (int)(LOW_BARRIER_HEIGHT * obstacle->scale);
        bottom_gap = 0;
    } else if (obstacle->kind == BARRIER_HIGH) {
        draw_width = (int)(HIGH_BARRIER_WIDTH * obstacle->scale);
        draw_height = (int)(HIGH_BARRIER_HEIGHT * obstacle->scale);
        bottom_gap = (int)(HIGH_BARRIER_GAP * obstacle->scale);
    } else {
        draw_width = (int)(TRAIN_BLOCK_WIDTH * obstacle->scale);
        draw_height = (int)(TRAIN_BLOCK_HEIGHT * obstacle->scale);
        bottom_gap = 0;
    }

    if (draw_width < 8) {
        draw_width = 8;
    }
    if (draw_height < 8) {
        draw_height = 8;
    }

    obstacle->width = draw_width;
    obstacle->height = draw_height;
    obstacle->screen_rect.left = base.x - draw_width / 2;
    obstacle->screen_rect.right = base.x + draw_width / 2;
    obstacle->screen_rect.bottom = base.y - bottom_gap;
    obstacle->screen_rect.top = obstacle->screen_rect.bottom - draw_height;
}

void obstacles_init(Obstacle obstacles[], int count)
{
    int i;
    for (i = 0; i < count; ++i) {
        obstacles[i].active = 0;
        obstacles[i].lane = LANE_CENTER;
        obstacles[i].kind = BARRIER_LOW;
        obstacles[i].depth = OBSTACLE_START_DEPTH;
        obstacles[i].scale = 0.0f;
        obstacles[i].width = 0;
        obstacles[i].height = 0;
        obstacles[i].scored = 0;
        obstacles[i].hit = 0;
        obstacles[i].screen_rect.left = 0;
        obstacles[i].screen_rect.top = 0;
        obstacles[i].screen_rect.right = 0;
        obstacles[i].screen_rect.bottom = 0;
    }
}

static ObstacleKind random_kind(Game *game)
{
    int roll = rand() % 100;
    ObstacleKind kind;
    if (game->elapsed < 8.0f) {
        kind = roll < 58 ? BARRIER_LOW : BARRIER_HIGH;
    } else if (roll < 38) {
        kind = BARRIER_LOW;
    } else if (roll < 72) {
        kind = BARRIER_HIGH;
    } else {
        kind = TRAIN_BLOCK;
    }

    if (game->obstacle_kind_streak >= 2 && kind == (ObstacleKind)game->last_obstacle_kind) {
        if (kind == BARRIER_LOW) {
            kind = BARRIER_HIGH;
        } else if (kind == BARRIER_HIGH) {
            kind = TRAIN_BLOCK;
        } else {
            kind = BARRIER_LOW;
        }
    }
    return kind;
}

static void remember_kind(Game *game, ObstacleKind kind)
{
    if (game->last_obstacle_kind == (int)kind) {
        game->obstacle_kind_streak += 1;
    } else {
        game->last_obstacle_kind = (int)kind;
        game->obstacle_kind_streak = 1;
    }
}

static void spawn_single(Game *game, Lane lane, ObstacleKind kind)
{
    int i;
    for (i = 0; i < MAX_OBSTACLES; ++i) {
        Obstacle *obstacle = &game->obstacles[i];
        if (!obstacle->active) {
            obstacle->active = 1;
            obstacle->lane = lane;
            obstacle->kind = kind;
            obstacle->depth = OBSTACLE_START_DEPTH;
            obstacle->scale = 0.0f;
            obstacle->scored = 0;
            obstacle->hit = 0;
            obstacle_project(obstacle);
            return;
        }
    }
}

static int can_spawn_group(const Game *game)
{
    int i;
    float nearest_start_depth = -10.0f;
    for (i = 0; i < MAX_OBSTACLES; ++i) {
        const Obstacle *obstacle = &game->obstacles[i];
        if (obstacle->active && obstacle->depth > nearest_start_depth) {
            nearest_start_depth = obstacle->depth;
        }
    }
    if (nearest_start_depth < -9.0f) {
        return 1;
    }
    return nearest_start_depth <= OBSTACLE_START_DEPTH - MIN_GROUP_DEPTH_GAP;
}

static void spawn_obstacle_group(Game *game)
{
    int lane_a = rand() % LANE_COUNT;
    int allow_pair = game->elapsed > 18.0f && (rand() % 100) < 32;
    ObstacleKind kind_a = random_kind(game);
    spawn_single(game, (Lane)lane_a, kind_a);
    remember_kind(game, kind_a);

    if (allow_pair) {
        int lane_b = rand() % LANE_COUNT;
        ObstacleKind kind_b;
        while (lane_b == lane_a) {
            lane_b = rand() % LANE_COUNT;
        }
        kind_b = random_kind(game);
        spawn_single(game, (Lane)lane_b, kind_b);
        remember_kind(game, kind_b);
    }
}

void obstacles_update(Game *game, float dt)
{
    int i;
    game->spawn_timer -= dt;
    if (game->spawn_timer <= 0.0f && can_spawn_group(game)) {
        spawn_obstacle_group(game);
        game->spawn_timer = rand_float(MIN_SPAWN_INTERVAL, MAX_SPAWN_INTERVAL) * (START_SPEED / game->current_speed);
        if (game->spawn_timer < 0.62f) {
            game->spawn_timer = 0.62f;
        }
    }

    for (i = 0; i < MAX_OBSTACLES; ++i) {
        Obstacle *obstacle = &game->obstacles[i];
        if (!obstacle->active) {
            continue;
        }
        obstacle->depth -= game->current_speed * dt / 900.0f;
        obstacle_project(obstacle);
        if (!obstacle->scored && obstacle->depth < OBSTACLE_REMOVE_DEPTH * 0.35f) {
            obstacle->scored = 1;
            game->score += obstacle->kind == TRAIN_BLOCK ? 25 : 15;
            game->obstacles_passed += 1;
        }
        if (obstacle->depth < OBSTACLE_REMOVE_DEPTH) {
            obstacle->active = 0;
        }
    }
}

RECT obstacle_get_rect(const Obstacle *obstacle)
{
    RECT rect = obstacle->screen_rect;
    if (obstacle->kind == BARRIER_LOW) {
        int solid_height = (int)(obstacle->height * 0.82f);
        rect.left += 8;
        rect.right -= 8;
        rect.top = rect.bottom - solid_height;
        rect.bottom -= 5;
    } else if (obstacle->kind == BARRIER_HIGH) {
        rect.left += 10;
        rect.right -= 10;
        rect.top += 6;
        rect.bottom -= 4;
    } else {
        rect.left += 12;
        rect.right -= 12;
        rect.top += 10;
        rect.bottom -= 6;
    }
    return rect;
}

static void draw_ground_placeholder(HDC hdc, const Obstacle *obstacle)
{
    int x = obstacle->screen_rect.left;
    int y = obstacle->screen_rect.top;
    HBRUSH brush = CreateSolidBrush(RGB(176, 74, 74));
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(102, 36, 36));
    HGDIOBJ old_brush = SelectObject(hdc, brush);
    HGDIOBJ old_pen = SelectObject(hdc, pen);
    RoundRect(hdc, x, y, x + obstacle->width, y + obstacle->height, 8, 8);
    MoveToEx(hdc, x + 10, y + 12, NULL);
    LineTo(hdc, x + obstacle->width - 10, y + obstacle->height - 12);
    MoveToEx(hdc, x + obstacle->width - 10, y + 12, NULL);
    LineTo(hdc, x + 10, y + obstacle->height - 12);
    SelectObject(hdc, old_pen);
    SelectObject(hdc, old_brush);
    DeleteObject(pen);
    DeleteObject(brush);
}

static void draw_air_placeholder(HDC hdc, const Obstacle *obstacle)
{
    int x = obstacle->screen_rect.left;
    int y = obstacle->screen_rect.top;
    HBRUSH brush = CreateSolidBrush(RGB(94, 86, 178));
    HBRUSH light = CreateSolidBrush(RGB(149, 196, 238));
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(48, 43, 106));
    HGDIOBJ old_brush = SelectObject(hdc, brush);
    HGDIOBJ old_pen = SelectObject(hdc, pen);
    RoundRect(hdc, x, y, x + obstacle->width, y + obstacle->height, 10, 10);
    SelectObject(hdc, light);
    Rectangle(hdc, x + obstacle->width / 8, y + obstacle->height / 3,
              x + obstacle->width * 7 / 8, y + obstacle->height * 2 / 3);
    SelectObject(hdc, brush);
    Rectangle(hdc, x + 6, y + obstacle->height - 6, x + 18, y + obstacle->height + 18);
    Rectangle(hdc, x + obstacle->width - 18, y + obstacle->height - 6,
              x + obstacle->width - 6, y + obstacle->height + 18);
    SelectObject(hdc, old_pen);
    SelectObject(hdc, old_brush);
    DeleteObject(pen);
    DeleteObject(brush);
    DeleteObject(light);
}

static void draw_train_placeholder(HDC hdc, const Obstacle *obstacle)
{
    int x = obstacle->screen_rect.left;
    int y = obstacle->screen_rect.top;
    HBRUSH body = CreateSolidBrush(RGB(50, 83, 112));
    HBRUSH window = CreateSolidBrush(RGB(163, 214, 232));
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(28, 43, 58));
    HGDIOBJ old_brush = SelectObject(hdc, body);
    HGDIOBJ old_pen = SelectObject(hdc, pen);
    RoundRect(hdc, x, y, x + obstacle->width, y + obstacle->height, 12, 12);
    SelectObject(hdc, window);
    Rectangle(hdc, x + obstacle->width / 6, y + obstacle->height / 6,
              x + obstacle->width * 5 / 6, y + obstacle->height / 3);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, x + 8, y + obstacle->height - 22, x + 24, y + obstacle->height - 6);
    Rectangle(hdc, x + obstacle->width - 24, y + obstacle->height - 22,
              x + obstacle->width - 8, y + obstacle->height - 6);
    SelectObject(hdc, old_pen);
    SelectObject(hdc, old_brush);
    DeleteObject(pen);
    DeleteObject(body);
    DeleteObject(window);
}

void obstacles_draw(HDC hdc, const Obstacle obstacles[], int count, const Assets *assets)
{
    int drawn[MAX_OBSTACLES];
    int draw_count = 0;
    int i;
    for (i = 0; i < count; ++i) {
        drawn[i] = 0;
    }
    while (draw_count < count) {
        int best = -1;
        float best_depth = -999.0f;
        for (i = 0; i < count; ++i) {
            if (!drawn[i] && obstacles[i].active && obstacles[i].depth > best_depth) {
                best = i;
                best_depth = obstacles[i].depth;
            }
        }
        if (best < 0) {
            break;
        }
        drawn[best] = 1;
        draw_count += 1;
        {
            const Obstacle *obstacle = &obstacles[best];
            const Sprite *sprite = NULL;
        sprite = obstacle->kind == BARRIER_HIGH ? &assets->obstacle_air : &assets->obstacle_ground;
        if (sprite && sprite->loaded) {
            sprite_draw(hdc, sprite, obstacle->screen_rect.left, obstacle->screen_rect.top,
                        obstacle->width, obstacle->height);
        } else if (obstacle->kind == BARRIER_LOW) {
            draw_ground_placeholder(hdc, obstacle);
        } else if (obstacle->kind == BARRIER_HIGH) {
            draw_air_placeholder(hdc, obstacle);
        } else {
            draw_train_placeholder(hdc, obstacle);
        }
#if DEBUG_HITBOX
        {
            RECT hitbox = obstacle_get_rect(obstacle);
            HPEN hit_pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 0));
            HGDIOBJ old_pen = SelectObject(hdc, hit_pen);
            HGDIOBJ old_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, hitbox.left, hitbox.top, hitbox.right, hitbox.bottom);
            SelectObject(hdc, old_brush);
            SelectObject(hdc, old_pen);
            DeleteObject(hit_pen);
        }
#endif
        }
    }
}
