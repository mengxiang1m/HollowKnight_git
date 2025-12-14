#ifndef __SPIKE_H__
#define __SPIKE_H__

#include "cocos2d.h"

USING_NS_CC;

class Spike : public cocos2d::Sprite
{
public:
    // 刺的状态枚举
    enum class State {
        IDLE,       // 静止（等待触发）
        FALLING,    // 下落中
        DEAD        // 已消失
    };

    // 创建刺的静态方法
    static Spike* create(const std::string& filename);

    // 初始化
    virtual bool init() override;

    // 每帧更新 (需要玩家位置)
    void update(float dt, const cocos2d::Vec2& playerPos, const std::vector<cocos2d::Rect>& platforms);

    // 获取碰撞盒
    cocos2d::Rect getHitbox() const;

    // 设置刺的初始位置（用于重置）
    void setInitialPosition(const cocos2d::Vec2& pos);

    // 析构函数
    virtual ~Spike();

private:
    // 状态管理
    State _currentState;
    void changeState(State newState);

    // AI逻辑
    bool isPlayerBelowSpike(const cocos2d::Vec2& playerPos);
    void updateFallingBehavior(float dt);

    // 物理系统
    void updateMovementY(float dt);
    void updateCollisionY(const std::vector<cocos2d::Rect>& platforms);

    // 检测范围
    float _detectionRange;      // 检测玩家的水平范围

    // 物理属性
    cocos2d::Vec2 _velocity;    // 速度（主要是Y轴）
    float _gravity;             // 重力加速度
    float _maxFallSpeed;        // 最大下落速度

    // 初始位置（用于重置）
    cocos2d::Vec2 _initialPosition;

    // 伤害值
    int _damage;
};

#endif // __SPIKE_H__
