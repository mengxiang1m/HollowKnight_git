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

    // 初始化物理属性
    _velocity = Vec2::ZERO;
    _isOnGround = false;
    _gravity = 2000.0f;         // 使用与 Player 类似的重力值
    _maxFallSpeed = -1500.0f;   // 最大下落速度

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
void Zombie::update(float dt, const cocos2d::Vec2& playerPos, const std::vector<cocos2d::Rect>& platforms)
{
    if (_currentState == State::DEAD)
    {
        return;
    }

    // 【新增】边界检查：如果 Zombie 掉出地图，记录日志
    Vec2 currentPos = this->getPosition();
    if (currentPos.y < -100.0f)
    {
        CCLOG("[Zombie] WARNING! Zombie fell off map at position (%.1f, %.1f)", currentPos.x, currentPos.y);
        // 可以选择删除或重置位置
        // this->removeFromParent();
        return;
    }

    // 【修复】无论什么状态，都要应用物理系统（包括击退）
    updateMovementY(dt);
    updateCollisionY(platforms);

    // 【修复】DAMAGED 状态下也要应用水平移动（击退效果）
    if (_currentState == State::DAMAGED)
    {
        // 只应用物理和碰撞，不执行 AI 逻辑
        updateMovementX(dt);
        updateCollisionX(platforms);
        return;
    }

    // 【新增】调试日志：每秒输出一次位置和状态
    static float debugTimer = 0;
    debugTimer += dt;
    if (debugTimer >= 1.0f)
    {
        debugTimer = 0;
        CCLOG("[Zombie] Position: (%.1f, %.1f), OnGround: %d, State: %d, Velocity: (%.1f, %.1f)", 
              currentPos.x, currentPos.y, _isOnGround, (int)_currentState, _velocity.x, _velocity.y);
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
        updateAttackBehavior(dt, playerPos);
        break;

    default:
        break;
    }

    // 应用水平移动的墙壁碰撞检测
    updateCollisionX(platforms);
}

bool Zombie::isPlayerInRange(const cocos2d::Vec2& playerPos)
{
    float distance = std::abs(playerPos.x - this->getPositionX());
    return distance <= _detectionRange;
}

void Zombie::updatePatrolBehavior(float dt)
{
    if (_movingRight)
    {
        _velocity.x = _moveSpeed;

        // 检查是否到达右边界
        if (this->getPositionX() >= _patrolRightBound)
        {
            _movingRight = false;
            _isFacingRight = false;
            this->setFlippedX(true);
            _velocity.x = 0;
        }
    }
    else
    {
        _velocity.x = -_moveSpeed;

        // 检查是否到达左边界
        if (this->getPositionX() <= _patrolLeftBound)
        {
            _movingRight = true;
            _isFacingRight = true;
            this->setFlippedX(false);
            _velocity.x = 0;
        }
    }

    // 应用水平移动
    updateMovementX(dt);
}

void Zombie::updateAttackBehavior(float dt, const cocos2d::Vec2& playerPos)
{
    float directionToPlayer = playerPos.x - this->getPositionX();

    if (_isFacingRight && directionToPlayer > 0)
    {
        _velocity.x = _attackSpeed;
    }
    else if (!_isFacingRight && directionToPlayer < 0)
    {
        _velocity.x = -_attackSpeed;
    }
    else
    {
        // 玩家在身后，恢复巡逻状态
        _velocity.x = 0;
        changeState(State::PATROL);
        return;
    }

    // 应用水平移动
    updateMovementX(dt);
}

void Zombie::takeDamage(int damage, const cocos2d::Vec2& attackerPos)
{
    // 【修改】移除 ATTACK_READY 状态的判断，允许在准备攻击时受击
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
    
    // 【新增】检查是否死亡，在改变状态前
    if (_health <= 0)
    {
        CCLOG("? Zombie defeated!");
        changeState(State::DEAD);
        return; // 【关键】立即返回，不执行击退等后续逻辑
    }
    
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

    // ========================================
    // 【修复】击退效果 - 检查墙壁，避免在墙边消失
    // ========================================
    // 停止之前可能存在的击退动作
    this->stopActionByTag(777);

    // 计算 Zombie 相对于攻击者的方向
    float directionX = currentPos.x - attackerPos.x;
    
    // 根据相对位置决定击退方向（远离攻击者）
    float knockbackDirection = (directionX > 0) ? 1.0f : -1.0f;
    
    // 【新增】检查击退方向是否有墙
    bool canKnockback = true;
    float knockbackSpeed = 300.0f;  // 【修复】提前声明变量
    Rect zombieRect = this->getBoundingBox();
    float knockbackDistance = 90.0f;  // 预计的击退距离（300 * 0.3秒）
    
    // 预测击退后的位置
    Vec2 predictedPos = Vec2(currentPos.x + knockbackDirection * knockbackDistance, currentPos.y);
    Rect predictedRect = Rect(
        predictedPos.x - zombieRect.size.width * 0.5f,
        predictedPos.y - zombieRect.size.height * 0.5f,
        zombieRect.size.width,
        zombieRect.size.height
    );
    
    // 检查预测位置是否会撞墙（需要从外部传入 platforms，暂时简化处理）
    // 如果 Zombie 靠墙（检测前方很近的距离），不应用击退
    if (knockbackDirection > 0)
    {
        // 向右击退，检查右边是否紧贴墙
        if (currentPos.x >= _patrolRightBound - 20.0f)
        {
            canKnockback = false;
            CCLOG("[Zombie] Near right wall, knockback disabled");
        }
    }
    else
    {
        // 向左击退，检查左边是否紧贴墙
        if (currentPos.x <= _patrolLeftBound + 20.0f)
        {
            canKnockback = false;
            CCLOG("[Zombie] Near left wall, knockback disabled");
        }
    }
    
    // 只有在可以击退的情况下才应用速度
    if (canKnockback)
    {
        _velocity.x = knockbackDirection * knockbackSpeed;
        CCLOG("[Zombie] Knocked back away from attacker at (%.1f, %.1f), velocity: %.1f", 
              attackerPos.x, attackerPos.y, _velocity.x);
    }
    else
    {
        // 靠墙时不击退，但保持受伤状态
        _velocity.x = 0;
        CCLOG("[Zombie] Knockback blocked by wall, staying in place");
    }

    // 0.3秒后停止击退速度并恢复之前的状态
    auto stopKnockback = CallFunc::create([this, previousState]() {
        // 停止击退速度
        if (_currentState == State::DAMAGED)
        {
            _velocity.x = 0;
            
            if (previousState == State::ATTACKING)
            {
                changeState(State::PATROL);
            }
            else
            {
                changeState(previousState);
            }
        }
    });

    auto delay = DelayTime::create(0.3f);
    auto sequence = Sequence::create(delay, stopKnockback, nullptr);
    sequence->setTag(777);  // 设置标签，方便停止
    this->runAction(sequence);

    // 【移除】后续的死亡检查（已经在前面处理了）
    // if (_health <= 0) { ... }

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

    // 【修复】降低碰撞箱高度，避免玩家跳跃时被撞到
    Rect originalBox = this->getBoundingBox();
    
    // 将碰撞箱高度降低到原来的 70%，并从底部对齐
    float reducedHeight = originalBox.size.height * 0.7f;
    float offsetY = 0.0f;  // 从底部开始
    
    return Rect(
        originalBox.origin.x,
        originalBox.origin.y + offsetY,
        originalBox.size.width,
        reducedHeight
    );
}

// ========================================
// 【修复】碰到主角时的击退反应
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

    // 【修复】改用速度冲量而非 MoveTo
    // 应用击退速度（会被碰撞检测自动限制，不会穿墙）
    float knockbackSpeed = 100.0f;  // 比受伤击退稍弱
    _velocity.x = knockbackDirection * knockbackSpeed;

    CCLOG("[Zombie] Knocked back by player collision, velocity: %.1f", _velocity.x);

    // 0.2秒后停止击退速度
    this->scheduleOnce([this](float dt) {
        if (_currentState != State::DEAD && _currentState != State::DAMAGED)
        {
            _velocity.x = 0;
        }
    }, 0.2f, "player_collision_knockback");
}

// ========================================
// 物理系统：垂直移动（重力）
// ========================================
void Zombie::updateMovementY(float dt)
{
    // 应用重力
    _velocity.y -= _gravity * dt;

    // 限制最大下落速度
    if (_velocity.y < _maxFallSpeed)
    {
        _velocity.y = _maxFallSpeed;
    }

    // 更新垂直位置
    float dy = _velocity.y * dt;
    this->setPositionY(this->getPositionY() + dy);
}

// ========================================
// 物理系统：地面碰撞检测
// ========================================
void Zombie::updateCollisionY(const std::vector<cocos2d::Rect>& platforms)
{
    _isOnGround = false;
    Rect zombieRect = this->getBoundingBox();

    for (const auto& platform : platforms)
    {
        if (zombieRect.intersectsRect(platform))
        {
            // 计算 X 轴重叠宽度
            float overlapX = std::min(zombieRect.getMaxX(), platform.getMaxX()) -
                             std::max(zombieRect.getMinX(), platform.getMinX());

            // 只有当 X 轴重叠足够大时，才进行 Y 轴修正（防止侧面贴墙问题）
            if (overlapX > zombieRect.size.width * 0.3f)
            {
                // 下落中（或静止）
                if (_velocity.y <= 0)
                {
                    float tolerance = 40.0f;
                    float overlapY = platform.getMaxY() - zombieRect.getMinY();

                    if (overlapY > -0.1f && overlapY <= tolerance)
                    {
                        // 落地修正
                        float newY = platform.getMaxY() + zombieRect.size.height * 0.5f;
                        this->setPositionY(newY);
                        _velocity.y = 0;
                        _isOnGround = true;
                    }
                }
                // 上升中（撞天花板）
                else if (_velocity.y > 0)
                {
                    float tolerance = 20.0f;
                    float overlapY = zombieRect.getMaxY() - platform.getMinY();

                    if (overlapY > 0 && overlapY <= tolerance)
                    {
                        // 顶头修正
                        float newY = platform.getMinY() - zombieRect.size.height * 0.5f;
                        this->setPositionY(newY);
                        _velocity.y = 0;
                    }
                }
            }
        }
    }
}

// ========================================
// 物理系统：水平移动
// ========================================
void Zombie::updateMovementX(float dt)
{
    float dx = _velocity.x * dt;
    this->setPositionX(this->getPositionX() + dx);
}

// ========================================
// 物理系统：墙壁碰撞检测
// ========================================
void Zombie::updateCollisionX(const std::vector<cocos2d::Rect>& platforms)
{
    Rect zombieRect = this->getBoundingBox();

    for (const auto& wall : platforms)
    {
        if (zombieRect.intersectsRect(wall))
        {
            // 计算 Y 轴重叠高度
            float overlapY = std::min(zombieRect.getMaxY(), wall.getMaxY()) -
                             std::max(zombieRect.getMinY(), wall.getMinY());

            // 只有当 Y 轴重叠足够大时，才进行 X 轴修正（防止地面边缘卡住）
            if (overlapY > zombieRect.size.height * 0.5f)
            {
                if (_velocity.x > 0)
                {
                    // 向右移动，撞到右侧墙壁
                    float newX = wall.getMinX() - zombieRect.size.width * 0.5f - 0.1f;
                    this->setPositionX(newX);
                    _velocity.x = 0;

                    // 如果在巡逻状态，转向
                    if (_currentState == State::PATROL)
                    {
                        _movingRight = false;
                        _isFacingRight = false;
                        this->setFlippedX(true);
                    }
                }
                else if (_velocity.x < 0)
                {
                    // 向左移动，撞到左侧墙壁
                    float newX = wall.getMaxX() + zombieRect.size.width * 0.5f + 0.1f;
                    this->setPositionX(newX);
                    _velocity.x = 0;

                    // 如果在巡逻状态，转向
                    if (_currentState == State::PATROL)
                    {
                        _movingRight = true;
                        _isFacingRight = true;
                        this->setFlippedX(false);
                    }
                }
            }
        }
    }
}

Zombie::~Zombie()
{
    CC_SAFE_RELEASE(_walkAnimation);
    CC_SAFE_RELEASE(_attackReadyAnimation);
    CC_SAFE_RELEASE(_attackAnimation);
    CC_SAFE_RELEASE(_deathAnimation);
}