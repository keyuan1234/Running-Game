#include "player.h"
#include "resources.h"

static float lane_screen_x(Lane lane)
{
    return (float)(WINDOW_WIDTH / 2 + ((int)lane - 1) * LANE_BOTTOM_SPACING);
}

void player_init(Player *player)
{
    player->lane = LANE_CENTER;
    player->target_lane = LANE_CENTER;
    player->screen_x = lane_screen_x(LANE_CENTER);
    player->jump_height = 0.0f;
    player->vy = 0.0f;
    player->width = PLAYER_WIDTH;
    player->height = PLAYER_HEIGHT;
    player->hp = PLAYER_START_HP;
    player->animation_frame = 0;
    player->animation_timer = 0.0f;
    player->slide_timer = 0.0f;
    player->invincible_timer = 0.0f;
    player->action = PLAYER_RUNNING;
    player->on_ground = 1;
    player->jump_buffer_timer = 0.0f;
}

void player_change_lane(Player *player, int direction)
{
    int target = (int)player->target_lane + direction;
    if (target < LANE_LEFT) {
        target = LANE_LEFT;
    }
    if (target > LANE_RIGHT) {
        target = LANE_RIGHT;
    }
    player->target_lane = (Lane)target;
    player->lane = player->target_lane;
}

void player_start_jump(Player *player)
{
    if (player->on_ground && player->action != PLAYER_SLIDING) {
        player->vy = PLAYER_JUMP_SPEED;
        player->on_ground = 0;
        player->action = PLAYER_JUMPING;
        player->jump_buffer_timer = 0.0f;
    } else {
        player->jump_buffer_timer = JUMP_BUFFER_WINDOW;
    }
}

void player_start_crouch_or_dive(Player *player)
{
    if (player->on_ground) {
        player->action = PLAYER_SLIDING;
        player->slide_timer = SLIDE_DURATION;
    } else {
        player->action = PLAYER_AIR_DIVE;
        player->vy = PLAYER_DIVE_SPEED;
    }
}

void player_take_hit(Player *player)
{
    if (player->invincible_timer > 0.0f) {
        return;
    }
    player->hp -= 1;
    if (player->hp < 0) {
        player->hp = 0;
    }
    player->invincible_timer = INVINCIBLE_DURATION;
}

int player_is_invincible(const Player *player)
{
    return player->invincible_timer > 0.0f;
}

void player_update(Player *player, float dt)
{
    float target_x = lane_screen_x(player->target_lane);
    float delta_x = target_x - player->screen_x;
    float step = delta_x * LANE_MOVE_SPEED * dt;
    if (step > -1.0f && step < 1.0f) {
        player->screen_x = target_x;
    } else {
        player->screen_x += step;
    }

    if (player->invincible_timer > 0.0f) {
        player->invincible_timer -= dt;
        if (player->invincible_timer < 0.0f) {
            player->invincible_timer = 0.0f;
        }
    }

    if (player->jump_buffer_timer > 0.0f) {
        player->jump_buffer_timer -= dt;
        if (player->jump_buffer_timer < 0.0f) {
            player->jump_buffer_timer = 0.0f;
        }
    }

    if (!player->on_ground) {
        player->jump_height += player->vy * dt;
        player->vy -= GRAVITY * dt;
        if (player->jump_height <= 0.0f) {
            player->jump_height = 0.0f;
            player->vy = 0.0f;
            player->on_ground = 1;
            if (player->action == PLAYER_AIR_DIVE) {
                player->action = PLAYER_SLIDING;
                player->slide_timer = SLIDE_DURATION;
            } else if (player->jump_buffer_timer > 0.0f) {
                player->vy = PLAYER_JUMP_SPEED;
                player->on_ground = 0;
                player->action = PLAYER_JUMPING;
                player->jump_buffer_timer = 0.0f;
            }
        }
    }

    if (player->action == PLAYER_SLIDING) {
        player->slide_timer -= dt;
        if (player->slide_timer <= 0.0f) {
            player->slide_timer = 0.0f;
            if (player->on_ground) {
                player->action = PLAYER_RUNNING;
            }
        }
    } else if (player->on_ground) {
        player->action = PLAYER_RUNNING;
    } else if (player->action != PLAYER_AIR_DIVE) {
        player->action = PLAYER_JUMPING;
    }

    if (player->action == PLAYER_SLIDING) {
        player->width = PLAYER_SLIDE_WIDTH;
        player->height = PLAYER_SLIDE_HEIGHT;
    } else {
        player->width = PLAYER_WIDTH;
        player->height = PLAYER_HEIGHT;
    }

    player->animation_timer += dt;
    if (player->animation_timer >= 0.11f) {
        player->animation_timer = 0.0f;
        player->animation_frame = 1 - player->animation_frame;
    }
}

RECT player_get_rect(const Player *player)
{
    RECT rect;
    int visual_width = player->action == PLAYER_SLIDING ? PLAYER_SLIDE_WIDTH : PLAYER_WIDTH;
    int visual_height = player->action == PLAYER_SLIDING ? PLAYER_SLIDE_HEIGHT : PLAYER_HEIGHT;
    int x = (int)(lane_screen_x(player->target_lane) - visual_width / 2);
    int y = PLAYER_BASE_Y - visual_height - (int)player->jump_height;
    int margin_x = player->action == PLAYER_SLIDING ? 14 : 16;
    int margin_y = player->action == PLAYER_SLIDING ? 10 : 12;
    rect.left = x + margin_x;
    rect.top = y + margin_y;
    rect.right = x + visual_width - margin_x;
    rect.bottom = y + visual_height - 4;
    return rect;
}

static void draw_player_placeholder(HDC hdc, const Player *player)
{
    int x = (int)(player->screen_x - player->width / 2);
    int y = PLAYER_BASE_Y - player->height - (int)player->jump_height;
    int h = player->height;
    HBRUSH body = CreateSolidBrush(RGB(244, 139, 66));
    HBRUSH head = CreateSolidBrush(RGB(255, 216, 168));
    HBRUSH dark = CreateSolidBrush(RGB(44, 62, 80));
    HBRUSH accent = CreateSolidBrush(RGB(39, 129, 214));
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(39, 49, 63));
    HGDIOBJ old_brush = SelectObject(hdc, body);
    HGDIOBJ old_pen = SelectObject(hdc, pen);

    SelectObject(hdc, dark);
    Ellipse(hdc, x + 12, PLAYER_BASE_Y - 10, x + player->width - 12, PLAYER_BASE_Y + 2);

    if (player->action == PLAYER_SLIDING) {
        RoundRect(hdc, x + 10, y + 16, x + player->width - 8, y + h - 8, 14, 14);
        SelectObject(hdc, head);
        Ellipse(hdc, x + player->width - 38, y + 5, x + player->width - 8, y + 35);
        SelectObject(hdc, dark);
        Rectangle(hdc, x + 6, y + h - 16, x + 44, y + h - 6);
    } else {
        RoundRect(hdc, x + 22, y + 38, x + 60, y + h - 24, 14, 14);
        SelectObject(hdc, head);
        Ellipse(hdc, x + 23, y + 4, x + 59, y + 40);
        SelectObject(hdc, accent);
        Rectangle(hdc, x + 28, y + 52, x + 54, y + 70);
        SelectObject(hdc, dark);
        if (player->action == PLAYER_AIR_DIVE) {
            Rectangle(hdc, x + 8, y + 60, x + 32, y + 72);
            Rectangle(hdc, x + 52, y + 66, x + 76, y + 78);
        } else if (player->animation_frame == 0) {
            Rectangle(hdc, x + 14, y + h - 42, x + 31, y + h - 28);
            Rectangle(hdc, x + 48, y + h - 24, x + 64, y + h);
        } else {
            Rectangle(hdc, x + 9, y + h - 24, x + 26, y + h);
            Rectangle(hdc, x + 52, y + h - 42, x + 69, y + h - 28);
        }
    }

    if (player->invincible_timer > 0.0f) {
        HPEN aura_pen = CreatePen(PS_SOLID, 2, RGB(255, 244, 122));
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        SelectObject(hdc, aura_pen);
        Ellipse(hdc, x - 8, y - 8, x + player->width + 8, y + h + 8);
        SelectObject(hdc, old_pen);
        DeleteObject(aura_pen);
    } else {
        SelectObject(hdc, old_pen);
    }

    SelectObject(hdc, old_pen);
    SelectObject(hdc, old_brush);
    DeleteObject(pen);
    DeleteObject(body);
    DeleteObject(head);
    DeleteObject(dark);
    DeleteObject(accent);
}

void player_draw(HDC hdc, const Player *player, const Assets *assets)
{
    const Sprite *sprite = NULL;

    if (player->invincible_timer > 0.0f && ((int)(player->invincible_timer * 12.0f) % 2) == 0) {
        return;
    }

    if (player->action == PLAYER_JUMPING) {
        sprite = &assets->player_jump;
    } else if (player->action == PLAYER_SLIDING || player->action == PLAYER_AIR_DIVE) {
        sprite = &assets->player_crouch;
    } else {
        sprite = &assets->player_run[player->animation_frame];
    }

    if (sprite && sprite->loaded) {
        int x = (int)(player->screen_x - player->width / 2);
        int y = PLAYER_BASE_Y - player->height - (int)player->jump_height;
        sprite_draw(hdc, sprite, x, y, player->width, player->height);
    } else {
        draw_player_placeholder(hdc, player);
    }
}
