#pragma once
// 这是一个纯虚基类，不要在这里 include Player.h
// 而是使用前向声明 (Forward Declaration)

class Player; // 关键：告诉编译器 Player 是个类，但先别问细节

class PlayerState {
public:
    virtual ~PlayerState() {}
    virtual void enter(Player* player) = 0;
    virtual void update(Player* player, float dt) = 0;
    virtual void exit(Player* player) = 0;
};