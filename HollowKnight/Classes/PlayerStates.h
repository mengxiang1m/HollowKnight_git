#ifndef __PLAYER_STATES_H__
#define __PLAYER_STATES_H__

#include "PlayerState.h" // 包含基类

// =========================
// 待机
// =========================
class StateIdle : public PlayerState {
public:
    void enter(Player* player) override;
    void update(Player* player, float dt) override;
    void exit(Player* player) override;
};

// =========================
// 跑步
// =========================
class StateRun : public PlayerState {
public:
    void enter(Player* player) override;
    void update(Player* player, float dt) override;
    void exit(Player* player) override;
};

// =========================
// 跳跃 (上升)
// =========================
class StateJump : public PlayerState {
public:
    void enter(Player* player) override;
    void update(Player* player, float dt) override;
    void exit(Player* player) override;
};

// =========================
// 下落
// =========================
class StateFall : public PlayerState {
public:
    void enter(Player* player) override;
    void update(Player* player, float dt) override;
    void exit(Player* player) override;
};

// =========================
// 攻击
// =========================
class StateSlash : public PlayerState {
public:
    void enter(Player* player) override;
    void update(Player* player, float dt) override;
    void exit(Player* player) override;
private:
    float _timer;
    float _duration;
};

// =========================
// 受伤
// =========================
class StateDamaged : public PlayerState {
public:
    void enter(Player* player) override;
    void update(Player* player, float dt) override;
    void exit(Player* player) override;
private:
    float _timer;
    float _duration;
};

// =========================
// 仰望
// =========================
class StateLookUp : public PlayerState {
public:
    void enter(Player* player) override;
    void update(Player* player, float dt) override;
    void exit(Player* player) override;
private:
    float _timer;
    float _duration;
};

// =========================
// 俯视
// =========================
class StateLookDown : public PlayerState {
public:
    void enter(Player* player) override;
    void update(Player* player, float dt) override;
    void exit(Player* player) override;
private:
    float _timer;
    float _duration;
};

// 上劈状态
class StateSlashUp : public PlayerState {
public:
    void enter(Player* player) override;
    void update(Player* player, float dt) override;
    void exit(Player* player) override;
private:
    float _timer; float _duration;
};

// 下劈状态
class StateSlashDown : public PlayerState {
public:
    void enter(Player* player) override;
    void update(Player* player, float dt) override;
    void exit(Player* player) override;
private:
    float _timer; float _duration;
};
#endif // __PLAYER_STATES_H__