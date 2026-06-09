#ifndef PLAYER_H
#define PLAYER_H

#include <windows.h>
#include "types.h"

void player_init(Player *player, CharacterType character);
void player_change_lane(Player *player, int direction);
void player_start_jump(Player *player);
void player_start_crouch_or_dive(Player *player);
void player_take_hit(Player *player);
int player_is_invincible(const Player *player);
int player_is_low_barrier_immune(const Player *player);
void player_update(Player *player, float dt);
void player_draw(HDC hdc, const Player *player, const Assets *assets);
RECT player_get_rect(const Player *player);

float player_gravity(const Player *player);
float player_jump_speed(const Player *player);
float player_invincible_duration(const Player *player);
float player_speed_multiplier(const Player *player);
float player_speed_gain_multiplier(const Player *player);

#endif
