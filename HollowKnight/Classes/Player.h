#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "cocos2d.h"
#include <functional>
#include <vector>

// 引入组件头文件
#include "PlayerStats.h"
#include "PlayerAnimator.h"

// 【关键】前向声明状态类，避免循环引用
class PlayerState;

class Player : public cocos2d::Sprite
{
public:
    virtual ~Player(); // 析构函数
    static Player* create(const std::string& filename = "");
    virtual bool init() override;

    // 【核心修改】update 增加平台数据参数，用于物理检测
    void update(float dt, const std::vector<cocos2d::Rect>& platforms);

    // ==========================================
    // 1. 状态机接口 (State Machine Interface)
    // ==========================================
    void changeState(PlayerState* newState);
    PlayerState* getState() const { return _state; }

    // ==========================================
    // 2. 动作与物理接口 (Action & Physics)
    // ==========================================
    void moveInDirection(int dir);
    void setVelocityX(float x);
    void setVelocityY(float y);

    void startJump();
    void stopJump();
    void pogoJump(); // 下劈命中后的弹起

    // 攻击 (委托给 Animator)
    void attack();

    // 动画通用接口
    void playAnimation(const std::string& animName);

    // 凝聚特效接口 (委托给 Animator)
    void startFocusEffect();
    void stopFocusEffect();
    void playFocusEndEffect();

    // ==========================================
    // 3. 战斗与数值接口 (Combat & Stats)
    // ==========================================
    // 【核心修改】受击逻辑 (带防穿墙检测)
    void takeDamage(int damage, const cocos2d::Vec2& attackerPos, const std::vector<cocos2d::Rect>& platforms);
    void executeHeal(); // 执行回血
    bool canFocus() const; // 是否满足凝聚条件


    // 魂量操作 (转发给 Stats)
    void gainSoul(int amount);
    void gainSoulOnKill();

    // 数据获取 (直接从 Stats 组件读取)
    int getHealth() const { return _stats ? _stats->getHealth() : 0; }
    int getMaxHealth() const { return _stats ? _stats->getMaxHealth() : 0; }
    float getVelocityX();
    PlayerStats* getStats() const { return _stats; }

    // ==========================================
    // 4. 输入设置 (Input Setters)
    // ==========================================
    void setInputDirectionX(int dir);
    void setInputDirectionY(int dir);
    int getInputX() const { return _inputDirectionX; }
    int getInputY() const { return _inputDirectionY; }

    void setAttackPressed(bool pressed);
    bool isAttackPressed() const { return _isAttackPressed; }

    void setJumpPressed(bool pressed);
    bool isJumpPressed() const { return _isJumpPressed; }

    void setAttackDir(int dir); // 1:上, -1:下, 0:水平
    int getAttackDir() const { return _currentAttackDir; }

    void setFocusInput(bool pressed);
    bool isFocusInputPressed() const { return _isFocusInputPressed; }

    // ==========================================
    // 5. 辅助与调试 (Helpers & Debug)
    // ==========================================
    cocos2d::Rect getCollisionBox() const;
    cocos2d::Rect getAttackHitbox() const;

    bool isOnGround() const { return _isOnGround; }
    float getVelocityY() const { return _velocity.y; }
    bool isInvincible() const { return _isInvincible; }

    void drawDebugRects();

    // ==========================================
    // 6. UI 回调绑定
    // ==========================================
    void setOnHealthChanged(const std::function<void(int, int)>& callback);
    void setOnSoulChanged(const std::function<void(int)>& callback);

public:
    bool isFacingRight() const { return _isFacingRight; }

private:
    // --- 内部物理逻辑 ---
    void updateMovementX(float dt);
    void updateMovementY(float dt);
    void updateCollisionX(const std::vector<cocos2d::Rect>& platforms);
    void updateCollisionY(const std::vector<cocos2d::Rect>& platforms);

private:
    // ==========================================
    // 成员变量
    // ==========================================

    // --- 组件 ---
    PlayerStats* _stats = nullptr;       // 数值管理
    PlayerAnimator* _animator = nullptr; // 动画管理
    PlayerState* _state = nullptr;       // 当前状态

    // --- 物理参数 ---
    cocos2d::Vec2 _velocity;
    cocos2d::Size _bodySize;
    cocos2d::Vec2 _bodyOffset;
    cocos2d::Rect _localBodyRect;
    cocos2d::Vec2 _lastSafePosition;// 记录上一次安全落地的位置

    bool _isOnGround;
    bool _isFacingRight;

    // --- 逻辑标记 ---
    bool _isInvincible;
    bool _isJumpingAction;
    float _jumpTimer;

    // --- 输入缓存 ---
    int _inputDirectionX;
    int _inputDirectionY;
    bool _isAttackPressed;
    bool _isJumpPressed;
    bool _isFocusInputPressed;
    int _currentAttackDir;

    // --- 调试 ---
    cocos2d::DrawNode* _debugNode = nullptr;

    // --- 回调函数存储 ---
    std::function<void(int, int)> _onHealthChanged;
    std::function<void(int)> _onSoulChanged;
};

#endif // __PLAYER_H__