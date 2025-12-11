#include "Zombie.h"

USING_NS_CC;

Zombie* Zombie::create(const std::string& filename)
{
    Zombie* zombie = new (std::nothrow) Zombie();
    if (zombie && zombie->initWithFile(filename) && zombie->init())
    {
        zombie->autorelease();
        CCLOG("? [Zombie::create] Succeeded with file: %s", filename.c_str());
        return zombie;
    }
    CCLOG("? [Zombie::create] FAILED with file: %s", filename.c_str());
    CC_SAFE_DELETE(zombie);
    return nullptr;
}

bool Zombie::init()
{
    if (!Sprite::init())
    {
        return false;
    }

    // 初始化属性
    _currentState = State::PATROL;
    _health = 5;
    _maxHealth = 5;
    _moveSpeed = 40.0f;
    _attackSpeed = 200.0f;
    _movingRight = true;
    _isFacingRight = true;

    _patrolLeftBound = 0.0f;
    _patrolRightBound = 300.0f;

    // 检测范围 (约半个屏幕)
    _detectionRange = 400.0f;

    // 【修改】追逐范围限制 (1个屏幕宽度)
    auto visibleSize = Director::getInstance()->getVisibleSize();
    _maxChaseRange = visibleSize.width * 1.0f;

    // 【新增】记录初始出生位置
    _spawnPosition = Vec2::ZERO; // 稍后在场景中设置位置时会更新

    loadAnimations();
    playWalkAnimation();

    CCLOG("? [Zombie::init] Zombie initialized successfully!");

    return true;
}

void Zombie::loadAnimations()
{
    // ========================================
    // 加载走路动画 (7帧)
    // ========================================
    Vector<SpriteFrame*> walkFrames;

    for (int i = 1; i <= 7; i++)
    {
        std::string frameName = StringUtils::format("zombie/walk/walk_%d.png", i);

        auto testSprite = Sprite::create(frameName);
        if (testSprite)
        {
            auto frame = testSprite->getSpriteFrame();
            walkFrames.pushBack(frame);
            CCLOG("  ? Loaded walk frame: %s", frameName.c_str());
        }
        else
        {
            CCLOG("  ? Failed to load walk frame: %s", frameName.c_str());
        }
    }

    if (walkFrames.size() > 0)
    {
        _walkAnimation = Animation::createWithSpriteFrames(walkFrames, 0.15f);
        _walkAnimation->retain();
        CCLOG("? Walk animation created with %d frames", (int)walkFrames.size());
    }
    else
    {
        _walkAnimation = nullptr;
    }

    // ========================================
    // 加载准备攻击动画 (5帧)
    // ========================================
    Vector<SpriteFrame*> attackReadyFrames;

    for (int i = 1; i <= 5; i++)
    {
        std::string frameName = StringUtils::format("zombie/attack/attackReady_%d.png", i);

        auto testSprite = Sprite::create(frameName);
        if (testSprite)
        {
            auto frame = testSprite->getSpriteFrame();
            attackReadyFrames.pushBack(frame);
            CCLOG("  ? Loaded attackReady frame: %s", frameName.c_str());
        }
        else
        {
            CCLOG("  ? Failed to load attackReady frame: %s", frameName.c_str());
        }
    }

    if (attackReadyFrames.size() > 0)
    {
        _attackReadyAnimation = Animation::createWithSpriteFrames(attackReadyFrames, 0.1f);
        _attackReadyAnimation->retain();
        CCLOG("? AttackReady animation created with %d frames", (int)attackReadyFrames.size());
    }
    else
    {
        _attackReadyAnimation = nullptr;
    }

    // ========================================
    // 加载攻击动画 (3帧)
    // ========================================
    Vector<SpriteFrame*> attackFrames;

    for (int i = 1; i <= 3; i++)
    {
        std::string frameName = StringUtils::format("zombie/attack/attack_%d.png", i);

        auto testSprite = Sprite::create(frameName);
        if (testSprite)
        {
            auto frame = testSprite->getSpriteFrame();
            attackFrames.pushBack(frame);
            CCLOG("  ? Loaded attack frame: %s", frameName.c_str());
        }
        else
        {
            CCLOG("  ? Failed to load attack frame: %s", frameName.c_str());
        }
    }

    if (attackFrames.size() > 0)
    {
        _attackAnimation = Animation::createWithSpriteFrames(attackFrames, 0.1f);
        _attackAnimation->retain();
        CCLOG("? Attack animation created with %d frames", (int)attackFrames.size());
    }
    else
    {
        _attackAnimation = nullptr;
    }

    _deathAnimation = nullptr;
}

void Zombie::playWalkAnimation()
{
    if (_walkAnimation)
    {
        // 【修复】只停止动画动作，不停止颜色恢复动作
        this->stopActionByTag(100); // 只停止动画相关的动作

        auto animate = Animate::create(_walkAnimation);
        auto repeat = RepeatForever::create(animate);
        repeat->setTag(100); // 设置动画标签
        this->runAction(repeat);
        CCLOG("?? Playing walk animation");
    }
}

void Zombie::playAttackReadyAnimation()
{
    if (_attackReadyAnimation)
    {
        // 【修复】只停止动画動作，不停止顏色恢復動作
        this->stopActionByTag(100);

        auto animate = Animate::create(_attackReadyAnimation);
        animate->setTag(100);
        this->runAction(animate);

        float animDuration = _attackReadyAnimation->getDuration();
        this->scheduleOnce([this](float dt) {
            changeState(State::ATTACKING);
            }, animDuration, "attack_ready_end");

        CCLOG("? Playing attack ready animation");
    }
}

void Zombie::playAttackAnimation()
{
    if (_attackAnimation)
    {
        // 【修復】只停止動畫動作，不停止顏色恢復動作
        this->stopActionByTag(100);

        auto animate = Animate::create(_attackAnimation);
        auto repeat = RepeatForever::create(animate);
        repeat->setTag(100);
        this->runAction(repeat);
        CCLOG("?? Playing attack animation");
    }
}

void Zombie::playDeathAnimation()
{
    // 【修复】停止所有动作（死亡时可以停止所有）
    this->stopAllActions();

    auto fadeOut = FadeOut::create(0.5f);
    auto rotateBy = RotateBy::create(0.5f, 180);
    auto spawn = Spawn::create(fadeOut, rotateBy, nullptr);

    auto removeSelf = CallFunc::create([this]() {
        this->removeFromParent();
        CCLOG("?? Zombie removed from scene");
        });

    auto sequence = Sequence::create(spawn, removeSelf, nullptr);
    this->runAction(sequence);
}

// ========================================
// AI 核心更新函数
// ========================================
void Zombie::update(float dt, const cocos2d::Vec2& playerPos)
{
    if (_currentState == State::DEAD || _currentState == State::DAMAGED)
    {
        return;
    }

    // 【新增】记录第一次设置位置时的出生点
    if (_spawnPosition == Vec2::ZERO)
    {
        _spawnPosition = this->getPosition();
        CCLOG("[Zombie] Spawn position set to (%.0f, %.0f)", _spawnPosition.x, _spawnPosition.y);
    }

    // 根据状态执行不同的行为
    switch (_currentState)
    {
    case State::PATROL:
        // 检测玩家
        if (isPlayerInRange(playerPos))
        {
            // 转向玩家
            float directionToPlayer = playerPos.x - this->getPositionX();
            _isFacingRight = (directionToPlayer > 0);
            this->setFlippedX(!_isFacingRight);

            changeState(State::ATTACK_READY);
        }
        else
        {
            updatePatrolBehavior(dt);
        }
        break;

    case State::ATTACK_READY:
        // 在原地播放准备动画
        break;

    case State::ATTACKING:
        // 【修改】检查是否超出追逐范围
        if (!isPlayerInChaseRange(playerPos))
        {
            CCLOG("[Zombie] Player out of chase range, returning to patrol");
            changeState(State::PATROL);
        }
        else
        {
            updateAttackBehavior(dt, playerPos);
        }
        break;

    default:
        break;
    }
}

bool Zombie::isPlayerInRange(const cocos2d::Vec2& playerPos)
{
    float distance = std::abs(playerPos.x - this->getPositionX());
    return distance <= _detectionRange;
}

// 【新增】检查玩家是否在追逐范围内
bool Zombie::isPlayerInChaseRange(const cocos2d::Vec2& playerPos)
{
    float distanceFromSpawn = std::abs(playerPos.x - _spawnPosition.x);
    return distanceFromSpawn <= _maxChaseRange;
}

void Zombie::updatePatrolBehavior(float dt)
{
    Vec2 currentPos = this->getPosition();

    if (_movingRight)
    {
        currentPos.x += _moveSpeed * dt;

        if (currentPos.x >= _patrolRightBound)
        {
            _movingRight = false;
            _isFacingRight = false;
            this->setFlippedX(true);
        }
    }
    else
    {
        currentPos.x -= _moveSpeed * dt;

        if (currentPos.x <= _patrolLeftBound)
        {
            _movingRight = true;
            _isFacingRight = true;
            this->setFlippedX(false);
        }
    }

    this->setPosition(currentPos);
}

void Zombie::updateAttackBehavior(float dt, const cocos2d::Vec2& playerPos)
{
    Vec2 currentPos = this->getPosition();
    float directionToPlayer = playerPos.x - currentPos.x;

    if (_isFacingRight && directionToPlayer > 0)
    {
        currentPos.x += _attackSpeed * dt;
    }
    else if (!_isFacingRight && directionToPlayer < 0)
    {
        currentPos.x -= _attackSpeed * dt;
    }
    else
    {
        // 玩家在身后，恢复巡逻状态
        changeState(State::PATROL);
    }

    this->setPosition(currentPos);
}

void Zombie::takeDamage(int damage)
{
    // 1. 如果已经死亡 或 处于无敌状态，直接返回
    if (_currentState == State::DEAD || _currentState == State::DAMAGED || _isInvincible)
    {
        return;
    }

    // 2. 开启无敌
    _isInvincible = true;
    _health -= damage;
    CCLOG("?? Zombie took %d damage! Health: %d/%d", damage, _health, _maxHealth);

    Vec2 currentPos = this->getPosition();

    State previousState = _currentState;
    changeState(State::DAMAGED);

    // ========================================
    // 【修复】受击闪烁效果 - 确保恢复原始颜色和透明度
    // ========================================
    // 1. 停止之前的所有颜色相关动作
    this->stopActionByTag(888);

    // 2. 创建闪烁序列（红色 -> 白色）重复2次
    auto tintRed = TintTo::create(0.1f, 255, 0, 0);
    auto tintNormal = TintTo::create(0.1f, 255, 255, 255);
    auto blink = Sequence::create(tintRed, tintNormal, nullptr);
    auto repeat = Repeat::create(blink, 2);

    // 3. 在闪烁结束后强制恢复原始颜色
    auto resetColor = CallFunc::create([this]() {
        this->setColor(Color3B::WHITE);
        this->setOpacity(255);
        CCLOG("? Zombie color restored to original");
        });

    auto blinkSequence = Sequence::create(repeat, resetColor, nullptr);
    blinkSequence->setTag(888);
    this->runAction(blinkSequence);

    // 击退效果
    float knockbackDistance = 50.0f;
    float knockbackDuration = 0.3f;
    float direction = _isFacingRight ? -1.0f : 1.0f;

    Vec2 knockbackTarget = Vec2(currentPos.x + direction * knockbackDistance, currentPos.y);

    auto knockback = MoveTo::create(knockbackDuration, knockbackTarget);
    auto easeOut = EaseOut::create(knockback, 2.0f);
    this->runAction(easeOut);

    // 0.3秒后恢复之前的状态
    this->scheduleOnce([this, previousState](float dt) {
        if (_currentState == State::DAMAGED)
        {
            if (previousState == State::ATTACKING)
            {
                changeState(State::PATROL);
            }
            else
            {
                changeState(previousState);
            }
        }
        }, 0.3f, "damage_recovery");

    // 检查是否死亡
    if (_health <= 0)
    {
        CCLOG("? Zombie defeated!");
        changeState(State::DEAD);
    }

    // 3. 设置定时器关闭无敌 (0.2秒后恢复)
    this->scheduleOnce([this](float dt) {
        _isInvincible = false;
        }, 0.2f, "invincible_cooldown");
}

void Zombie::changeState(State newState)
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

    case State::ATTACK_READY:
        playAttackReadyAnimation();
        break;

    case State::ATTACKING:
        playAttackAnimation();
        break;

    case State::DEAD:
        playDeathAnimation();
        break;

    case State::DAMAGED:
        // 不切换动画，保持当前动画
        break;
    }
}

void Zombie::setPatrolRange(float leftBound, float rightBound)
{
    _patrolLeftBound = leftBound;
    _patrolRightBound = rightBound;
    CCLOG("[Zombie] Patrol range set: %.0f to %.0f", leftBound, rightBound);
}

cocos2d::Rect Zombie::getHitbox() const
{
    // 【新增】死亡状态不返回碰撞箱
    if (_currentState == State::DEAD)
    {
        return Rect::ZERO;
    }

    return this->getBoundingBox();
}

// ========================================
// 【新增】碰到主角时的击退反应
// ========================================
void Zombie::onCollideWithPlayer(const cocos2d::Vec2& playerPos)
{
    if (_currentState == State::DEAD || _currentState == State::DAMAGED)
    {
        return;
    }

    // 计算僵尸相对玩家的方向
    Vec2 zombiePos = this->getPosition();
    float directionX = zombiePos.x - playerPos.x;

    // 根据相对位置决定击退方向
    float knockbackDirection = (directionX > 0) ? 1.0f : -1.0f;

    // 击退参数（僵尸更重，击退距离稍小）
    float knockbackDistance = 30.0f;  // 比普通敌人稍小
    float knockbackDuration = 0.2f;   // 稍慢一点

    // 计算目标位置
    Vec2 knockbackTarget = Vec2(zombiePos.x + knockbackDirection * knockbackDistance, zombiePos.y);

    // 执行击退动作
    auto knockback = MoveTo::create(knockbackDuration, knockbackTarget);
    auto easeOut = EaseOut::create(knockback, 2.0f);
    this->runAction(easeOut);

    CCLOG("[Zombie] Knocked back by player collision");
}

Zombie::~Zombie()
{
    CC_SAFE_RELEASE(_walkAnimation);
    CC_SAFE_RELEASE(_attackReadyAnimation);
    CC_SAFE_RELEASE(_attackAnimation);
    CC_SAFE_RELEASE(_deathAnimation);
}