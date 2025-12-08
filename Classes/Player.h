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
        DAMAGED, //受攻击
        SLASHING   // 攻击
    };

    static Player* create(const std::string& filename);
    virtual bool init() override;

    // 核心循环：参数包含地图碰撞框
    virtual void update(float dt, const std::vector<cocos2d::Rect>& platforms);

    // 状态切换入口
    void changeState(State newState, bool force = false);
    // 动作控制接口 (供 HelloWorldScene 调用)
    void moveLeft();
    void moveRight();
    void stopMove();
    void attack();
    void jump(); 
    void Player::takeDamage(int damage);

    // 获取用于逻辑判定的碰撞箱 (世界坐标)
    cocos2d::Rect getCollisionBox() const;

    // 获取攻击判定框
    cocos2d::Rect getAttackHitbox() const;

    // 检查是否处于无敌状态
    bool isInvincible() const { return _isInvincible; }

    void startJump(); // 按下跳跃键调用
    void stopJump();  // 松开跳跃键调用

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
	bool _isInvincible; // 是否处于无敌状态
    bool _isJumpingAction;      // 是否正在执行跳跃动作（按住按键中）
    float _jumpTimer;           // 已经按住跳跃键多久了
    float _maxJumpTime;         // 允许按住加力的最大时长（比如0.35秒）

    // 物理参数
        // 生命值初始化
    int _health;
    int _maxHealth;
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
    cocos2d::Animation*  _damageAnim; 
    cocos2d::Animation* _slashAnim; 
    cocos2d::Animation* _slashEffectAnim;// 刀光动画数据

    cocos2d::Sprite* _slashEffectSprite; // 专门显示刀光的精灵
    // 获取固定的物理身体框 (世界坐标)
    cocos2d::Rect _localBodyRect;

    // 专门用于渲染调试框的函数
    void drawDebugRects();
};

#endif // __PLAYER_H__