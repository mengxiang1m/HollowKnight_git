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
    // 加载图片
    if (!this->initWithFile("Knight/idle/idle_1.png"))
    {
        CCLOG("Error: Failed to load 'Knight/idle/idle_1.png' in Player::init");
        return false;
    }

    // 设置锚点为 "底边中心"
    this->setAnchorPoint(Vec2(0.5f, 0.0f));

    // 获取加载后的正确尺寸
    Size size = this->getContentSize();
    CCLOG("Player init: Texture size = (%f, %f)", size.width, size.height);

    // ============================================================
    // 3. 定义固定的物理碰撞箱 (Physics Body)
    // ============================================================

    // 1. 宽度收缩：调整为宽度的 60%，避免太窄
    float bodyW = size.width * 0.6f;

    // 2. 高度收缩：去掉底部 30 像素的留白
    float bottomGap = 30.0f;
    float bodyH = size.height - bottomGap;

    // 3. 计算本地坐标系下的矩形 (Local Rect)
    // X: 从中心向左偏移一半宽度 (因为锚点X是0.5)
    // Y: 从底部留白处开始 (因为锚点Y是0)
    float startX = -bodyW * 0.5f;
    float startY = bottomGap;

    _localBodyRect = Rect(startX, startY, bodyW, bodyH);

    // 存一下偏移量和尺寸供计算使用
    _bodySize = Size(bodyW, bodyH);
    _bodyOffset = Vec2(0, bottomGap);

    CCLOG("Player init: Body Rect (Local) = (x:%f, y:%f, w:%f, h:%f)",
        _localBodyRect.origin.x, _localBodyRect.origin.y,
        _localBodyRect.size.width, _localBodyRect.size.height);

    // ============================================================
    // 4. 初始化参数
    // ============================================================
    _moveSpeed = 300.0f;
    _velocity = Vec2::ZERO;
    _gravity = 2000.0f;
    _jumpForce = 700.0f;
    _isJumpingAction = false;
    _jumpTimer = 0.0f;
    _maxJumpTime = 0.35f; // 【调整手感】长按最多能持续加速多久

    _isFacingRight = false;
    _isAttacking = false;
    _isOnGround = false;
    _currentState = State::IDLE;

    // 生命值初始化
    _health = 5;
    _maxHealth = 5;
    _isInvincible = false;

    // ============================================================
    // 5. 初始化特效精灵
    // ============================================================
    // 1. 创建特效精灵
    _slashEffectSprite = Sprite::create();
    // 2. 设置位置（根据你的美术资源调整，比如刀光要在身前）
    _slashEffectSprite->setPosition(Vec2(60, 60));
    // 3. 默认隐藏（只有攻击时才显示）
    _slashEffectSprite->setVisible(false);
    // 4. 加到主角身上，zOrder 设大一点，盖在主角上面
    this->addChild(_slashEffectSprite, 10);

    // ============================================================
    // 6. 初始化调试绘图节点
    // ============================================================
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
    // 受伤状态不处理移动和碰撞
    if (_currentState == State::DAMAGED)
    {
        drawDebugRects();
        return;
    }

    // 1. 处理 X 轴：先移动，立即修正
    updateMovementX(dt);
    updateCollisionX(platforms);

    // 2. 处理 Y 轴：先移动，立即修正
    updateMovementY(dt);
    updateCollisionY(platforms);

    // 3. 逻辑层：状态机流转
    updateStateMachine();

	// 4. 调试绘图
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
            // 计算 Y 轴重叠区域的高度
            float overlapY = std::min(playerRect.getMaxY(), wall.getMaxY()) -
                std::max(playerRect.getMinY(), wall.getMinY());

            // 【重要修正】只有当 Y 轴重叠足够大时，才认为是撞墙
            // 防止脚底擦过地面被误判为撞墙
            if (overlapY > playerRect.size.height * 0.5f)
            {
                if (_velocity.x > 0) // 向右撞墙
                {
                    float newX = wall.getMinX() - _bodySize.width * 0.5f - _bodyOffset.x - 0.1f;
                    this->setPositionX(newX);
                }
                else if (_velocity.x < 0) // 向左撞墙
                {
                    float newX = wall.getMaxX() + _bodySize.width * 0.5f - _bodyOffset.x + 0.1f;
                    this->setPositionX(newX);
                }
                // 撞墙后可以考虑将速度设为0，但这可能会影响手感，先注释掉
                // _velocity.x = 0; 
            }
        }
    }
}

void Player::updateMovementY(float dt)
{
    // ============================================================
    //  空洞骑士式跳跃核心逻辑
    // ============================================================
    if (_isJumpingAction)
    {
        _jumpTimer += dt;

        // 如果按住时间还在允许范围内
        if (_jumpTimer < _maxJumpTime)
        {
            // 【核心参数2】上升加速度
            // 在按住期间，持续提供向上的力来对抗重力
            // 这个值通常要比重力大，或者刚好抵消重力
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
    //  标准重力处理
    // ============================================================
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
            // 计算 X 轴重叠区域的宽度
            float overlapX = std::min(playerRect.getMaxX(), platform.getMaxX()) -
                std::max(playerRect.getMinX(), platform.getMinX());

            // 【重要修正】只有当 X 轴重叠足够大时，才进行 Y 轴修正
            // 防止侧面贴墙时被错误地吸附到墙顶或墙底
            if (overlapX > playerRect.size.width * 0.1f)
            {
                // 下落中 (或者静止在地面)
                if (_velocity.y <= 0)
                {
                    // 增加容错范围，应对高速下落
                    float tolerance = 40.0f;
                    float overlapY = platform.getMaxY() - playerRect.getMinY();

                    if (overlapY > -0.1f && overlapY <= tolerance)
                    {
                        // === 落地修正 ===
                        float newY = platform.getMaxY() - _bodyOffset.y - 1.0f;
                        this->setPositionY(newY);
                        _velocity.y = 0;
                        _isOnGround = true;
                    }
                }
                // 上升中 (顶头)
                else if (_velocity.y > 0)
                {
                    float tolerance = 20.0f;
                    float overlapY = playerRect.getMaxY() - platform.getMinY();

                    if (overlapY > 0 && overlapY <= tolerance)
                    {
                        // === 顶头修正 ===
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
    // 因为 DrawNode 原点是左下角，而锚点在 (0.5, 0)
    // 所以锚点相对于 DrawNode 的位置是 (宽的一半, 0)
    Vec2 centerOffset = Vec2(size.width * 0.5f, 0.0f);
    // A. 画物理框 (实心绿)
    Vec2 greenMin = _localBodyRect.origin+ centerOffset;
    Vec2 greenMax = greenMin + _localBodyRect.size;
    _debugNode->drawSolidRect(greenMin, greenMax, Color4F(0, 1, 0, 0.4f));

    // B. 画图片轮廓 (空心蓝)
    _debugNode->drawRect(Vec2::ZERO, Vec2(size.width, size.height), Color4F(0, 0, 1, 0.5f));

    // C. 画锚点位置 (品红点)
    _debugNode->drawDot(centerOffset, 5.0f, Color4F::MAGENTA);

    // D. 画攻击判定框 (红色)
    if (_currentState == State::SLASHING) {
        // --- 1. 参数 (必须和 getAttackHitbox 一模一样) ---
        float attackRange = 120.0f;
        float attackHeight = 70.0f;
        float innerOffset = 20.0f;

        // --- 2. Y 轴计算 ---
        // 逻辑：从脚底(0) + 身体底部留白 + 10像素
        float startY = 0 + _bodyOffset.y + 10.0f;

        // --- 3. X 轴计算 ---
        float startX;
        if (_isFacingRight) {
            // 向右：中心点 - 内缩
            startX = (size.width / 2) - innerOffset;
        }
        else {
            // 向左：中心点 + 内缩 - 刀长
            startX = (size.width / 2) + innerOffset - attackRange;
        }

        // --- 4. 绘制 ---
        Vec2 origin(startX, startY);
        Vec2 dest(startX + attackRange, startY + attackHeight);

        _debugNode->drawRect(origin, dest, Color4F(1, 0, 0, 0.6f));
    }
}

cocos2d::Rect Player::getAttackHitbox() const
{
    if (_currentState != State::SLASHING) return Rect::ZERO;

    // 1. 参数配置 (根据手感调整)
    float attackRange = 120.0f; // 刀长
    float attackHeight = 70.0f; // 刀高
    float innerOffset = 20.0f;  // 向身后延伸一点点，防止贴身打不到

    // 2. 获取基准点 (锚点/脚底中心)
    Vec2 pos = this->getPosition();
    float startY = pos.y + _bodyOffset.y + 10.0f; // 从脚踝上方开始

    float startX;
    if (_isFacingRight) {
        // 向右：从 (中心 - 20) 开始向右
        startX = pos.x - innerOffset;
    }
    else {
        // 向左：从 (中心 + 20) 开始向左 (记得 Rect 原点是左下角)
        startX = pos.x + innerOffset - attackRange;
    }

    return Rect(startX, startY, attackRange, attackHeight);
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
    case State::SLASHING:
        if (_slashAnim) {
            // --- 1. 处理主角动作  ---
            auto charAnimate = Animate::create(_slashAnim);
            auto finishCallback = CallFunc::create([this]() {
                _isAttacking = false;
                this->updateStateMachine();
                });
            newAction = Sequence::create(charAnimate, finishCallback, nullptr);

            // --- 2. 处理特效动作 ---
            if (_slashEffectSprite && _slashEffectAnim)
            {
                // A. 显示特效
                _slashEffectSprite->setVisible(true);

                // B. 同步朝向 (如果主角向右，特效也要翻转)
                _slashEffectSprite->setFlippedX(_isFacingRight);

                // C. 调整位置偏移 (如果向左，特效应该出现在左边)
                // 假设向右时特效在 x=30，向左时应该在 x=-30
                float offsetX = _isFacingRight ? 60.0f : 60.0f;
                _slashEffectSprite->setPosition(Vec2(offsetX, 70)); // Y轴根据需要调整

                // D. 创建特效动作序列：播放动画 -> 播放完隐藏
                auto effectAnimate = Animate::create(_slashEffectAnim);
                auto hideEffect = CallFunc::create([this]() {
                    _slashEffectSprite->setVisible(false);
                    });

                // E. 让特效精灵自己跑这个动作，不占用主角的 Action
                _slashEffectSprite->stopAllActions(); // 防止连点时动画重叠
                _slashEffectSprite->runAction(Sequence::create(effectAnimate, hideEffect, nullptr));
            }
        }
        break;

        // ... 其他状态
    default: break;
    }

    if (newAction) {
        newAction->setTag(101);
        this->runAction(newAction);
    }

    // 强制重置锚点，防止动画帧修改它
    this->setAnchorPoint(Vec2(0.5f, 0.0f));
}

// =================================================================
//  5. 动作接口
// =================================================================

void Player::moveLeft()
{
    if (_isAttacking) return;
    _velocity.x = -_moveSpeed;
    _isFacingRight = false;
    this->setFlippedX(false);
}

void Player::moveRight()
{
    if (_isAttacking) return;
    _velocity.x = _moveSpeed;
    _isFacingRight = true;
    this->setFlippedX(true);
}

void Player::stopMove()
{
    _velocity.x = 0;
}

// 替换原来的 jump()
void Player::startJump()
{
    // 只有在地上才能起跳 (或者你有二段跳逻辑)
    if (_isOnGround && !_isAttacking)
    {
        _isOnGround = false;
        _isJumpingAction = true; // 标记开始蓄力跳
        _jumpTimer = 0.0f;       // 重置计时器

        // 【核心参数1】起跳初速度
        // 这个值决定了“轻轻点一下”能跳多高
        // 建议设为最高跳跃速度的 50% 左右
        _velocity.y = 400.0f;

        changeState(State::JUMPING);
    }
}

void Player::stopJump()
{
    // 玩家松开按键，立刻停止向上的动力
    _isJumpingAction = false;

    // 【核心手感优化 - 截断跳跃】
    // 如果松手时还在上升，立刻把速度砍半（或设为0），实现“松手即停”的干脆感
    if (_velocity.y > 0)
    {
        _velocity.y *= 0.5f; // 乘以 0.5 甚至 0.1，越小停得越快
    }
}

void Player::attack()
{
    if (_isAttacking || _currentState == State::DAMAGED) return;

    //地面攻击通常会定身，空中攻击通常保持惯性
    if (_isOnGround) _velocity.x = 0;

    changeState(State::SLASHING);
}

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
// =================================================================
//  6. 资源加载
// =================================================================
void Player::initAnimations()
{
    // 加载待机动画
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
        CCLOG("Success: Walk animation loaded with %d frames", (int)runFrames.size());
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
        }
        else
        {
            CCLOG(" Failed to load damage frame: %s", filename.c_str());
        }
    }
    if (!damageFrames.empty())
    {
        _damageAnim = Animation::createWithSpriteFrames(damageFrames, 0.1f);
        _damageAnim->retain();
    }
    else
    {
        _damageAnim = nullptr;
        CCLOG("✗ Damage animation failed to load!");
    }
}
