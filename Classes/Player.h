#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "cocos2d.h"

// 【关键】前向声明
// 告诉编译器 PlayerState 是一个类，但不要在这里 #include "PlayerStates.h"
// 否则会导致 Player.h 和 PlayerStates.h 互相引用而报错
class PlayerState;

class Player : public cocos2d::Sprite
{
public:
    static Player* create(const std::string& filename);
    virtual bool init() override;

    // 核心循环
    virtual void update(float dt, const std::vector<cocos2d::Rect>& platforms);

    // ==========================================
    // 1. 状态机接口 (State Machine Interface)
    // ==========================================
    // 切换状态（旧状态会被 delete）
    void changeState(PlayerState* newState);

    // 获取当前状态指针（仅供调试或特殊逻辑使用）
    PlayerState* getCurrentState() const { return _state; }

    // ==========================================
    // 2. 输入接口 (Input Interface) - 供 Scene 调用
    // ==========================================
    // 设置水平输入方向: -1(左), 0(停), 1(右)
    void setInputDirectionX(int dir);
    void setInputDirectionY(int dir);
    void setAttackDir(int dir);
    void setJumpPressed(bool pressed);
    void setAttackPressed(bool pressed);

    // ==========================================
    // 3. 状态查询接口 (Query Interface) - 供 State 类调用
    // ==========================================
    int getInputX() const { return _inputDirectionX; }
    int getInputY() const { return _inputDirectionY; }
    bool isJumpPressed() const { return _isJumpPressed; }
    bool isAttackPressed() const { return _isAttackPressed; }
    int getAttackDir() const { return _currentAttackDir; }

    // 物理查询
    bool isOnGround() const { return _isOnGround; }
    float getVelocityY() const { return _velocity.y; }
    float getVelocityX() const { return _velocity.x; }
    bool isInvincible() const { return _isInvincible; }
    // ==========================================
    // 4. 行为执行接口 (Action Interface) - 供 State 类调用
    // ==========================================
    // 执行水平移动（根据 Config 的速度）
    void moveInDirection(int dir);

    // 直接设置水平速度（用于急停、冲刺等）
    void setVelocityX(float x);

    // 跳跃逻辑
    void startJump(); // 施加向上初速度
    void stopJump();  // 截断跳跃力（松手即停）

    // 攻击逻辑
    void attack();

    // 下劈命中后的弹跳反馈 (Pogo Jump)
    void pogoJump();

    // 播放动画的通用接口 (name = "run", "idle", "jump"...)
    void playAnimation(const std::string& animName);

    // 受伤逻辑 (Scene 碰撞检测后调用)
    void takeDamage(int damage);

    // ==========================================
    // 5. 碰撞与调试 (Collision & Debug)
    // ==========================================
    cocos2d::Rect getCollisionBox() const;
    cocos2d::Rect getAttackHitbox() const;

private:
    // ==========================================
    // 私有成员变量
    // ==========================================

    // --- 状态机核心 ---
    PlayerState* _state = nullptr;

    // --- 输入缓存 ---
    int _inputDirectionX = 0;   // -1, 0, 1
    int _inputDirectionY = 0;   // -1, 0, 1
    bool _isAttackPressed = false;
    int _health; 
    int _maxHealth;
    bool _isJumpPressed = false;


    // --- 物理参数 ---
    cocos2d::Vec2 _velocity;
    cocos2d::Size _bodySize;
    cocos2d::Vec2 _bodyOffset;
    cocos2d::Rect _localBodyRect;

    bool _isOnGround = false;
    bool _isFacingRight = true; 
	bool _isInvincible = false; //无敌状态（受伤后短暂无敌）

    // 跳跃相关辅助变量
    bool _isJumpingAction = false;
    float _jumpTimer = 0.0f;
    int _currentAttackDir = 0;

    // --- 动画缓存 ---
    // 使用 Map 存储所有动画，字符串作为 Key，方便扩展
    cocos2d::Map<std::string, cocos2d::Animation*> _animations;

    // 特效精灵 (刀光)
    cocos2d::Sprite* _slashEffectSprite = nullptr;

    // 调试节点
    cocos2d::DrawNode* _debugNode = nullptr;

    // ==========================================
    // 私有辅助函数
    // ==========================================
    void initAnimations();
    void updateMovementX(float dt);
    void updateCollisionX(const std::vector<cocos2d::Rect>& platforms);
    void updateMovementY(float dt);
    void updateCollisionY(const std::vector<cocos2d::Rect>& platforms);
    void drawDebugRects();
};

#endif // __PLAYER_H__