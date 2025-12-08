#include "Player.h"

USING_NS_CC;

// =================================================================
//  1. 创建与初始化 (Lifecycle)
// =================================================================

Player* Player::create(const std::string& filename)
{
    Player* player = new (std::nothrow) Player();
    if (player && player->init())
    {
        player->autorelease();
        return player;
    }
    CC_SAFE_DELETE(player);
    return nullptr;
}

bool Player::init()
{
    if (!this->initWithFile("Knight/idle/idle_1.png"))
    {
        CCLOG("Error: Failed to load 'Knight/idle_1.png' in Player::init");
        return false;
    }

    this->setAnchorPoint(Vec2(0.5f, 0.0f));

    Size size = this->getContentSize();
    CCLOG("Player init: Texture size = (%f, %f)", size.width, size.height);

    // 物理碰撞箱设置
    float bodyW = size.width * 0.8f;
    float bottomGap = 30.0f;
    float bodyH = size.height - bottomGap;
    float startX = -bodyW * 0.5f;
    float startY = bottomGap;

    _localBodyRect = Rect(startX, startY, bodyW, bodyH);
    _bodySize = Size(bodyW, bodyH);
    _bodyOffset = Vec2(0, bottomGap);

    // 运动参数
    _moveSpeed = 300.0f;
    _velocity = Vec2::ZERO;
    _gravity = 2000.0f;
    _jumpForce = 700.0f;

    _isFacingRight = true;
    _isAttacking = false;
    _isOnGround = false;
    _currentState = State::IDLE;

    // 生命值初始化
    _health = 5;
    _maxHealth = 5;
    _isInvincible = false;

    initAnimations();

    _debugNode = DrawNode::create();
    this->addChild(_debugNode, 999);

    changeState(State::IDLE);

    return true;
}

// =================================================================
//  2. 物理核心循环 (Physics Loop)
// =================================================================

void Player::update(float dt, const std::vector<cocos2d::Rect>& platforms)
{
    // 【新增】受伤状态不处理移动和碰撞
    if (_currentState == State::DAMAGED)
    {
        drawDebugRects();
        return;
    }

    updateMovementX(dt);
    updateCollisionX(platforms);

    updateMovementY(dt);
    updateCollisionY(platforms);

    updateStateMachine();

    drawDebugRects();
}

void Player::updateMovementX(float dt)
{
    float dx = _velocity.x * dt;
    this->setPositionX(this->getPositionX() + dx);
}

void Player::updateCollisionX(const std::vector<cocos2d::Rect>& platforms)
{
    Rect playerRect = getCollisionBox();

    for (const auto& wall : platforms)
    {
        if (playerRect.intersectsRect(wall))
        {
            float overlapY = std::min(playerRect.getMaxY(), wall.getMaxY()) -
                std::max(playerRect.getMinY(), wall.getMinY());

            if (overlapY > playerRect.size.height * 0.5f)
            {
                if (_velocity.x > 0)
                {
                    float newX = wall.getMinX() - _bodySize.width * 0.5f - _bodyOffset.x - 0.1f;
                    this->setPositionX(newX);
                }
                else if (_velocity.x < 0)
                {
                    float newX = wall.getMaxX() + _bodySize.width * 0.5f - _bodyOffset.x + 0.1f;
                    this->setPositionX(newX);
                }
            }
        }
    }
}

void Player::updateMovementY(float dt)
{
    _velocity.y -= _gravity * dt;
    if (_velocity.y < -1500.0f) _velocity.y = -1500.0f;

    float dy = _velocity.y * dt;
    this->setPositionY(this->getPositionY() + dy);
}

void Player::updateCollisionY(const std::vector<cocos2d::Rect>& platforms)
{
    _isOnGround = false;
    Rect playerRect = getCollisionBox();

    for (const auto& platform : platforms)
    {
        if (playerRect.intersectsRect(platform))
        {
            float overlapX = std::min(playerRect.getMaxX(), platform.getMaxX()) -
                std::max(playerRect.getMinX(), platform.getMinX());

            if (overlapX > playerRect.size.width * 0.8f)
            {
                if (_velocity.y <= 0)
                {
                    float tolerance = 40.0f;
                    float overlapY = platform.getMaxY() - playerRect.getMinY();

                    if (overlapY > 0 && overlapY <= tolerance)
                    {
                        float newY = platform.getMaxY() - _bodyOffset.y;
                        this->setPositionY(newY);
                        _velocity.y = 0;
                        _isOnGround = true;
                    }
                }
                else if (_velocity.y > 0)
                {
                    float tolerance = 20.0f;
                    float overlapY = playerRect.getMaxY() - platform.getMinY();

                    if (overlapY > 0 && overlapY <= tolerance)
                    {
                        float newY = platform.getMinY() - _bodySize.height - _bodyOffset.y;
                        this->setPositionY(newY);
                        _velocity.y = 0;
                    }
                }
            }
        }
    }
}

// =================================================================
//  3. 辅助功能 (Helpers)
// =================================================================

cocos2d::Rect Player::getCollisionBox() const
{
    Vec2 worldPos = this->getPosition();
    return Rect(
        worldPos.x + _localBodyRect.origin.x,
        worldPos.y + _localBodyRect.origin.y,
        _localBodyRect.size.width,
        _localBodyRect.size.height
    );
}

void Player::drawDebugRects()
{
    if (!_debugNode) return;
    _debugNode->clear();

    Size size = this->getContentSize();
    Vec2 centerOffset = Vec2(size.width * 0.5f, 0.0f);
    
    Vec2 greenMin = _localBodyRect.origin + centerOffset;
    Vec2 greenMax = greenMin + _localBodyRect.size;
    _debugNode->drawSolidRect(greenMin, greenMax, Color4F(0, 1, 0, 0.4f));

    _debugNode->drawRect(Vec2::ZERO, Vec2(size.width, size.height), Color4F(0, 0, 1, 0.5f));

    _debugNode->drawDot(centerOffset, 5.0f, Color4F::MAGENTA);

    if (_currentState == State::ATTACKING) {
        float dir = _isFacingRight ? 1.0f : -1.0f;
        Vec2 origin = Vec2(dir * 20, 30);
        Vec2 dest = origin + Vec2(dir * 60, 40);
        _debugNode->drawRect(origin, dest, Color4F(1, 0, 0, 0.6f));
    }
}

// =================================================================
//  4. 状态机与输入 (State & Input)
// =================================================================

void Player::updateStateMachine()
{
    if (_isAttacking || _currentState == State::DAMAGED) return;

    State targetState = _currentState;

    if (_isOnGround)
    {
        if (std::abs(_velocity.x) > 10.0f) targetState = State::RUNNING;
        else targetState = State::IDLE;
    }
    else
    {
        if (_velocity.y > 0) targetState = State::JUMPING;
        else targetState = State::FALLING;
    }

    if (targetState != _currentState)
    {
        changeState(targetState);
    }
}

void Player::changeState(State newState, bool force)
{
    if (_currentState == State::DAMAGED && newState != State::DAMAGED && !force)
    {
        return;
    }

    if (_currentState != State::ATTACKING) {
        this->stopActionByTag(101);
    }

    _currentState = newState;
    _isAttacking = (newState == State::ATTACKING);

    Action* newAction = nullptr;
    switch (_currentState)
    {
    case State::IDLE:
        if (_idleAnim) newAction = RepeatForever::create(Animate::create(_idleAnim));
        break;
    case State::RUNNING:
        if (_runAnim) newAction = RepeatForever::create(Animate::create(_runAnim));
        break;
    case State::JUMPING:
        if (_jumpAnim) newAction = RepeatForever::create(Animate::create(_jumpAnim));
        break;
    case State::FALLING:
        if (_fallAnim) newAction = RepeatForever::create(Animate::create(_fallAnim));
        break;
    case State::DAMAGED:
        if (_damageAnim) newAction = Animate::create(_damageAnim);
        break;
    default: 
        break;
    }

    if (newAction) {
        newAction->setTag(101);
        this->runAction(newAction);
    }

    this->setAnchorPoint(Vec2(0.5f, 0.0f));
}

// =================================================================
//  5. 动作接口
// =================================================================

void Player::moveLeft()
{
    if (_isAttacking || _currentState == State::DAMAGED) return;
    _velocity.x = -_moveSpeed;
    _isFacingRight = false;
    this->setFlippedX(false);
}

void Player::moveRight()
{
    if (_isAttacking || _currentState == State::DAMAGED) return;
    _velocity.x = _moveSpeed;
    _isFacingRight = true;
    this->setFlippedX(true);
}

void Player::stopMove()
{
    _velocity.x = 0;
}

void Player::jump()
{
    if (_isOnGround && !_isAttacking && _currentState != State::DAMAGED)
    {
        _velocity.y = _jumpForce;
        _isOnGround = false;
    }
}

void Player::attack()
{
    if (_isAttacking || !_isOnGround || _currentState == State::DAMAGED) return;

    changeState(State::ATTACKING);
    _velocity.x = 0;

    this->scheduleOnce([this](float dt) {
        _isAttacking = false;
        changeState(State::IDLE);
    }, 0.3f, "attack_end_key");
}

cocos2d::Rect Player::getAttackHitbox() const
{
    if (_currentState != State::ATTACKING) return Rect::ZERO;

    float attackRange = 80.0f;
    float attackHeight = 60.0f;
    float innerOffset = 20.0f;

    Vec2 pos = this->getPosition();
    float startY = pos.y + _bodyOffset.y + 10.0f;

    float startX;
    if (_isFacingRight) {
        startX = pos.x - innerOffset;
    }
    else {
        startX = pos.x + innerOffset - attackRange;
    }

    return Rect(startX, startY, attackRange, attackHeight);
}

// =================================================================
//  6. 资源加载
// =================================================================
void Player::initAnimations()
{
    Vector<SpriteFrame*> idleFrames;
    for (int i = 1; i <= 9; i++)
    {
        std::string filename = StringUtils::format("Knight/idle/idle_%d.png", i);
        auto sprite = Sprite::create(filename);
        if (sprite) idleFrames.pushBack(sprite->getSpriteFrame());
    }
    if (!idleFrames.empty())
    {
        _idleAnim = Animation::createWithSpriteFrames(idleFrames, 0.15f);
        _idleAnim->retain();
    }

    Vector<SpriteFrame*> runFrames;
    for (int i = 1; i <= 13; i++)
    {
        std::string filename = StringUtils::format("Knight/run/run_%d.png", i);
        auto sprite = Sprite::create(filename);
        if (sprite) runFrames.pushBack(sprite->getSpriteFrame());
    }
    if (!runFrames.empty())
    {
        _runAnim = Animation::createWithSpriteFrames(runFrames, 0.15f);
        _runAnim->retain();
    }

    Vector<SpriteFrame*> dashFrames;
    for (int i = 1; i <= 12; i++)
    {
        std::string filename = StringUtils::format("Knight/dash/dash_%d.png", i);
        auto sprite = Sprite::create(filename);
        if (sprite) dashFrames.pushBack(sprite->getSpriteFrame());
    }
    if (!dashFrames.empty())
    {
        _dashAnim = Animation::createWithSpriteFrames(dashFrames, 0.15f);
        _dashAnim->retain();
    }

    Vector<SpriteFrame*> jumpFrames;
    for (int i = 1; i <= 6; i++)
    {
        std::string filename = StringUtils::format("Knight/jump/jump_%d.png", i);
        auto sprite = Sprite::create(filename);
        if (sprite) jumpFrames.pushBack(sprite->getSpriteFrame());
    }
    if (!jumpFrames.empty())
    {
        _jumpAnim = Animation::createWithSpriteFrames(jumpFrames, 0.15f);
        _jumpAnim->retain();
    }

    Vector<SpriteFrame*> fallFrames;
    for (int i = 1; i <= 6; i++)
    {
        std::string filename = StringUtils::format("Knight/fall/fall_%d.png", i);
        auto sprite = Sprite::create(filename);
        if (sprite) fallFrames.pushBack(sprite->getSpriteFrame());
    }
    if (!fallFrames.empty())
    {
        _fallAnim = Animation::createWithSpriteFrames(fallFrames, 0.15f);
        _fallAnim->retain();
    }

    Vector<SpriteFrame*> damageFrames;
    for (int i = 1; i <= 4; i++)
    {
        std::string filename = StringUtils::format("knight/damage/damage_%d.png", i);
        auto sprite = Sprite::create(filename);
        if (sprite) 
        {
            damageFrames.pushBack(sprite->getSpriteFrame());
            CCLOG("  ✓ Loaded damage frame: %s", filename.c_str());
        }
        else
        {
            CCLOG("  ✗ Failed to load damage frame: %s", filename.c_str());
        }
    }
    if (!damageFrames.empty())
    {
        _damageAnim = Animation::createWithSpriteFrames(damageFrames, 0.1f);
        _damageAnim->retain();
        CCLOG("✓ Damage animation created with %d frames", (int)damageFrames.size());
    }
    else
    {
        _damageAnim = nullptr;
        CCLOG("✗ Damage animation failed to load!");
    }
}

// ======================================================================
// 【优化】玩家受伤逻辑
// ======================================================================
void Player::takeDamage(int damage)
{
    if (_isInvincible)
    {
        CCLOG("[Player] Invincible! Ignoring damage.");
        return;
    }

    _health -= damage;
    CCLOG("💔 Player took %d damage! Health: %d/%d", damage, _health, _maxHealth);

    // ========================================
    // 1. 保存当前脚底位置（防止穿模）
    // ========================================
    Vec2 currentPos = this->getPosition();
    float groundY = currentPos.y; // 脚底的 Y 坐标

    // ========================================
    // 2. 切换到受伤状态
    // ========================================
    changeState(State::DAMAGED);

    // ========================================
    // 3. 【关键修复】立即修正位置，保持脚底不变
    // ========================================
    this->setPositionY(groundY);

    // ========================================
    // 4. 动画播放完毕后恢复 IDLE
    // ========================================
    float damageAnimDuration = 0.4f;

    this->scheduleOnce([this, groundY](float dt) {
        if (_currentState == State::DAMAGED)
        {
            changeState(State::IDLE, true);
            // 再次确保脚底位置正确
            this->setPositionY(groundY);
            CCLOG("[Player] Damage animation finished, returning to IDLE.");
        }
    }, damageAnimDuration, "damage_anim_end");

    // ========================================
    // 5. 【增强】击退效果（水平 + 垂直）
    // ========================================
    _velocity.x = 0; // 停止水平移动

    float knockbackDistanceX = 100.0f; // 增加击退距离
    float knockbackDistanceY = 80.0f;  // 向上弹起
    float knockbackDuration = 0.4f;

    float direction = _isFacingRight ? -1.0f : 1.0f;
    Vec2 knockbackTarget = Vec2(
        currentPos.x + direction * knockbackDistanceX,
        currentPos.y + knockbackDistanceY
    );

    // 使用抛物线击退
    auto jumpTo = JumpTo::create(knockbackDuration, knockbackTarget, knockbackDistanceY, 1);
    this->runAction(jumpTo);

    // ========================================
    // 6. 进入无敌状态（3秒）
    // ========================================
    _isInvincible = true;

    // 闪烁效果
    auto fadeOut = FadeTo::create(0.1f, 100);
    auto fadeIn = FadeTo::create(0.1f, 255);
    auto blinkSequence = Sequence::create(fadeOut, fadeIn, nullptr);
    auto blinkForever = RepeatForever::create(blinkSequence);
    blinkForever->setTag(999);
    this->runAction(blinkForever);

    // 3秒后结束无敌
    this->scheduleOnce([this](float dt) {
        _isInvincible = false;
        this->stopActionByTag(999);
        this->setOpacity(255);
        CCLOG("[Player] Invincibility ended (3 seconds).");
    }, 3.0f, "invincibility_end");

    // ========================================
    // 7. 检查死亡
    // ========================================
    if (_health <= 0)
    {
        CCLOG("☠ Player is dead!");
    }
}