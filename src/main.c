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
#include "scores.h"
#include "sound.h"
#include "types.h"
#include "util.h"

static Game g_game;
static int g_running = 1;

static const char *char_name(CharacterType c)
{
    switch (c) {
    case CHAR_RUNNER:   return "Runner";
    case CHAR_SPRINTER: return "Sprinter";
    case CHAR_TANK:     return "Tank";
    case CHAR_JUMPER:   return "Jumper";
    default:            return "???";
    }
}

static const char *char_desc(CharacterType c)
{
    switch (c) {
    case CHAR_RUNNER:
        return "Balanced all-rounder. 3 HP. Jump buffer for smooth landings.";
    case CHAR_SPRINTER:
        return "Fragile speed demon. 2 HP. +10% speed, +40% speed gain.";
    case CHAR_TANK:
        return "Heavy juggernaut. 5 HP. Immune to low barriers. 5s invincibility.";
    case CHAR_JUMPER:
        return "Aerial acrobat. 3 HP. Double jump. Higher/longer air time.";
    default:
        return "";
    }
}

static COLORREF char_card_color(CharacterType c)
{
    switch (c) {
    case CHAR_RUNNER:   return RGB(244, 139, 66);
    case CHAR_SPRINTER: return RGB(76, 175, 80);
    case CHAR_TANK:     return RGB(70, 90, 130);
    case CHAR_JUMPER:   return RGB(156, 39, 176);
    default:            return RGB(128, 128, 128);
    }
}

static float char_speed_mult_ct(CharacterType c)
{
    switch (c) {
    case CHAR_SPRINTER: return CHAR_SPRINTER_SPEED;
    default:            return 1.0f;
    }
}

static int player_max_hp_ct(CharacterType c)
{
    switch (c) {
    case CHAR_SPRINTER: return CHAR_SPRINTER_HP;
    case CHAR_TANK:     return CHAR_TANK_HP;
    case CHAR_JUMPER:   return CHAR_JUMPER_HP;
    default:            return CHAR_RUNNER_HP;
    }
}

static void game_reset(Game *game)
{
    Assets keep_assets = game->assets;
    HighScores keep_scores = game->high_scores;
    CharacterType keep_character = game->character;
    memset(game, 0, sizeof(*game));
    game->assets = keep_assets;
    game->high_scores = keep_scores;
    game->character = keep_character;
    game->state = STATE_PLAYING;
    game->previous_state = STATE_MENU;
    game->base_speed = START_SPEED;
    game->current_speed = START_SPEED * char_speed_mult_ct(game->character);
    game->spawn_timer = 1.0f;
    game->last_obstacle_kind = -1;
    game->obstacle_kind_streak = 0;
    game->score = 0;
    game->score_accumulator = 0.0f;
    game->elapsed = 0.0f;
    game->obstacles_passed = 0;
    game->hit_freeze_timer = 0.0f;
    game->hit_flash_timer = 0.0f;
    game->latest_high_score_rank = 0;
    player_init(&game->player, game->character);
    background_init(&game->background);
    obstacles_init(game->obstacles, MAX_OBSTACLES);
}

static void game_init(Game *game)
{
    memset(game, 0, sizeof(*game));
    game->state = STATE_MENU;
    game->previous_state = STATE_MENU;
    game->character = CHAR_RUNNER;
    game->char_select_index = 0;
    game->last_obstacle_kind = -1;
    assets_load(&game->assets);
    scores_load(&game->high_scores);
    player_init(&game->player, CHAR_RUNNER);
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

static void draw_text_in_rect(HDC hdc, const char *text, RECT rect, int size, COLORREF color, int weight, UINT format)
{
    HFONT font;
    HFONT old_font;
    font = select_font(hdc, size, weight, &old_font);
    SetTextColor(hdc, color);
    DrawTextA(hdc, text, -1, &rect, format);
    SelectObject(hdc, old_font);
    DeleteObject(font);
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
    RECT panel;
    background_draw(hdc, &g_game.background);
    draw_overlay(hdc);

    panel.left = WINDOW_WIDTH / 2 - 320;
    panel.top = 60;
    panel.right = WINDOW_WIDTH / 2 + 320;
    panel.bottom = 470;
    draw_panel(hdc, panel, PANEL_COLOR, PANEL_BORDER, 24);

    draw_text_center(hdc, "SUBWAY RUNNER 2D", 99, 48, RGB(8, 14, 28), FW_BOLD);
    draw_text_center(hdc, "SUBWAY RUNNER 2D", 96, 48, RGB(244, 215, 118), FW_BOLD);

    draw_text_center(hdc, "1    Start Game", 225, 28, RGB(160, 210, 140), FW_BOLD);
    draw_text_center(hdc, "2    Game Help", 278, 28, RGB(160, 210, 140), FW_BOLD);
    draw_text_center(hdc, "3    Exit", 331, 28, RGB(210, 130, 120), FW_BOLD);
    draw_text_center(hdc, "4    Leaderboard", 384, 28, RGB(160, 180, 220), FW_BOLD);

    draw_text_center(hdc, "A/D: lanes    Space: jump    S: slide/dive    W: boost", 460, 18, RGB(140, 160, 180), FW_NORMAL);
}

static void draw_help(HDC hdc)
{
    RECT panel;
    background_draw(hdc, &g_game.background);
    draw_overlay(hdc);

    panel.left = WINDOW_WIDTH / 2 - 370;
    panel.top = 60;
    panel.right = WINDOW_WIDTH / 2 + 370;
    panel.bottom = 450;
    draw_panel(hdc, panel, PANEL_COLOR, PANEL_BORDER, 24);

    draw_text_center(hdc, "GAME HELP", 99, 44, RGB(8, 14, 28), FW_BOLD);
    draw_text_center(hdc, "GAME HELP", 96, 44, RGB(244, 215, 118), FW_BOLD);

    draw_text_center(hdc, "A / D: switch left and right lanes.", 190, 24, PANEL_TEXT_COLOR, FW_NORMAL);
    draw_text_center(hdc, "Space: jump over low barriers.", 236, 24, PANEL_TEXT_COLOR, FW_NORMAL);
    draw_text_center(hdc, "S: slide under high barriers; in air it dives down.", 282, 24, PANEL_TEXT_COLOR, FW_NORMAL);
    draw_text_center(hdc, "W: hold to boost. A hit gives 3 seconds of invincibility.", 328, 24, PANEL_TEXT_COLOR, FW_NORMAL);
    draw_text_center(hdc, "Jump buffer: press jump before landing to buffer the input.", 370, 22, RGB(160, 180, 210), FW_NORMAL);

    draw_text_center(hdc, "Press Esc to return to menu", 435, 22, RGB(210, 160, 140), FW_BOLD);
}

static void draw_hud(HDC hdc, const Game *game)
{
    char buffer[128];
    RECT hud_bar;
    HBRUSH hud_brush;
    HPEN hud_pen;
    HGDIOBJ old_brush;
    HGDIOBJ old_pen;
    int i;
    int gauge_width;

    hud_bar.left = 0;
    hud_bar.top = 0;
    hud_bar.right = WINDOW_WIDTH;
    hud_bar.bottom = 104;
    hud_brush = CreateSolidBrush(HUD_BAR_COLOR);
    hud_pen = CreatePen(PS_SOLID, 1, RGB(40, 55, 72));
    old_brush = SelectObject(hdc, hud_brush);
    old_pen = SelectObject(hdc, hud_pen);
    Rectangle(hdc, hud_bar.left, hud_bar.top, hud_bar.right, hud_bar.bottom);
    SelectObject(hdc, old_pen);
    SelectObject(hdc, old_brush);
    DeleteObject(hud_pen);
    DeleteObject(hud_brush);

    sprintf(buffer, "%06d", game->score);
    draw_text_left(hdc, buffer, 24, 8, 30, RGB(244, 215, 118), FW_BOLD);
    draw_text_left(hdc, "SCORE", 24, 44, 13, RGB(140, 160, 180), FW_NORMAL);

    sprintf(buffer, "[%s]", char_name(game->player.character));
    draw_text_left(hdc, buffer, 24, 62, 14, char_card_color(game->player.character), FW_BOLD);

    draw_text_left(hdc, "HP", 220, 14, 14, RGB(140, 160, 180), FW_NORMAL);
    for (i = 0; i < player_max_hp_ct(game->player.character); ++i) {
        COLORREF heart_color = i < game->player.hp ? RGB(210, 70, 60) : RGB(50, 50, 60);
        HPEN heart_pen = CreatePen(PS_SOLID, 2, heart_color);
        HBRUSH heart_brush = CreateSolidBrush(heart_color);
        int hx = 220 + i * 36;
        int hy = 32;
        SelectObject(hdc, heart_brush);
        SelectObject(hdc, heart_pen);
        Ellipse(hdc, hx, hy, hx + 14, hy + 14);
        Ellipse(hdc, hx + 16, hy, hx + 30, hy + 14);
        {
            POINT tri[3];
            tri[0].x = hx - 1;  tri[0].y = hy + 7;
            tri[1].x = hx + 15;  tri[1].y = hy + 28;
            tri[2].x = hx + 31;  tri[2].y = hy + 7;
            Polygon(hdc, tri, 3);
        }
        DeleteObject(heart_pen);
        DeleteObject(heart_brush);
    }

    sprintf(buffer, "Time: %.1fs", game->elapsed);
    draw_text_left(hdc, buffer, 460, 14, 22, PANEL_TEXT_COLOR, FW_BOLD);
    sprintf(buffer, "Passed: %d", game->obstacles_passed);
    draw_text_left(hdc, buffer, 460, 44, 16, RGB(140, 160, 180), FW_NORMAL);

    gauge_width = (int)(200.0f * (game->current_speed - START_SPEED) / (MAX_SPEED * BOOST_MULTIPLIER - START_SPEED));
    if (gauge_width < 0) gauge_width = 0;
    if (gauge_width > 200) gauge_width = 200;

    draw_text_left(hdc, "SPEED", 760, 14, 14, RGB(140, 160, 180), FW_NORMAL);
    {
        RECT gauge_bg = { 680, 36, 884, 50 };
        RECT gauge_fill = { 680, 36, 680 + gauge_width, 50 };
        HBRUSH gauge_bg_brush = CreateSolidBrush(RGB(20, 28, 38));
        HBRUSH gauge_fill_brush;
        HGDIOBJ old_gauge_brush;
        HPEN gauge_pen = CreatePen(PS_SOLID, 1, RGB(40, 55, 72));
        HGDIOBJ old_gauge_pen;

        if (game->input.boost_down) {
            gauge_fill_brush = CreateSolidBrush(RGB(244, 180, 60));
        } else {
            gauge_fill_brush = CreateSolidBrush(RGB(80, 150, 210));
        }

        old_gauge_brush = SelectObject(hdc, gauge_bg_brush);
        old_gauge_pen = SelectObject(hdc, gauge_pen);
        Rectangle(hdc, gauge_bg.left, gauge_bg.top, gauge_bg.right, gauge_bg.bottom);

        SelectObject(hdc, gauge_fill_brush);
        SelectObject(hdc, GetStockObject(NULL_PEN));
        if (gauge_width > 2) {
            Rectangle(hdc, gauge_fill.left + 1, gauge_fill.top + 1,
                      gauge_fill.right, gauge_fill.bottom - 1);
        }

        SelectObject(hdc, old_gauge_pen);
        SelectObject(hdc, old_gauge_brush);
        DeleteObject(gauge_pen);
        DeleteObject(gauge_bg_brush);
        DeleteObject(gauge_fill_brush);
    }

    if (game->input.boost_down) {
        draw_text_left(hdc, "BOOST", 700, 54, 16, RGB(244, 180, 60), FW_BOLD);
    }

    if (player_is_invincible(&game->player)) {
        sprintf(buffer, "INVINCIBLE %.1fs", game->player.invincible_timer);
        draw_text_center(hdc, buffer, 60, 18, RGB(255, 220, 80), FW_BOLD);
    }
}

static void draw_game_over(HDC hdc)
{
    char buffer[128];
    RECT panel;
    background_draw(hdc, &g_game.background);
    draw_overlay(hdc);

    panel.left = WINDOW_WIDTH / 2 - 320;
    panel.top = 80;
    panel.right = WINDOW_WIDTH / 2 + 320;
    panel.bottom = 460;
    draw_panel(hdc, panel, PANEL_COLOR, PANEL_BORDER, 24);

    draw_text_center(hdc, "GAME OVER", 123, 52, RGB(8, 14, 28), FW_BOLD);
    draw_text_center(hdc, "GAME OVER", 120, 52, RGB(210, 100, 80), FW_BOLD);

    sprintf(buffer, "Final Score: %d", g_game.score);
    draw_text_center(hdc, buffer, 210, 32, PANEL_HIGHLIGHT_COLOR, FW_BOLD);

    sprintf(buffer, "Survival Time: %.1fs", g_game.elapsed);
    draw_text_center(hdc, buffer, 262, 26, PANEL_TEXT_COLOR, FW_NORMAL);

    sprintf(buffer, "Obstacles Passed: %d", g_game.obstacles_passed);
    draw_text_center(hdc, buffer, 306, 24, PANEL_TEXT_COLOR, FW_NORMAL);

    if (g_game.latest_high_score_rank > 0) {
        sprintf(buffer, "NEW HIGH SCORE!  Rank #%d", g_game.latest_high_score_rank);
        draw_text_center(hdc, buffer, 170, 26, RGB(255, 220, 80), FW_BOLD);
    }

    draw_text_center(hdc, "R    Restart", 376, 28, RGB(160, 210, 140), FW_BOLD);
    draw_text_center(hdc, "L    Leaderboard", 418, 24, RGB(160, 180, 220), FW_BOLD);
    draw_text_center(hdc, "Esc  Main Menu", 455, 24, RGB(210, 130, 120), FW_BOLD);
}

static void draw_leaderboard(HDC hdc)
{
    RECT panel;
    int i;
    int y;
    int col_rank;
    int col_name;
    int col_score;
    int col_time;
    char buffer[128];

    background_draw(hdc, &g_game.background);
    draw_overlay(hdc);

    panel.left = WINDOW_WIDTH / 2 - 350;
    panel.top = 44;
    panel.right = WINDOW_WIDTH / 2 + 350;
    panel.bottom = 500;
    draw_panel(hdc, panel, PANEL_COLOR, PANEL_BORDER, 24);

    draw_text_center(hdc, "HIGH SCORES", 69, 44, RGB(8, 14, 28), FW_BOLD);
    draw_text_center(hdc, "HIGH SCORES", 66, 44, PANEL_TITLE_COLOR, FW_BOLD);

    col_rank = panel.left + 35;
    col_name = panel.left + 110;
    col_score = panel.left + 350;
    col_time = panel.left + 510;

    draw_text_left(hdc, "Rank", col_rank, 118, 18, RGB(140, 160, 180), FW_BOLD);
    draw_text_left(hdc, "Name", col_name, 118, 18, RGB(140, 160, 180), FW_BOLD);
    draw_text_left(hdc, "Score", col_score, 118, 18, RGB(140, 160, 180), FW_BOLD);
    draw_text_left(hdc, "Time", col_time, 118, 18, RGB(140, 160, 180), FW_BOLD);

    y = 154;

    if (g_game.high_scores.count == 0) {
        draw_text_center(hdc, "No scores yet -- play a game!", 260, 22, PANEL_TEXT_COLOR, FW_NORMAL);
    } else {
        for (i = 0; i < g_game.high_scores.count; ++i) {
            const HighScoreEntry *entry = &g_game.high_scores.entries[i];
            COLORREF row_color;
            COLORREF rank_color;

            if (g_game.latest_high_score_rank == i + 1) {
                row_color = PANEL_HIGHLIGHT_COLOR;
                rank_color = PANEL_HIGHLIGHT_COLOR;
                {
                    RECT hl_rect;
                    hl_rect.left = panel.left + 24;
                    hl_rect.top = y - 3;
                    hl_rect.right = panel.right - 24;
                    hl_rect.bottom = y + 29;
                    {
                        HBRUSH hl_brush = CreateSolidBrush(RGB(32, 36, 52));
                        HGDIOBJ old_hl = SelectObject(hdc, hl_brush);
                        HPEN hl_pen = CreatePen(PS_SOLID, 1, RGB(80, 70, 30));
                        HGDIOBJ old_hlp = SelectObject(hdc, hl_pen);
                        RoundRect(hdc, hl_rect.left, hl_rect.top, hl_rect.right, hl_rect.bottom, 8, 8);
                        SelectObject(hdc, old_hlp);
                        SelectObject(hdc, old_hl);
                        DeleteObject(hl_pen);
                        DeleteObject(hl_brush);
                    }
                }
            } else {
                row_color = PANEL_TEXT_COLOR;
                rank_color = i == 0 ? RGB(255, 215, 0) :
                            (i == 1 ? RGB(192, 192, 192) :
                            (i == 2 ? RGB(205, 127, 50) : RGB(120, 140, 160)));
            }

            sprintf(buffer, "%d", i + 1);
            draw_text_left(hdc, buffer, col_rank, y, 20, rank_color, FW_BOLD);

            draw_text_left(hdc, entry->name, col_name, y, 20, row_color, FW_NORMAL);

            sprintf(buffer, "%d", entry->score);
            draw_text_left(hdc, buffer, col_score, y, 20, row_color, FW_BOLD);

            sprintf(buffer, "%.1fs", entry->survival_time);
            draw_text_left(hdc, buffer, col_time, y, 20, row_color, FW_NORMAL);

            y += 34;
        }
    }

    draw_text_center(hdc, "Esc  Return", 470, 20, RGB(210, 160, 140), FW_BOLD);
}

static void draw_name_entry(HDC hdc)
{
    RECT panel;
    char display_name[32];
    char buffer[128];
    char cursor_char[2];

    background_draw(hdc, &g_game.background);
    draw_overlay(hdc);

    panel.left = WINDOW_WIDTH / 2 - 280;
    panel.top = 160;
    panel.right = WINDOW_WIDTH / 2 + 280;
    panel.bottom = 370;
    draw_panel(hdc, panel, PANEL_COLOR, PANEL_BORDER, 24);

    draw_text_center(hdc, "NEW HIGH SCORE!", 192, 26, PANEL_HIGHLIGHT_COLOR, FW_BOLD);

    draw_text_center(hdc, "Enter your name:", 248, 22, PANEL_TEXT_COLOR, FW_NORMAL);

    if (g_game.name_input_len == 0) {
        strcpy(display_name, "---");
    } else {
        strncpy(display_name, g_game.name_input, (size_t)g_game.name_input_len);
        display_name[g_game.name_input_len] = '\0';
    }

    cursor_char[0] = ((int)(g_game.name_entry_timer * 3.0f) % 2) == 0 ? '_' : ' ';
    cursor_char[1] = '\0';
    sprintf(buffer, "%s%s", display_name, cursor_char);

    draw_text_center(hdc, buffer, 296, 34, RGB(244, 215, 118), FW_BOLD);

    draw_text_center(hdc, "Enter: confirm    Backspace: delete", 344, 18, RGB(140, 160, 180), FW_NORMAL);

    sprintf(buffer, "Score: %d    Time: %.1fs    Passed: %d",
            g_game.score, g_game.elapsed, g_game.obstacles_passed);
    draw_text_center(hdc, buffer, 210, 18, PANEL_TEXT_COLOR, FW_NORMAL);
}

static void draw_char_select(HDC hdc)
{
    RECT panel;
    int i;
    int card_w = 200;
    int card_h = 280;
    int card_gap = 16;
    int total_w = CHARACTER_COUNT * card_w + (CHARACTER_COUNT - 1) * card_gap;
    int start_x = (WINDOW_WIDTH - total_w) / 2;
    int card_y = 130;

    background_draw(hdc, &g_game.background);
    draw_overlay(hdc);

    panel.left = WINDOW_WIDTH / 2 - total_w / 2 - 30;
    panel.top = 60;
    panel.right = WINDOW_WIDTH / 2 + total_w / 2 + 30;
    panel.bottom = card_y + card_h + 50;
    draw_panel(hdc, panel, PANEL_COLOR, PANEL_BORDER, 24);

    draw_text_center(hdc, "SELECT CHARACTER", 81, 48, RGB(8, 14, 28), FW_BOLD);
    draw_text_center(hdc, "SELECT CHARACTER", 78, 48, PANEL_TITLE_COLOR, FW_BOLD);

    for (i = 0; i < CHARACTER_COUNT; ++i) {
        CharacterType c = (CharacterType)i;
        int cx = start_x + i * (card_w + card_gap);
        COLORREF card_bg;
        COLORREF card_border;
        int is_selected = (i == g_game.char_select_index);

        if (is_selected) {
            card_bg = RGB(30, 36, 52);
            card_border = char_card_color(c);
        } else {
            card_bg = RGB(20, 24, 36);
            card_border = RGB(40, 50, 65);
        }

        {
            RECT card = { cx, card_y, cx + card_w, card_y + card_h };
            HPEN card_pen = CreatePen(PS_SOLID, is_selected ? 3 : 1, card_border);
            HBRUSH card_brush = CreateSolidBrush(card_bg);
            HGDIOBJ old_card_pen = SelectObject(hdc, card_pen);
            HGDIOBJ old_card_brush = SelectObject(hdc, card_brush);
            RoundRect(hdc, card.left, card.top, card.right, card.bottom, 16, 16);
            SelectObject(hdc, old_card_brush);
            SelectObject(hdc, old_card_pen);
            DeleteObject(card_brush);
            DeleteObject(card_pen);
        }

        {
            HBRUSH avatar_brush = CreateSolidBrush(char_card_color(c));
            HBRUSH head_brush = CreateSolidBrush(RGB(255, 216, 168));
            HPEN avatar_pen = CreatePen(PS_SOLID, 1, RGB(39, 49, 63));
            HGDIOBJ old_av_brush = SelectObject(hdc, avatar_brush);
            HGDIOBJ old_av_pen = SelectObject(hdc, avatar_pen);
            int ax = cx + card_w / 2;
            int ay = card_y + 60;

            Ellipse(hdc, ax - 8, ay + 40, ax + 8, ay + 52);

            SelectObject(hdc, head_brush);
            RoundRect(hdc, ax - 16, ay - 4, ax + 16, ay + 50, 12, 12);
            Ellipse(hdc, ax - 10, ay - 30, ax + 10, ay - 4);

            if (c == CHAR_SPRINTER) {
                HPEN trail = CreatePen(PS_SOLID, 2, RGB(255, 235, 59));
                HGDIOBJ old_trail = SelectObject(hdc, trail);
                MoveToEx(hdc, ax - 28, ay + 28, NULL);
                LineTo(hdc, ax - 48, ay + 16);
                MoveToEx(hdc, ax - 28, ay + 38, NULL);
                LineTo(hdc, ax - 44, ay + 42);
                SelectObject(hdc, old_trail);
                DeleteObject(trail);
            } else if (c == CHAR_TANK) {
                HPEN shield_pen = CreatePen(PS_SOLID, 3, RGB(200, 60, 60));
                HGDIOBJ old_shield_pen = SelectObject(hdc, shield_pen);
                HGDIOBJ old_shield_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
                Ellipse(hdc, ax - 26, ay - 16, ax + 26, ay + 60);
                SelectObject(hdc, old_shield_brush);
                SelectObject(hdc, old_shield_pen);
                DeleteObject(shield_pen);
            } else if (c == CHAR_JUMPER) {
                HPEN wing_pen = CreatePen(PS_SOLID, 2, RGB(0, 188, 212));
                HGDIOBJ old_wing = SelectObject(hdc, wing_pen);
                MoveToEx(hdc, ax - 20, ay + 14, NULL);
                LineTo(hdc, ax - 42, ay - 10);
                MoveToEx(hdc, ax - 18, ay + 28, NULL);
                LineTo(hdc, ax - 38, ay + 10);
                MoveToEx(hdc, ax + 20, ay + 14, NULL);
                LineTo(hdc, ax + 42, ay - 10);
                MoveToEx(hdc, ax + 18, ay + 28, NULL);
                LineTo(hdc, ax + 38, ay + 10);
                SelectObject(hdc, old_wing);
                DeleteObject(wing_pen);
            }

            SelectObject(hdc, old_av_brush);
            SelectObject(hdc, old_av_pen);
            DeleteObject(avatar_brush);
            DeleteObject(head_brush);
            DeleteObject(avatar_pen);
        }

        {
            char hp_buf[32];
            int hp_val;
            RECT name_rect = { cx, card_y + 4, cx + card_w, card_y + 34 };
            RECT hp_rect   = { cx, card_y + 128, cx + card_w, card_y + 152 };
            switch (c) {
            case CHAR_SPRINTER: hp_val = CHAR_SPRINTER_HP; break;
            case CHAR_TANK:     hp_val = CHAR_TANK_HP; break;
            case CHAR_JUMPER:   hp_val = CHAR_JUMPER_HP; break;
            default:            hp_val = CHAR_RUNNER_HP; break;
            }
            sprintf(hp_buf, "%d HP", hp_val);
            draw_text_in_rect(hdc, char_name(c), name_rect, 20,
                              is_selected ? PANEL_HIGHLIGHT_COLOR : PANEL_TEXT_COLOR, FW_BOLD,
                              DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            draw_text_in_rect(hdc, hp_buf, hp_rect, 15,
                              RGB(200, 100, 100), FW_BOLD,
                              DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        }

        {
            RECT desc_rect = { cx + 10, card_y + 160, cx + card_w - 10, card_y + card_h - 10 };
            HFONT desc_font = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                          ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                          DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            HFONT old_desc_font = (HFONT)SelectObject(hdc, desc_font);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(150, 165, 180));
            DrawTextA(hdc, char_desc(c), -1, &desc_rect, DT_CENTER | DT_WORDBREAK | DT_TOP);
            SelectObject(hdc, old_desc_font);
            DeleteObject(desc_font);
        }
    }

    draw_text_center(hdc, "A / D  select    Enter  confirm", card_y + card_h + 26, 18, RGB(140, 160, 180), FW_NORMAL);
}

static void game_update(Game *game, float dt)
{
    int i;
    RECT player_rect;

    if (dt > 0.05f) {
        dt = 0.05f;
    }

    if (game->hit_freeze_timer > 0.0f) {
        game->hit_freeze_timer -= dt;
        if (game->hit_freeze_timer < 0.0f) {
            game->hit_freeze_timer = 0.0f;
        }
        return;
    }

    if (game->hit_flash_timer > 0.0f) {
        game->hit_flash_timer -= dt;
        if (game->hit_flash_timer < 0.0f) {
            game->hit_flash_timer = 0.0f;
        }
    }

    if (game->state == STATE_MENU || game->state == STATE_HELP ||
        game->state == STATE_CHAR_SELECT ||
        game->state == STATE_GAME_OVER || game->state == STATE_LEADERBOARD ||
        game->state == STATE_NAME_ENTRY) {
        background_update(&game->background, dt, START_SPEED * 0.45f);
        if (game->state == STATE_NAME_ENTRY) {
            game->name_entry_timer += dt;
        }
        return;
    }

    game->elapsed += dt;
    {
        float gain_mult = player_speed_gain_multiplier(&game->player);
        game->base_speed += SPEED_GAIN_PER_SECOND * gain_mult * dt;
    }
    if (game->base_speed > MAX_SPEED) {
        game->base_speed = MAX_SPEED;
    }
    {
        float char_mult = player_speed_multiplier(&game->player);
        float effective = game->base_speed * char_mult;
        game->current_speed = game->input.boost_down ? effective * BOOST_MULTIPLIER : effective;
    }
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
            if (obstacle->kind == BARRIER_LOW && player_is_low_barrier_immune(&game->player)) {
                obstacle->hit = 1;
                continue;
            }
            obstacle->hit = 1;
            player_take_hit(&game->player);
            sound_play_hit(&game->assets);
            game->hit_freeze_timer = (float)HIT_FREEZE_MS / 1000.0f;
            game->hit_flash_timer = (float)HIT_FLASH_MS / 1000.0f;
            if (game->player.hp <= 0) {
                game->player.hp = 0;
                game->latest_high_score_rank = 0;
                if (scores_is_high_score(&game->high_scores, game->score)) {
                    game->name_input_len = 0;
                    game->name_entry_timer = 0.0f;
                    memset(game->name_input, 0, sizeof(game->name_input));
                    game->state = STATE_NAME_ENTRY;
                } else {
                    game->state = STATE_GAME_OVER;
                }
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
    } else if (g_game.state == STATE_CHAR_SELECT) {
        draw_char_select(hdc);
    } else if (g_game.state == STATE_GAME_OVER) {
        draw_game_over(hdc);
    } else if (g_game.state == STATE_LEADERBOARD) {
        draw_leaderboard(hdc);
    } else if (g_game.state == STATE_NAME_ENTRY) {
        draw_name_entry(hdc);
    } else {
        background_draw(hdc, &g_game.background);
        obstacles_draw(hdc, g_game.obstacles, MAX_OBSTACLES, &g_game.assets);
        player_draw(hdc, &g_game.player, &g_game.assets);
        draw_hud(hdc, &g_game);
    }

    if (g_game.hit_flash_timer > 0.0f && g_game.state == STATE_PLAYING) {
        RECT flash_rect;
        HBRUSH flash_brush;
        BLENDFUNCTION blend;
        HDC flash_dc;
        HBITMAP flash_bmp;
        HGDIOBJ old_flash_bmp;
        int alpha = (int)(160.0f * g_game.hit_flash_timer / ((float)HIT_FLASH_MS / 1000.0f));

        flash_rect.left = 0;
        flash_rect.top = 0;
        flash_rect.right = WINDOW_WIDTH;
        flash_rect.bottom = WINDOW_HEIGHT;

        flash_dc = CreateCompatibleDC(hdc);
        flash_bmp = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
        old_flash_bmp = SelectObject(flash_dc, flash_bmp);

        flash_brush = CreateSolidBrush(RGB(180, 20, 10));
        FillRect(flash_dc, &flash_rect, flash_brush);
        DeleteObject(flash_brush);

        memset(&blend, 0, sizeof(blend));
        blend.BlendOp = AC_SRC_OVER;
        blend.BlendFlags = 0;
        blend.SourceConstantAlpha = (BYTE)alpha;
        blend.AlphaFormat = 0;

        AlphaBlend(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                   flash_dc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, blend);

        SelectObject(flash_dc, old_flash_bmp);
        DeleteObject(flash_bmp);
        DeleteDC(flash_dc);
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
            g_game.char_select_index = 0;
            g_game.character = CHAR_RUNNER;
            g_game.state = STATE_CHAR_SELECT;
        } else if (key == '2') {
            g_game.state = STATE_HELP;
        } else if (key == '3') {
            g_running = 0;
            PostQuitMessage(0);
        } else if (key == '4') {
            g_game.previous_state = STATE_MENU;
            g_game.latest_high_score_rank = 0;
            g_game.state = STATE_LEADERBOARD;
        }
        return;
    }

    if (g_game.state == STATE_HELP) {
        if (key == VK_ESCAPE) {
            g_game.state = STATE_MENU;
        }
        return;
    }

    if (g_game.state == STATE_CHAR_SELECT) {
        if (key == VK_ESCAPE) {
            g_game.state = STATE_MENU;
        } else if ((key == 'A' || key == VK_LEFT) && first_press) {
            g_game.char_select_index -= 1;
            if (g_game.char_select_index < 0) {
                g_game.char_select_index = CHARACTER_COUNT - 1;
            }
        } else if ((key == 'D' || key == VK_RIGHT) && first_press) {
            g_game.char_select_index += 1;
            if (g_game.char_select_index >= CHARACTER_COUNT) {
                g_game.char_select_index = 0;
            }
        } else if (key == VK_RETURN || key == VK_SPACE) {
            g_game.character = (CharacterType)g_game.char_select_index;
            game_reset(&g_game);
        }
        return;
    }

    if (g_game.state == STATE_GAME_OVER) {
        if (key == 'R') {
            game_reset(&g_game);
        } else if (key == VK_ESCAPE) {
            g_game.state = STATE_MENU;
        } else if (key == 'L') {
            g_game.previous_state = STATE_GAME_OVER;
            g_game.latest_high_score_rank = scores_get_rank(&g_game.high_scores, g_game.score);
            g_game.state = STATE_LEADERBOARD;
        }
        return;
    }

    if (g_game.state == STATE_LEADERBOARD) {
        if (key == VK_ESCAPE) {
            g_game.state = g_game.previous_state;
            if (g_game.previous_state == STATE_MENU) {
                g_game.latest_high_score_rank = 0;
            }
            if (g_game.previous_state == STATE_GAME_OVER) {
                g_game.state = STATE_GAME_OVER;
            }
        }
        return;
    }

    if (g_game.state == STATE_NAME_ENTRY) {
        if (key == VK_RETURN) {
            if (g_game.name_input_len == 0) {
                strcpy(g_game.name_input, "AAA");
                g_game.name_input_len = 3;
            }
            g_game.latest_high_score_rank = scores_insert(&g_game.high_scores, g_game.name_input,
                                                           g_game.score, g_game.elapsed);
            scores_save(&g_game.high_scores);
            g_game.previous_state = STATE_MENU;
            g_game.state = STATE_LEADERBOARD;
        } else if (key == VK_BACK && g_game.name_input_len > 0) {
            g_game.name_input_len -= 1;
            g_game.name_input[g_game.name_input_len] = '\0';
        } else if (key >= 'A' && key <= 'Z' && g_game.name_input_len < NAME_INPUT_LENGTH && first_press) {
            g_game.name_input[g_game.name_input_len] = (char)key;
            g_game.name_input_len += 1;
            g_game.name_input[g_game.name_input_len] = '\0';
        } else if (key >= '0' && key <= '9' && g_game.name_input_len < NAME_INPUT_LENGTH && first_press) {
            g_game.name_input[g_game.name_input_len] = (char)key;
            g_game.name_input_len += 1;
            g_game.name_input[g_game.name_input_len] = '\0';
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
