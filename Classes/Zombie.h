#ifndef __ZOMBIE_H__
#define __ZOMBIE_H__

#include "cocos2d.h"

USING_NS_CC;

class Zombie : public cocos2d::Sprite
{
public:
    // 僵尸状态枚举
    enum class State {
        PATROL,         // 巡逻
        ATTACK_READY,   // 准备攻击
        ATTACKING,      // 攻击中
        DAMAGED,        // 受伤
        DEAD            // 死亡
    };

    // 创建僵尸的静态方法
    static Zombie* create(const std::string& filename);

    // 初始化
    virtual bool init() override;

    // 每帧更新 (需要玩家位置)
    void update(float dt, const cocos2d::Vec2& playerPos);

    // 受击处理
    void takeDamage(int damage);

    // 设置巡逻范围
    void setPatrolRange(float leftBound, float rightBound);

    // 获取碰撞箱
    cocos2d::Rect getHitbox() const;

    // 【新增】碰到主角时的击退反应
    void onCollideWithPlayer(const cocos2d::Vec2& playerPos);

    // 析构函数
    virtual ~Zombie();

private:
    // 状态管理
    State _currentState;
    void changeState(State newState);

    // 动画相关
    void loadAnimations();
    void playWalkAnimation();
    void playAttackReadyAnimation();
    void playAttackAnimation();
    void playDeathAnimation();

    // AI逻辑
    bool isPlayerInRange(const cocos2d::Vec2& playerPos);
    bool isPlayerInChaseRange(const cocos2d::Vec2& playerPos); // ← 新增
    void updatePatrolBehavior(float dt);
    void updateAttackBehavior(float dt, const cocos2d::Vec2& playerPos);

    // 移动相关
    float _moveSpeed;           // 巡逻速度
    float _attackSpeed;         // 攻击冲锋速度
    bool _movingRight;          // 是否向右移动
    float _patrolLeftBound;     // 巡逻左边界
    float _patrolRightBound;    // 巡逻右边界

    // 检测范围
    float _detectionRange;      // 检测玩家的范围
    float _maxChaseRange;       // ← 新增：最大追逐范围
    cocos2d::Vec2 _spawnPosition;   // ← 新增：出生位置

    // 属性
    int _health;                // 生命值
    int _maxHealth;             // 最大生命值
    bool _isFacingRight;        // 面向方向

    // 动画缓存
    cocos2d::Animation* _walkAnimation;
    cocos2d::Animation* _attackReadyAnimation;
    cocos2d::Animation* _attackAnimation;
    cocos2d::Animation* _deathAnimation;
};

#endif // __ZOMBIE_H__#pragma once
