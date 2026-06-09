#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "background.h"
#include "config.h"
#include "obstacle.h"
#include "player.h"
#include "resources.h"
#include "sound.h"
#include "types.h"
#include "util.h"

static Game g_game;
static int g_running = 1;

static void game_reset(Game *game)
{
    Assets keep_assets = game->assets;
    memset(game, 0, sizeof(*game));
    game->assets = keep_assets;
    game->state = STATE_PLAYING;
    game->base_speed = START_SPEED;
    game->current_speed = START_SPEED;
    game->spawn_timer = 1.0f;
    game->last_obstacle_kind = -1;
    game->obstacle_kind_streak = 0;
    game->score = 0;
    game->score_accumulator = 0.0f;
    game->elapsed = 0.0f;
    player_init(&game->player);
    background_init(&game->background);
    obstacles_init(game->obstacles, MAX_OBSTACLES);
}

static void game_init(Game *game)
{
    memset(game, 0, sizeof(*game));
    game->state = STATE_MENU;
    game->last_obstacle_kind = -1;
    assets_load(&game->assets);
    player_init(&game->player);
    background_init(&game->background);
    obstacles_init(game->obstacles, MAX_OBSTACLES);
}

static HFONT select_font(HDC hdc, int size, int weight, HFONT *old_font)
{
    HFONT font = CreateFontA(size, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    *old_font = (HFONT)SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);
    return font;
}

static void draw_text_center(HDC hdc, const char *text, int y, int size, COLORREF color, int weight)
{
    RECT rect;
    HFONT font;
    HFONT old_font;
    rect.left = 0;
    rect.top = y;
    rect.right = WINDOW_WIDTH;
    rect.bottom = y + size + 20;
    font = select_font(hdc, size, weight, &old_font);
    SetTextColor(hdc, color);
    DrawTextA(hdc, text, -1, &rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    SelectObject(hdc, old_font);
    DeleteObject(font);
}

static void draw_text_left(HDC hdc, const char *text, int x, int y, int size, COLORREF color, int weight)
{
    HFONT font;
    HFONT old_font;
    font = select_font(hdc, size, weight, &old_font);
    SetTextColor(hdc, color);
    TextOutA(hdc, x, y, text, (int)strlen(text));
    SelectObject(hdc, old_font);
    DeleteObject(font);
}

static void draw_menu(HDC hdc)
{
    background_draw(hdc, &g_game.background);
    draw_text_center(hdc, "SUBWAY RUNNER 2D", 84, 52, RGB(28, 44, 72), FW_BOLD);
    draw_text_center(hdc, "1  Start Game", 215, 30, RGB(30, 76, 46), FW_BOLD);
    draw_text_center(hdc, "2  Game Help", 270, 30, RGB(30, 76, 46), FW_BOLD);
    draw_text_center(hdc, "3  Exit", 325, 30, RGB(134, 52, 52), FW_BOLD);
    draw_text_center(hdc, "A/D: lanes    Space: jump    S: slide/dive    W: boost", 458, 20, RGB(60, 79, 86), FW_NORMAL);
}

static void draw_help(HDC hdc)
{
    background_draw(hdc, &g_game.background);
    draw_text_center(hdc, "GAME HELP", 80, 44, RGB(28, 44, 72), FW_BOLD);
    draw_text_left(hdc, "A / D: switch left and right lanes.", 210, 165, 22, RGB(34, 55, 64), FW_NORMAL);
    draw_text_left(hdc, "Space: jump over low barriers.", 210, 215, 22, RGB(34, 55, 64), FW_NORMAL);
    draw_text_left(hdc, "S: slide under high barriers; in air it dives down fast.", 210, 265, 22, RGB(34, 55, 64), FW_NORMAL);
    draw_text_left(hdc, "W: hold to boost. A hit gives 3 seconds of invincibility.", 210, 315, 22, RGB(34, 55, 64), FW_NORMAL);
    draw_text_center(hdc, "Press Esc to return to menu", 440, 22, RGB(134, 52, 52), FW_BOLD);
}

static void draw_hud(HDC hdc, const Game *game)
{
    char buffer[128];
    sprintf(buffer, "Score: %d", game->score);
    draw_text_left(hdc, buffer, 24, 18, 22, RGB(24, 44, 62), FW_BOLD);
    sprintf(buffer, "HP: %d", game->player.hp);
    draw_text_left(hdc, buffer, 24, 48, 22, RGB(134, 52, 52), FW_BOLD);
    sprintf(buffer, "Time: %.1fs", game->elapsed);
    draw_text_left(hdc, buffer, 780, 18, 22, RGB(24, 44, 62), FW_BOLD);
    sprintf(buffer, "Speed: %.0f%s", game->current_speed, game->input.boost_down ? " BOOST" : "");
    draw_text_left(hdc, buffer, 760, 48, 20, RGB(24, 44, 62), FW_BOLD);
    sprintf(buffer, "Lane: %s", game->player.target_lane == LANE_LEFT ? "Left" :
            (game->player.target_lane == LANE_RIGHT ? "Right" : "Center"));
    draw_text_left(hdc, buffer, 24, 78, 20, RGB(24, 44, 62), FW_BOLD);
    if (player_is_invincible(&game->player)) {
        sprintf(buffer, "INVINCIBLE %.1fs", game->player.invincible_timer);
        draw_text_center(hdc, buffer, 18, 22, RGB(176, 82, 22), FW_BOLD);
    }
}

static void draw_game_over(HDC hdc)
{
    char buffer[128];
    background_draw(hdc, &g_game.background);
    draw_text_center(hdc, "GAME OVER", 110, 52, RGB(128, 40, 40), FW_BOLD);
    sprintf(buffer, "Final Score: %d", g_game.score);
    draw_text_center(hdc, buffer, 220, 30, RGB(28, 44, 72), FW_BOLD);
    sprintf(buffer, "Survival Time: %.1fs", g_game.elapsed);
    draw_text_center(hdc, buffer, 270, 26, RGB(28, 44, 72), FW_NORMAL);
    draw_text_center(hdc, "R  Restart", 350, 26, RGB(30, 76, 46), FW_BOLD);
    draw_text_center(hdc, "Esc  Main Menu", 395, 24, RGB(134, 52, 52), FW_BOLD);
}

static void game_update(Game *game, float dt)
{
    int i;
    RECT player_rect;

    if (dt > 0.05f) {
        dt = 0.05f;
    }

    if (game->state == STATE_MENU || game->state == STATE_HELP || game->state == STATE_GAME_OVER) {
        background_update(&game->background, dt, START_SPEED * 0.45f);
        return;
    }

    game->elapsed += dt;
    game->base_speed += SPEED_GAIN_PER_SECOND * dt;
    if (game->base_speed > MAX_SPEED) {
        game->base_speed = MAX_SPEED;
    }
    game->current_speed = game->input.boost_down ? game->base_speed * BOOST_MULTIPLIER : game->base_speed;
    game->score_accumulator += dt * (game->input.boost_down ? 3.0f : 2.0f);
    if (game->score_accumulator >= 1.0f) {
        int add = (int)game->score_accumulator;
        game->score += add;
        game->score_accumulator -= (float)add;
    }

    if (game->input.move_left_request) {
        player_change_lane(&game->player, -1);
    }
    if (game->input.move_right_request) {
        player_change_lane(&game->player, 1);
    }
    if (game->input.jump_request) {
        player_start_jump(&game->player);
    }
    if (game->input.crouch_request) {
        player_start_crouch_or_dive(&game->player);
    }
    game->input.move_left_request = 0;
    game->input.move_right_request = 0;
    game->input.jump_request = 0;
    game->input.crouch_request = 0;

    background_update(&game->background, dt, game->current_speed);
    player_update(&game->player, dt);
    obstacles_update(game, dt);

    player_rect = player_get_rect(&game->player);
    for (i = 0; i < MAX_OBSTACLES; ++i) {
        Obstacle *obstacle = &game->obstacles[i];
        if (!obstacle->active || obstacle->hit) {
            continue;
        }
        if (obstacle->lane == game->player.target_lane &&
            obstacle->depth <= HIT_DEPTH_MAX &&
            obstacle->depth >= HIT_DEPTH_MIN &&
            rects_overlap(player_rect, obstacle_get_rect(obstacle)) &&
            !player_is_invincible(&game->player)) {
            obstacle->hit = 1;
            player_take_hit(&game->player);
            sound_play_hit(&game->assets);
            if (game->player.hp <= 0) {
                game->player.hp = 0;
                game->state = STATE_GAME_OVER;
                break;
            }
        }
    }
}

static void game_render(HDC hdc)
{
    if (g_game.state == STATE_MENU) {
        draw_menu(hdc);
    } else if (g_game.state == STATE_HELP) {
        draw_help(hdc);
    } else if (g_game.state == STATE_GAME_OVER) {
        draw_game_over(hdc);
    } else {
        background_draw(hdc, &g_game.background);
        obstacles_draw(hdc, g_game.obstacles, MAX_OBSTACLES, &g_game.assets);
        player_draw(hdc, &g_game.player, &g_game.assets);
        draw_hud(hdc, &g_game);
    }
}

static void render_frame(HWND hwnd)
{
    HDC window_dc;
    HDC memory_dc;
    HBITMAP back_bitmap;
    HGDIOBJ old_bitmap;

    window_dc = GetDC(hwnd);
    memory_dc = CreateCompatibleDC(window_dc);
    back_bitmap = CreateCompatibleBitmap(window_dc, WINDOW_WIDTH, WINDOW_HEIGHT);
    old_bitmap = SelectObject(memory_dc, back_bitmap);

    game_render(memory_dc);
    BitBlt(window_dc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, memory_dc, 0, 0, SRCCOPY);

    SelectObject(memory_dc, old_bitmap);
    DeleteObject(back_bitmap);
    DeleteDC(memory_dc);
    ReleaseDC(hwnd, window_dc);
}

static void handle_key_down(WPARAM key, LPARAM lparam)
{
    int first_press = (lparam & 0x40000000) == 0;

    if (g_game.state == STATE_MENU) {
        if (key == '1') {
            game_reset(&g_game);
        } else if (key == '2') {
            g_game.state = STATE_HELP;
        } else if (key == '3') {
            g_running = 0;
            PostQuitMessage(0);
        }
        return;
    }

    if (g_game.state == STATE_HELP) {
        if (key == VK_ESCAPE) {
            g_game.state = STATE_MENU;
        }
        return;
    }

    if (g_game.state == STATE_GAME_OVER) {
        if (key == 'R') {
            game_reset(&g_game);
        } else if (key == VK_ESCAPE) {
            g_game.state = STATE_MENU;
        }
        return;
    }

    if (g_game.state == STATE_PLAYING) {
        if (key == 'A' && first_press) {
            g_game.input.move_left_request = 1;
        } else if (key == 'D' && first_press) {
            g_game.input.move_right_request = 1;
        } else if (key == VK_SPACE && first_press) {
            g_game.input.jump_request = 1;
        } else if (key == 'S' && first_press) {
            g_game.input.crouch_request = 1;
        } else if (key == 'W') {
            g_game.input.boost_down = 1;
        } else if (key == VK_ESCAPE) {
            g_game.state = STATE_MENU;
        }
    }
}

static void handle_key_up(WPARAM key)
{
    if (key == 'W') {
        g_game.input.boost_down = 0;
    }
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case WM_KEYDOWN:
        handle_key_down(wparam, lparam);
        return 0;
    case WM_KEYUP:
        handle_key_up(wparam);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        g_running = 0;
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcA(hwnd, message, wparam, lparam);
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance, LPSTR command_line, int show_command)
{
    WNDCLASSA window_class;
    HWND hwnd;
    RECT window_rect;
    DWORD previous_time;
    MSG message;

    (void)previous_instance;
    (void)command_line;

    srand((unsigned int)time(NULL));
    game_init(&g_game);

    memset(&window_class, 0, sizeof(window_class));
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = "StrictCRunningGameWindow";
    window_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassA(&window_class)) {
        MessageBoxA(NULL, "Failed to register window class.", "Running Game", MB_ICONERROR);
        assets_free(&g_game.assets);
        return 1;
    }

    window_rect.left = 0;
    window_rect.top = 0;
    window_rect.right = WINDOW_WIDTH;
    window_rect.bottom = WINDOW_HEIGHT;
    AdjustWindowRect(&window_rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);

    hwnd = CreateWindowA("StrictCRunningGameWindow",
                         "Running Game - Strict C Win32/GDI",
                         WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                         CW_USEDEFAULT, CW_USEDEFAULT,
                         window_rect.right - window_rect.left,
                         window_rect.bottom - window_rect.top,
                         NULL, NULL, instance, NULL);

    if (!hwnd) {
        MessageBoxA(NULL, "Failed to create window.", "Running Game", MB_ICONERROR);
        assets_free(&g_game.assets);
        return 1;
    }

    ShowWindow(hwnd, show_command);
    UpdateWindow(hwnd);

    previous_time = GetTickCount();
    while (g_running) {
        DWORD frame_start = GetTickCount();
        DWORD now;
        float dt;

        while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                g_running = 0;
                break;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        now = GetTickCount();
        dt = (float)(now - previous_time) / 1000.0f;
        previous_time = now;

        game_update(&g_game, dt);
        render_frame(hwnd);

        {
            DWORD frame_time = GetTickCount() - frame_start;
            if (frame_time < FRAME_TIME_MS) {
                Sleep(FRAME_TIME_MS - frame_time);
            }
        }
    }

    assets_free(&g_game.assets);
    return 0;
}
