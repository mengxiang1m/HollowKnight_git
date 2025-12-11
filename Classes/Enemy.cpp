#include "Enemy.h"

USING_NS_CC;

Enemy* Enemy::create(const std::string& filename)
{
    Enemy* enemy = new (std::nothrow) Enemy();
    if (enemy && enemy->initWithFile(filename) && enemy->init())
    {
        enemy->autorelease();
        CCLOG(" [Enemy::create] Succeeded with file: %s", filename.c_str());
        return enemy;
    }
    CCLOG("[Enemy::create] FAILED with file: %s", filename.c_str());
    CC_SAFE_DELETE(enemy);
    return nullptr;
}

bool Enemy::init()
{
    if (!Sprite::init())
    {
        return false;
    }

    // 初始化属性
    _currentState = State::PATROL;
    _health = 3;
    _maxHealth = 3;
    _moveSpeed = 50.0f;
    _movingRight = true;
    
    _patrolLeftBound = 0.0f;
    _patrolRightBound = 300.0f;

    // 加载动画
    loadAnimations();

    // 播放走路动画
    playWalkAnimation();

    // 启用每帧更新
    this->scheduleUpdate();

    CCLOG(" [Enemy::init] Enemy initialized successfully!");

    return true;
}

void Enemy::loadAnimations()
{
    Vector<SpriteFrame*> walkFrames;
    
    for (int i = 1; i <= 4; i++)
    {
        std::string frameName = StringUtils::format("enemies/enemy_walk_%d.png", i);
        
        auto testSprite = Sprite::create(frameName);
        if (testSprite)
        {
            auto frame = testSprite->getSpriteFrame();
            walkFrames.pushBack(frame);
            CCLOG("   Loaded frame: %s", frameName.c_str());
        }
        else
        {
            CCLOG("   Failed to load frame: %s", frameName.c_str());
        }
    }

    if (walkFrames.size() > 0)
    {
        _walkAnimation = Animation::createWithSpriteFrames(walkFrames, 0.15f);
        _walkAnimation->retain();
        CCLOG(" Walk animation created with %d frames", (int)walkFrames.size());
    }
    else
    {
        CCLOG(" No frames loaded, animation will not play!");
        _walkAnimation = nullptr;
    }

    _deathAnimation = nullptr;
}

void Enemy::playWalkAnimation()
{
    if (_walkAnimation)
    {
        auto animate = Animate::create(_walkAnimation);
        auto repeat = RepeatForever::create(animate);
        this->runAction(repeat);
        CCLOG(" Playing walk animation");
    }
    else
    {
        CCLOG(" Cannot play animation - _walkAnimation is null");
    }
}

void Enemy::playDeathAnimation()
{
    this->stopAllActions();

    auto fadeOut = FadeOut::create(0.5f);
    auto rotateBy = RotateBy::create(0.5f, 180);
    auto spawn = Spawn::create(fadeOut, rotateBy, nullptr);
    
    auto removeSelf = CallFunc::create([this]() {
        this->removeFromParent();
        CCLOG("Enemy removed from scene");
    });

    auto sequence = Sequence::create(spawn, removeSelf, nullptr);
    this->runAction(sequence);
}

void Enemy::update(float dt)
{
    if (_currentState == State::DEAD)
    {
        return;
    }

    // 巡逻逻辑
    Vec2 currentPos = this->getPosition();

    if (_movingRight)
    {
        currentPos.x += _moveSpeed * dt;

        if (currentPos.x >= _patrolRightBound)
        {
            _movingRight = false;
            this->setFlippedX(true);
            CCLOG("[Enemy] Reached right bound, turning left");
        }
    }
    else
    {
        currentPos.x -= _moveSpeed * dt;

        if (currentPos.x <= _patrolLeftBound)
        {
            _movingRight = true;
            this->setFlippedX(false);
            CCLOG("[Enemy] Reached left bound, turning right");
        }
    }

    this->setPosition(currentPos);
}

// ======================================================================
// 受击处理 - 包含击退效果
// ======================================================================
void Enemy::takeDamage(int damage)
{
    // 【修复】如果已经死亡 或 处于无敌状态，直接返回
    if (_currentState == State::DEAD || _isInvincible) return;

    _health -= damage;
    CCLOG(" Enemy took % d damage!Health: % d / % d", damage, _health, _maxHealth);
    // 开启无敌
    _isInvincible = true;
    // ========================================
    // 1. 受击闪烁效果（变红 + 闪烁）
    // ========================================
    auto tintRed = TintTo::create(0.1f, 255, 0, 0);      // 变红
    auto tintNormal = TintTo::create(0.1f, 255, 255, 255); // 恢复正常
    auto blink = Sequence::create(tintRed, tintNormal, nullptr);
    auto repeat = Repeat::create(blink, 2); // 闪烁2次
    this->runAction(repeat);

    // ========================================
    // 2. 击退效果（向后推）
    // ========================================
    float knockbackDistance = 30.0f; // 击退距离
    float knockbackDuration = 0.2f;  // 击退持续时间

    // 根据敌人当前朝向决定击退方向
    float direction = _movingRight ? -1.0f : 1.0f; // 向左击退或向右击退
    
    Vec2 currentPos = this->getPosition();
    Vec2 knockbackTarget = Vec2(currentPos.x + direction * knockbackDistance, currentPos.y);
    
    auto knockback = MoveTo::create(knockbackDuration, knockbackTarget);
    auto easeOut = EaseOut::create(knockback, 2.0f); // 缓动效果
    this->runAction(easeOut);

    // ========================================
    // 3. 检查是否死亡
    // ========================================
    if (_health <= 0)
    {
        CCLOG(" Enemy defeated!");
        changeState(State::DEAD);
    }

	// 【修复】设置无敌持续时间
    this->scheduleOnce([this](float dt) {
        _isInvincible = false;
        }, 0.2f, "invincible_cooldown");
}

void Enemy::changeState(State newState)
{
    if (_currentState == newState)
    {
        return;
    }

    _currentState = newState;

    switch (_currentState)
    {
    case State::PATROL:
        playWalkAnimation();
        break;

    case State::DEAD:
        playDeathAnimation();
        break;
    }
}

void Enemy::setPatrolRange(float leftBound, float rightBound)
{
    _patrolLeftBound = leftBound;
    _patrolRightBound = rightBound;
    CCLOG("[Enemy] Patrol range set: %.0f to %.0f", leftBound, rightBound);
}

// 【新增】获取敌人的碰撞箱
cocos2d::Rect Enemy::getHitbox() const
{
    if (_currentState == State::DEAD)
    {
        return Rect::ZERO;
    }

    // 获取敌人精灵在世界坐标系中的包围盒
    return this->getBoundingBox();
}

// ========================================
// 碰到主角时的击退反应
// ========================================
void Enemy::onCollideWithPlayer(const cocos2d::Vec2& playerPos)
{
    if (_currentState == State::DEAD)
    {
        return;
    }

    // 计算敌人相对玩家的方向
    Vec2 enemyPos = this->getPosition();
    float directionX = enemyPos.x - playerPos.x;

    // 根据相对位置决定击退方向
    float knockbackDirection = (directionX > 0) ? 1.0f : -1.0f;

    // 击退参数
    float knockbackDistance = 40.0f;  // 小幅度击退
    float knockbackDuration = 0.15f;  // 快速击退

    // 计算目标位置
    Vec2 knockbackTarget = Vec2(enemyPos.x + knockbackDirection * knockbackDistance, enemyPos.y);

    // 执行击退动作
    auto knockback = MoveTo::create(knockbackDuration, knockbackTarget);
    auto easeOut = EaseOut::create(knockback, 2.0f);
    this->runAction(easeOut);

    CCLOG("[Enemy] Knocked back by player collision");
}

Enemy::~Enemy()
{
    CC_SAFE_RELEASE(_walkAnimation);
    CC_SAFE_RELEASE(_deathAnimation);
}