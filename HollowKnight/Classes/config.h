#ifndef __GAME_CONFIG_H__
#define __GAME_CONFIG_H__

#include "cocos2d.h"

namespace Config {
    namespace Player {
        // 物理参数
        const float MOVE_SPEED = 300.0f;
        const float JUMP_FORCE = 700.0f;
        const float GRAVITY = 2000.0f;
        const float MAX_FALL_SPEED = -1500.0f;

        // 手感参数
        const float MAX_JUMP_TIME = 0.35f;      // 长按跳跃最大时间
        const float JUMP_ACCEL = 3000.0f;       // 长按上升加速度
        const float JUMP_FORCE_BASE = 400.0f;

        // 战斗参数
        const float ATTACK_COOLDOWN = 0.04f;
        const int MAX_HEALTH = 5;
    }

    namespace Path {
        static const char* PLAYER_IDLE = "Knight/idle/idle_%d.png";
        static const char* PLAYER_RUN = "Knight/run/run_%d.png";
        static const char* PLAYER_JUMP = "Knight/jump/jump_%d.png";
        static const char* PLAYER_FALL = "Knight/fall/fall_%d.png";
        static const char* PLAYER_SLASH = "Knight/slash/slash_%d.png";
        static const char* PLAYER_SLASH_EFFECT = "Knight/slash/slashEffect/slashEffect_%d.png";
        static const char* PLAYER_DAMAGE = "Knight/damage/damage_%d.png";
        static const char* PLAYER_LOOKUP = "Knight/LookUp/LookUp_%d.png";
        static const char* PLAYER_LOOKDOWN = "Knight/LookDown/LookDown_%d.png";
        static const char* PLAYER_UPSLASH = "Knight/upslash/upslash_%d.png";
        static const char* PLAYER_DOWNSLASH = "Knight/downslash/downslash_%d.png";
        static const char* PLAYER_UP_SLASH_EFFECT = "Knight/upslash/upslasheffect/upslasheffect_%d.png";
        static const char* PLAYER_DOWN_SLASH_EFFECT = "Knight/downslash/downslasheffect/downslasheffect_%d.png";
    }

    namespace Render {
        const int Z_ORDER_PLAYER = 10;
        const int Z_ORDER_ENEMY = 5;
        const int Z_ORDER_MAP = -99;
    }
}

#endif