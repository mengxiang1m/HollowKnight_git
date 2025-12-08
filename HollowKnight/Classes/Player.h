#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "cocos2d.h"
#include <vector>

class Player : public cocos2d::Sprite
{
public:
    enum class State {
        IDLE,
        RUNNING,
        JUMPING,
        FALLING,
        ATTACKING,
        DAMAGED     // ← 添加受伤状态
    };

    static Player* create(const std::string& filename);
    virtual bool init() override;

    // 物理更新
    void update(float dt, const std::vector<cocos2d::Rect>& platforms);

    // 动作接口
    void moveLeft();
    void moveRight();
    void stopMove();
    void jump();
    void attack();

    // 【新增】受伤接口
    void takeDamage(int damage);

    // 获取碰撞箱
    cocos2d::Rect getCollisionBox() const;

    // 攻击判定框
    cocos2d::Rect getAttackHitbox() const;

    // 【新增】检查是否处于无敌状态
    bool isInvincible() const { return _isInvincible; }

private:
    // 物理核心
    void updateMovementX(float dt);
    void updateCollisionX(const std::vector<cocos2d::Rect>& platforms);
    void updateMovementY(float dt);
    void updateCollisionY(const std::vector<cocos2d::Rect>& platforms);

    // 状态机
    void updateStateMachine();
    void changeState(State newState, bool force = false);

    // 资源加载
    void initAnimations();
    void drawDebugRects();

    // 状态
    State _currentState;
    bool _isFacingRight;
    bool _isAttacking;
    bool _isOnGround;

    // 【新增】受伤相关
    bool _isInvincible;      // 无敌状态
    int _health;             // 生命值
    int _maxHealth;          // 最大生命值

    // 运动参数
    cocos2d::Vec2 _velocity;
    float _moveSpeed;
    float _gravity;
    float _jumpForce;

    // 碰撞箱
    cocos2d::Rect _localBodyRect;
    cocos2d::Size _bodySize;
    cocos2d::Vec2 _bodyOffset;

    // 动画
    cocos2d::Animation* _idleAnim;
    cocos2d::Animation* _runAnim;
    cocos2d::Animation* _jumpAnim;
    cocos2d::Animation* _fallAnim;
    cocos2d::Animation* _dashAnim;
    cocos2d::Animation* _damageAnim;  // ← 新增：受伤动画

    // 调试
    cocos2d::DrawNode* _debugNode;
};

#endif // __PLAYER_H__