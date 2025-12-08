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
    float bodyW = size.width * 0.6f;
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
    
    // 【新增】可变跳跃参数
    _isJumpingAction = false;
    _jumpTimer = 0.0f;
    _maxJumpTime = 0.35f;

    _isFacingRight = false;  // 【修改】初始朝向改为向左
    _isAttacking = false;
    _isOnGround = false;
    _currentState = State::IDLE;

    // 生命值初始化
    _health = 5;
    _maxHealth = 5;
    _isInvincible = false;

    // 【新增】初始化攻击特效精灵
    _slashEffectSprite = Sprite::create();
    _slashEffectSprite->setPosition(Vec2(60, 70));
    _slashEffectSprite->setVisible(false);
    this->addChild(_slashEffectSprite, 10);

    // 【新增】初始化动画高度补偿
    _damageAnimHeightOffset = 0.0f;

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
    // 【修改】受伤状态时只处理X轴物理
    updateMovementX(dt);
    updateCollisionX(platforms);

    // 【关键修复】受伤状态时禁用Y轴物理，防止下沉
    if (_currentState != State::DAMAGED)
    {
        updateMovementY(dt);
        updateCollisionY(platforms);
    }

    // 只有非受伤状态才更新状态机
    if (_currentState != State::DAMAGED)
    {
        updateStateMachine();
    }

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
            // 【修改】计算 Y 轴重叠区域，防止脚底擦过地面被误判为撞墙
            float overlapY = std::min(playerRect.getMaxY(), wall.getMaxY()) -
                std::max(playerRect.getMinY(), wall.getMinY());

            // 只有当 Y 轴重叠足够大时，才认为是撞墙
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
    // ============================================================
    // 【新增】空洞骑士式可变高度跳跃
    // ============================================================
    if (_isJumpingAction)
    {
        _jumpTimer += dt;

        // 如果按住时间还在允许范围内
        if (_jumpTimer < _maxJumpTime)
        {
            // 上升加速度：持续提供向上的力
            float jumpAcc = 3000.0f;
            _velocity.y += jumpAcc * dt;
        }
        else
        {
            // 超时了，强制结束加力阶段
            _isJumpingAction = false;
        }
    }

    // ============================================================
    // 标准重力处理
    // ============================================================
    _velocity.y -= _gravity * dt;
    
    // 【修复】限制下落速度，防止穿地
    if (_velocity.y < -1200.0f) _velocity.y = -1200.0f;  // 从 -1500.0f 改为 -1200.0f

    float dy = _velocity.y * dt;
    
    // 【新增】限制单帧位移，防止穿透
    float maxDy = 50.0f;
    if (dy < -maxDy) dy = -maxDy;
    else if (dy > maxDy) dy = maxDy;
    
    this->setPositionY(this->getPositionY() + dy);
}

void Player::updateCollisionY(const std::vector<cocos2d::Rect>& platforms)
{
    _isOnGround = false;
    Rect playerRect = getCollisionBox();

    // 【调试】输出玩家碰撞箱信息
    CCLOG("[Collision Debug] Player Box: (%.1f, %.1f, %.1f, %.1f)", 
          playerRect.origin.x, playerRect.origin.y, 
          playerRect.size.width, playerRect.size.height);

    for (const auto& platform : platforms)
    {
        if (playerRect.intersectsRect(platform))
        {
            // 【调试】输出碰撞信息
            CCLOG("[Collision Debug] Platform: (%.1f, %.1f, %.1f, %.1f) | Velocity.y: %.1f", 
                  platform.origin.x, platform.origin.y,
                  platform.size.width, platform.size.height,
                  _velocity.y);

            // 【修改】计算 X 轴重叠区域，防止侧面贴墙时被错误吸附
            float overlapX = std::min(playerRect.getMaxX(), platform.getMaxX()) -
                std::max(playerRect.getMinX(), platform.getMinX());

            CCLOG("[Collision Debug] overlapX: %.1f, threshold: %.1f", 
                  overlapX, playerRect.size.width * 0.1f);

            // 【关键修复】提高X轴重叠阈值，从 0.1f 改为 0.6f
            // 只有当主角的大部分身体在平台上时，才进行Y轴修正
            if (overlapX > playerRect.size.width * 0.4f)
            {
                if (_velocity.y <= 0)
                {
                    // 【修复】增加容差值，防止高速下落时穿透
                    float tolerance = 60.0f;  // 从 40.0f 改为 60.0f
                    float overlapY = platform.getMaxY() - playerRect.getMinY();

                    if (overlapY > -0.1f && overlapY <= tolerance)
                    {
                        float newY = platform.getMaxY() - _bodyOffset.y - 1.0f;
                        this->setPositionY(newY);
                        
                        // 【新增】强制将速度清零，防止累积误差
                        _velocity.y = 0.0f;
                        _isOnGround = true;
                        
                        // 【新增】如果正在跳跃蓄力，立即停止
                        _isJumpingAction = false;
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
                        
                        // 【修复】撞到头顶时立即停止上升
                        _velocity.y = 0.0f;
                        
                        // 【新增】强制结束跳跃蓄力
                        _isJumpingAction = false;
                    }
                }
            }
        }
    }
    
    if (!_isOnGround)
    {
        CCLOG("[Collision Debug] 🔴 Player is NOT on ground");
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

    // 【修改】攻击框参数更新，与 getAttackHitbox 保持一致
    if (_currentState == State::SLASHING) {
        float attackRange = 150.0f;   // 与 getAttackHitbox 一致
        float attackHeight = 90.0f;   // 与 getAttackHitbox 一致
        float innerOffset = 20.0f;

        float startY = 0 + _bodyOffset.y + 10.0f;

        float startX;
        if (_isFacingRight) {
            startX = (size.width / 2) - innerOffset;
        }
        else {
            startX = (size.width / 2) + innerOffset - attackRange;
        }

        Vec2 origin(startX, startY);
        Vec2 dest(startX + attackRange, startY + attackHeight);
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

    // 【新增】保存旧状态，用于判断是否需要补偿高度
    State oldState = _currentState;

    // 【修改】判断改为 SLASHING
    if (_currentState != State::SLASHING) {
        this->stopActionByTag(101);
    }

    _currentState = newState;
    _isAttacking = (newState == State::SLASHING);

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
    // 【新增】SLASHING 状态处理，包含特效
    case State::SLASHING:
        if (_slashAnim) {
            // 主角攻击动画
            auto charAnimate = Animate::create(_slashAnim);
            auto finishCallback = CallFunc::create([this]() {
                _isAttacking = false;
                this->updateStateMachine();
            });
            newAction = Sequence::create(charAnimate, finishCallback, nullptr);

            // 攻击特效动画
            if (_slashEffectSprite && _slashEffectAnim)
            {
                _slashEffectSprite->setVisible(true);
                _slashEffectSprite->setFlippedX(_isFacingRight);

                float offsetX = _isFacingRight ? 60.0f : 60.0f;
                _slashEffectSprite->setPosition(Vec2(offsetX, 70));

                auto effectAnimate = Animate::create(_slashEffectAnim);
                auto hideEffect = CallFunc::create([this]() {
                    _slashEffectSprite->setVisible(false);
                });

                _slashEffectSprite->stopAllActions();
                _slashEffectSprite->runAction(Sequence::create(effectAnimate, hideEffect, nullptr));
            }
        }
        break;
    default: 
        break;
    }

    if (newAction) {
        newAction->setTag(101);
        this->runAction(newAction);
    }

    this->setAnchorPoint(Vec2(0.5f, 0.0f));
    
    // 【新增】动画高度补偿逻辑
    if (oldState != State::DAMAGED && newState == State::DAMAGED)
    {
        // 从其他状态切换到受伤状态：向上移动以补偿高度差
        float currentY = this->getPositionY();
        this->setPositionY(currentY + _damageAnimHeightOffset);
        CCLOG("[Animation] Switched to DAMAGED: Y adjusted from %.1f to %.1f (offset=%.1f)", 
              currentY, currentY + _damageAnimHeightOffset, _damageAnimHeightOffset);
    }
    else if (oldState == State::DAMAGED && newState != State::DAMAGED)
    {
        // 从受伤状态切换到其他状态：向下移动以恢复高度
        float currentY = this->getPositionY();
        this->setPositionY(currentY - _damageAnimHeightOffset);
        CCLOG("[Animation] Switched from DAMAGED: Y adjusted from %.1f to %.1f (offset=%.1f)", 
              currentY, currentY - _damageAnimHeightOffset, _damageAnimHeightOffset);
    }
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

// 【新增】可变高度跳跃 - 开始跳跃
void Player::startJump()
{
    if (_isOnGround && !_isAttacking && _currentState != State::DAMAGED)
    {
        _isOnGround = false;
        _isJumpingAction = true;
        _jumpTimer = 0.0f;

        // 起跳初速度（轻点能跳的高度）
        _velocity.y = 400.0f;

        changeState(State::JUMPING);
    }
}

// 【新增】可变高度跳跃 - 停止跳跃
void Player::stopJump()
{
    _isJumpingAction = false;

    // 松手时如果还在上升，立刻砍半速度
    if (_velocity.y > 0)
    {
        _velocity.y *= 0.5f;
    }
}

void Player::attack()
{
    if (_isAttacking || _currentState == State::DAMAGED) return;

    // 地面攻击定身，空中攻击保持惯性
    if (_isOnGround) _velocity.x = 0;

    changeState(State::SLASHING);
}

cocos2d::Rect Player::getAttackHitbox() const
{
    if (_currentState != State::SLASHING) return Rect::ZERO;

    // 【修改】增大攻击判定框
    float attackRange = 150.0f;   // 从 120.0f 改为 150.0f
    float attackHeight = 90.0f;   // 从 70.0f 改为 90.0f
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

    // 【新增】攻击动画
    Vector<SpriteFrame*> slashFrames;
    for (int i = 1; i <= 6; i++)
    {
        std::string filename = StringUtils::format("Knight/slash/slash_%d.png", i);
        auto sprite = Sprite::create(filename);
        if (sprite) slashFrames.pushBack(sprite->getSpriteFrame());
    }
    if (!slashFrames.empty())
    {
        _slashAnim = Animation::createWithSpriteFrames(slashFrames, 0.04f);
        _slashAnim->retain();
    }

    // 【新增】攻击特效动画
    Vector<SpriteFrame*> slashEffectFrames;
    for (int i = 1; i <= 6; i++)
    {
        std::string filename = StringUtils::format("Knight/slash/slashEffect/slashEffect_%d.png", i);
        auto sprite = Sprite::create(filename);
        if (sprite) slashEffectFrames.pushBack(sprite->getSpriteFrame());
    }
    if (!slashEffectFrames.empty())
    {
        _slashEffectAnim = Animation::createWithSpriteFrames(slashEffectFrames, 0.04f);
        _slashEffectAnim->retain();
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
        
        // 【修改】计算受伤动画和站立动画的高度差，并减少补偿让主角显得更低
        Size idleSize = this->getContentSize();  // 站立动画的尺寸
        Size damageSize = damageFrames.at(0)->getOriginalSize();  // 受伤动画的尺寸
        float calculatedOffset = idleSize.height - damageSize.height;
        
        // 【新增】减少50%的高度补偿，让受伤时主角更贴近地面
        _damageAnimHeightOffset = calculatedOffset * 0.5f;
        
        CCLOG("✓ Animation height offset calculated: idle=%f, damage=%f, raw_offset=%f, final_offset=%f", 
              idleSize.height, damageSize.height, calculatedOffset, _damageAnimHeightOffset);
    }
    else
    {
        _damageAnim = nullptr;
        _damageAnimHeightOffset = 0.0f;
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
    // 1. 切换到受伤状态（高度补偿在 changeState 中自动处理）
    // ========================================
    changeState(State::DAMAGED);

    // ========================================
    // 2. 【修改】增强击退效果
    // ========================================
    float knockbackSpeedX = 150.0f;  // 【修改】从 150.0f 提升到 250.0f，击退更明显
    float direction = _isFacingRight ? -1.0f : 1.0f;
    
    _velocity.x = direction * knockbackSpeedX;
    _velocity.y = 0.0f;  // 垂直速度清零

    CCLOG("[Damage] Knockback: vx=%.1f, vy=0", _velocity.x);

    // ========================================
    // 3. 【修改】延长击退持续时间，让击退距离更远
    // ========================================
    this->scheduleOnce([this](float dt) {
        _velocity.x = 0;
        CCLOG("[Damage] Knockback ended");
    }, 0.5f, "knockback_end");  // 【修改】从 0.2f 改为 0.3f，击退时间更长

    // ========================================
    // 4. 动画播放完毕后恢复 IDLE（高度恢复在 changeState 中自动处理）
    // ========================================
    this->scheduleOnce([this](float dt) {
        if (_currentState == State::DAMAGED)
        {
            changeState(State::IDLE, true);
            CCLOG("[Player] Damage animation finished, returning to IDLE.");
        }
    }, 0.4f, "damage_anim_end");

    // ========================================
    // 5. 进入无敌状态（1秒）
    // ========================================
    _isInvincible = true;

    // 【修改】1秒后结束无敌（从2秒改为1秒）
    this->scheduleOnce([this](float dt) {
        _isInvincible = false;
        CCLOG("[Player] Invincibility ended (1 second).");
    }, 1.0f, "invincibility_end");  // 从 2.0f 改为 1.0f

    // ========================================
    // 6. 检查死亡
    // ========================================
    if (_health <= 0)
    {
        CCLOG("☠ Player is dead!");
    }
}