#ifndef TYPES_H
#define TYPES_H

#include <windows.h>
#include "config.h"

typedef enum GameState {
    STATE_MENU = 0,
    STATE_HELP,
    STATE_PLAYING,
    STATE_GAME_OVER
} GameState;

typedef enum PlayerAction {
    PLAYER_RUNNING = 0,
    PLAYER_JUMPING,
    PLAYER_AIR_DIVE,
    PLAYER_SLIDING
} PlayerAction;

typedef enum Lane {
    LANE_LEFT = 0,
    LANE_CENTER = 1,
    LANE_RIGHT = 2
} Lane;

typedef enum ObstacleKind {
    BARRIER_LOW = 0,
    BARRIER_HIGH,
    TRAIN_BLOCK
} ObstacleKind;

typedef struct Sprite {
    HBITMAP bitmap;
    int width;
    int height;
    int loaded;
} Sprite;

typedef struct Assets {
    Sprite player_run[2];
    Sprite player_jump;
    Sprite player_crouch;
    Sprite obstacle_ground;
    Sprite obstacle_air;
    int hit_sound_available;
} Assets;

typedef struct Player {
    Lane lane;
    Lane target_lane;
    float screen_x;
    float jump_height;
    float vy;
    int width;
    int height;
    int hp;
    int animation_frame;
    float animation_timer;
    float slide_timer;
    float invincible_timer;
    PlayerAction action;
    int on_ground;
} Player;

typedef struct Obstacle {
    int active;
    Lane lane;
    ObstacleKind kind;
    float depth;
    float scale;
    int width;
    int height;
    int scored;
    int hit;
    RECT screen_rect;
} Obstacle;

typedef struct Background {
    float rail_offset;
    float sleeper_offset;
    float building_offset;
} Background;

typedef struct InputState {
    int move_left_request;
    int move_right_request;
    int jump_request;
    int crouch_request;
    int boost_down;
} InputState;

typedef struct Game {
    GameState state;
    Player player;
    Obstacle obstacles[MAX_OBSTACLES];
    Background background;
    Assets assets;
    InputState input;
    float base_speed;
    float current_speed;
    float spawn_timer;
    int last_obstacle_kind;
    int obstacle_kind_streak;
    int score;
    float score_accumulator;
    float elapsed;
} Game;

#endif
