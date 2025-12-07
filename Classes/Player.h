#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "cocos2d.h"

class Player : public cocos2d::Sprite
{
public:
    // 状态枚举
    enum class State {
        IDLE,       // 待机
        RUNNING, //跑步
        DASHING,    // 冲刺
        JUMPING,    // 跳跃 (上升中)
        FALLING,    // 下落 (下降中)
        ATTACKING   // 攻击
    };

    static Player* create(const std::string& filename);
    virtual bool init() override;

    // 核心循环：参数包含地图碰撞框
    virtual void update(float dt, const std::vector<cocos2d::Rect>& platforms);

    // 状态切换入口
    void changeState(State newState);

    // 动作控制接口 (供 HelloWorldScene 调用)
    void moveLeft();
    void moveRight();
    void stopMove();
    void attack();
    void jump(); // 预留跳跃接口

    // 获取用于逻辑判定的碰撞箱 (世界坐标)
    cocos2d::Rect getCollisionBox() const;

    // 获取攻击判定框
    cocos2d::Rect getAttackHitbox() const;

private:
    void initAnimations();

    //碰撞框参数
    cocos2d::Size _bodySize;      // 物理框的宽和高
    cocos2d::Vec2 _bodyOffset;    // 物理框相对于图片原点(0,0)的偏移

    // ==========================================
    // 【核心架构】逻辑分层
    // ==========================================
   // 将物理更新拆解为 X 和 Y 两个独立步骤
    void updateMovementX(float dt);
    void updateCollisionX(const std::vector<cocos2d::Rect>& platforms);

    void updateMovementY(float dt);
    void updateCollisionY(const std::vector<cocos2d::Rect>& platforms);

    void updateStateMachine();          // 3. 根据物理结果决定当前状态

    State _currentState;

    // 运动参数
    float _moveSpeed;
    cocos2d::Vec2 _velocity; // x: 水平速度, y: 垂直速度
    bool _isFacingRight;
    bool _isAttacking;

    // 物理参数
    float _gravity;        // 重力
    float _jumpForce;      // 跳跃力
    bool _isOnGround;      // 是否在地面

    cocos2d::DrawNode* _debugNode;

    // 动画缓存
    cocos2d::Animation* _idleAnim;
    cocos2d::Animation* _dashAnim; 
    cocos2d::Animation* _runAnim; 
    cocos2d::Animation* _jumpAnim; 
    cocos2d::Animation* _fallAnim; 
    cocos2d::Animation* _attackAnim; // 暂时还没用到，留着

    // 获取固定的物理身体框 (世界坐标)
    cocos2d::Rect _localBodyRect;
    cocos2d::Rect getBodyRect() const;

    // 专门用于渲染调试框的函数
    void drawDebugRects();
};

#endif // __PLAYER_H__