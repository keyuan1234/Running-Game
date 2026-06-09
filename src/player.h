#ifndef PLAYER_H
#define PLAYER_H

#include <windows.h>
#include "types.h"

void player_init(Player *player);
void player_change_lane(Player *player, int direction);
void player_start_jump(Player *player);
void player_start_crouch_or_dive(Player *player);
void player_take_hit(Player *player);
int player_is_invincible(const Player *player);
void player_update(Player *player, float dt);
void player_draw(HDC hdc, const Player *player, const Assets *assets);
RECT player_get_rect(const Player *player);

#endif
