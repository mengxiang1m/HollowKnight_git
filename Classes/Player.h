#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "cocos2d.h"
#include "PlayerStats.h"
#include "PlayerAnimator.h"

class PlayerState;

class Player : public cocos2d::Sprite
{
public:
    static Player* create(const std::string& filename = "");
    virtual bool init();

    void update(float dt, const std::vector<cocos2d::Rect>& platforms);

    // --- 状态机接口 ---
    void changeState(PlayerState* newState);
    PlayerState* getState() const { return _state; }

    // --- 动作与物理接口 ---
    void moveInDirection(int dir);
    void setVelocityX(float x);
    void setVelocityY(float y);
    void startJump();
    void stopJump();
    void pogoJump(); // 下劈弹起
    void attack();   // 播放攻击特效
    void playAnimation(const std::string& animName);
    // 凝聚特效
    void startFocusEffect();
    void stopFocusEffect();
    void playFocusEndEffect();

    // --- 委托给 Stats 组件 ---
    void takeDamage(int damage);
    void die();
    void executeHeal(); // 回血和特效
    bool canFocus() const;  // 是否满足凝聚条件

    // --- 输入相关 ---
    void setInputDirectionX(int dir);
    void setInputDirectionY(int dir);
    int getInputX() const { return _inputDirectionX; }
    int getInputY() const { return _inputDirectionY; }

    void setAttackPressed(bool pressed);
    bool isAttackPressed() const { return _isAttackPressed; }

    void setJumpPressed(bool pressed);
    bool isJumpPressed() const { return _isJumpPressed; }

    void setAttackDir(int dir);

    // 凝聚输入
    void setFocusInput(bool pressed);
    bool isFocusInputPressed() const { return _isFocusInputPressed; }

    // --- 组件访问 ---
    // 给状态机或外部使用
    PlayerStats* getStats() const { return _stats; }
    // 1. 修复 getHealth / getMaxHealth
    // 直接从 stats 组件里拿数据
    int getHealth() const { return _stats ? _stats->getHealth() : 0; }
    int getMaxHealth() const { return _stats ? _stats->getMaxHealth() : 0; }

    // 4. 修复 gainSoul (Wrapper)
    // 之前 HelloWorldScene 可能会调用 gainSoul(1)，我们转发给 Stats
    void gainSoul(int amount);
	void gainSoulOnKill();

    // 5. 修复 setOnHealthChanged / setOnSoulChanged
    // HelloWorldScene 用这个来绑定 UI
    void setOnHealthChanged(const std::function<void(int, int)>& callback);
    void setOnSoulChanged(const std::function<void(int)>& callback);

    // --- 辅助 ---
    cocos2d::Rect getCollisionBox() const;
    cocos2d::Rect getAttackHitbox() const;

    bool isOnGround() const { return _isOnGround; }
    float getVelocityY() const { return _velocity.y; }
    void drawDebugRects();
    int getAttackDir() const { return _currentAttackDir; }
    bool isInvincible() const { return _isInvincible; }

    // UI 回调接口
    std::function<void(int, int)> _onHealthChanged;
    std::function<void(int)> _onSoulChanged;

private:
    void updateMovementX(float dt);
    void updateMovementY(float dt);
    void updateCollisionX(const std::vector<cocos2d::Rect>& platforms);
    void updateCollisionY(const std::vector<cocos2d::Rect>& platforms);

private:
    PlayerStats* _stats;// 数据组件
    PlayerState* _state;// 当前状态

    // 物理相关
    cocos2d::Vec2 _velocity;
    cocos2d::Size _bodySize;
    cocos2d::Vec2 _bodyOffset;
    cocos2d::Rect _localBodyRect;

    bool _isFacingRight;
    bool _isOnGround;
    bool _isJumpingAction;
    float _jumpTimer;
    bool _isDead;

    // 输入状态
    int _inputDirectionX;
    int _inputDirectionY;
    bool _isAttackPressed;
    bool _isJumpPressed;
    int _currentAttackDir; // 0水平, 1上, -1下
    bool _isInvincible;
    bool _isFocusInputPressed;

	// 动画组件
    PlayerAnimator* _animator;

    //调试节点
    cocos2d::DrawNode* _debugNode;
};

#endif