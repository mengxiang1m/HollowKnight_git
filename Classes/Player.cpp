#include "Player.h"
#include "PlayerStates.h" // 引入状态类的实现
#include "config.h"   
#include "HelloWorldScene.h"

USING_NS_CC;

// =================================================================
//  1. 生命周期 (Lifecycle)
// =================================================================

Player* Player::create(const std::string& filename)
{
    // 注意：这里为了架构整洁，忽略了 filename 参数，直接在 init 里加载 Config 指定的资源
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
    // 1. 加载初始纹理
    if (!this->initWithFile("Knight/idle/idle_1.png"))
    {
        CCLOG("Error: Failed to load 'Knight/idle/idle_1.png'");
        return false;
    }

    // 2. 设置锚点 (底边中心)
    this->setAnchorPoint(Vec2(0.5f, 0.0f));

    // 3. 初始化物理碰撞箱参数
    Size size = this->getContentSize();
    float bodyW = size.width * 0.6f;
    float bottomGap = 30.0f;
    float bodyH = size.height - bottomGap;

    _bodySize = Size(bodyW, bodyH);
    _bodyOffset = Vec2(0, bottomGap);
    _localBodyRect = Rect(-bodyW * 0.5f, bottomGap, bodyW, bodyH);

    // 4. 初试化变量
    // ==========================================
    // 【核心】初始化 Stats 组件
    // ==========================================
    _stats = new PlayerStats();
    // 依赖注入：把 Config 里的数值传给 Stats
    _stats->initStats(Config::DEFAULT_PLAYER_CFG);

    // 绑定回调：Stats 数据变了 -> 通知 Player -> Player 通知 UI
    _stats->onHealthChanged = [this](int cur, int max) {
        if (_onHealthChanged) _onHealthChanged(cur, max);
        };
    _stats->onSoulChanged = [this](int cur) {
        if (_onSoulChanged) _onSoulChanged(cur);
        };

    // 其他变量初始化
    _velocity = Vec2::ZERO;
    _isInvincible = false; // 注意：_isInvincible 属于战斗状态，暂留 Player 或移入 Stats 均可
    _isFacingRight = false;
    _isOnGround = false;

    // 输入标记
    _inputDirectionX = 0;
    _inputDirectionY = 0;
    _isAttackPressed = false;
    _jumpTimer = 0.0f;
    _isJumpingAction = false;
    _isJumpPressed = false;
    _isFocusInputPressed = false;

    // 5. 加载所有动画资源
    _animator = new PlayerAnimator();
    _animator->init(this); // 把自己传进去，让 Animator 把特效贴在我身上

    // 6. 调试绘图
    _debugNode = DrawNode::create();
    this->addChild(_debugNode, 999);

    // 7. 启动状态机 - 进入待机状态
    this->changeState(new StateIdle());

    return true;
}

// =================================================================
//  2. 核心循环 (Core Loop)
// =================================================================

void Player::update(float dt, const std::vector<cocos2d::Rect>& platforms)
{
    // 1. 【逻辑层】委托给状态机处理
    if (_state)
    {
        _state->update(this, dt);
    }

    // 2. 【物理层】执行位移和碰撞
    updateMovementX(dt);
    updateCollisionX(platforms);

    updateMovementY(dt);
    updateCollisionY(platforms);

}

// =================================================================
//  3. 状态机接口 (State Machine Interface)
// =================================================================

void Player::changeState(PlayerState* newState)
{
    if (_state)
    {
        _state->exit(this);
        delete _state;
    }

    _state = newState;

    if (_state)
    {
        _state->enter(this);
    }
}

// =================================================================
//  4. 动作执行接口 (Actions)
// =================================================================

void Player::moveInDirection(int dir)
{
    // 使用 Config::Player::MOVE_SPEED
    _velocity.x = dir * Config::Player::MOVE_SPEED;

    // 处理翻转
    if (dir != 0)
    {
        bool newFacingRight = (dir > 0);
        if (_isFacingRight != newFacingRight)
        {
            _isFacingRight = newFacingRight;
            this->setFlippedX(_isFacingRight);
        }
    }
}

void Player::setVelocityX(float x)
{
    _velocity.x = x;
}

void Player::setVelocityY(float y)
{
    _velocity.y = y;
}

void Player::startJump()
{
    _isOnGround = false;
    _isJumpingAction = true;
    _jumpTimer = 0.0f;
    _velocity.y = Config::Player::JUMP_FORCE_BASE;
}

void Player::stopJump()
{
    _isJumpingAction = false;

    // 截断跳跃 - 手感优化
    if (_velocity.y > 0)
    {
        _velocity.y *= 0.5f;
    }
}

void Player::attack()
{
    _animator->playAttackEffect(_currentAttackDir, _isFacingRight);
}

void Player::startFocusEffect()
{
    _animator->startFocusEffect();
}

void Player::stopFocusEffect()
{
    _animator->stopFocusEffect();
}

void Player::playFocusEndEffect()
{
    _animator->playFocusEndEffect();
}

void Player::takeDamage(int damage)
{
    // 1. 状态检查
    // 如果已经死了，或者正在无敌中，直接无视伤害
    if (_isInvincible || _stats->isDead()) return;

    // 2. 数据层处理：扣血
    _stats->takeDamage(damage);
    CCLOG("Player took damage! Health: %d", _stats->getHealth());

    // 3. 开启无敌 (Invincible)
    _isInvincible = true;

    // 闪烁特效 (Blink)
    auto blink = RepeatForever::create(Sequence::create(
        FadeTo::create(0.1f, 100),
        FadeTo::create(0.1f, 255),
        nullptr
    ));
    blink->setTag(999);
    this->runAction(blink);

    // 2.0秒后取消无敌
    this->scheduleOnce([this](float dt) {
        _isInvincible = false;
        this->stopActionByTag(999);
        this->setOpacity(255);
        }, 1.0f, "invincible_end");

    // 4. 物理击退 (Knockback)
    // 根据朝向反向弹开
    float direction = _isFacingRight ? -1.0f : 1.0f;
    _velocity.x = direction * 400.0f; // 向后的速度 (数值可调)
    _velocity.y = 600.0f;             // 给一点向上的小跳跃，防卡地

    // 5. 切换到受击状态
    changeState(new StateDamaged());
}

void Player::executeHeal()
{
    // 调用组件尝试回血
    bool success = _stats->recoverHealth();
}

void Player::pogoJump()
{
    _velocity.y = 0;
    // 给一个向上的瞬时速度 (类似跳跃)
    _velocity.y = Config::Player::JUMP_FORCE_BASE*1.2f;
    _isOnGround = false;
}

// =================================================================
//  5. 动画系统
// =================================================================

void Player::playAnimation(const std::string& animName)
{
    _animator->playAnimation(animName);
}

// =================================================================
//  6. 输入设置
// =================================================================
void Player::setInputDirectionX(int dir) { _inputDirectionX = dir; }
void Player::setInputDirectionY(int dir) { _inputDirectionY = dir; }
void Player::setAttackPressed(bool pressed) { _isAttackPressed = pressed; }
void Player::setJumpPressed(bool pressed) { _isJumpPressed = pressed; }
void Player::setAttackDir(int dir) { _currentAttackDir = dir; }
void Player::setFocusInput(bool pressed) { _isFocusInputPressed = pressed; }

// =================================================================
//  7. 物理引擎 (使用 Config)
// =================================================================

void Player::updateMovementX(float dt)
{
    float dx = _velocity.x * dt;
    this->setPositionX(this->getPositionX() + dx);
}

void Player::updateMovementY(float dt)
{
    // 长按跳跃增高
    if (_isJumpingAction)
    {
        _jumpTimer += dt;
        // 使用 Config::Player::MAX_JUMP_TIME
        if (_jumpTimer < Config::Player::MAX_JUMP_TIME) {
            // 使用 Config::Player::JUMP_ACCEL
            _velocity.y += Config::Player::JUMP_ACCEL * dt;
        }
        else {
            _isJumpingAction = false;
        }
    }

    // 重力: 使用 Config::Player::GRAVITY
    _velocity.y -= Config::Player::GRAVITY * dt;

    // 终端速度: 使用 Config::Player::MAX_FALL_SPEED
    if (_velocity.y < Config::Player::MAX_FALL_SPEED)
        _velocity.y = Config::Player::MAX_FALL_SPEED;

    float dy = _velocity.y * dt;
    this->setPositionY(this->getPositionY() + dy);
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
                    this->setPositionX(wall.getMinX() - _bodySize.width * 0.5f - _bodyOffset.x - 0.1f);
                else if (_velocity.x < 0)
                    this->setPositionX(wall.getMaxX() + _bodySize.width * 0.5f - _bodyOffset.x + 0.1f);
            }
        }
    }
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

            if (overlapX > playerRect.size.width * 0.1f)
            {
                if (_velocity.y <= 0)
                {
                    float tolerance = 40.0f;
                    float overlapY = platform.getMaxY() - playerRect.getMinY();

                    if (overlapY > -0.1f && overlapY <= tolerance)
                    {
                        this->setPositionY(platform.getMaxY() - _bodyOffset.y - 1.0f);
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
                        this->setPositionY(platform.getMinY() - _bodySize.height - _bodyOffset.y);
                        _velocity.y = 0;
                    }
                }
            }
        }
    }
}

// =================================================================
//  8. 辅助函数
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

cocos2d::Rect Player::getAttackHitbox() const
{
    Vec2 pos = this->getPosition();

    // ================== 上劈判定 ==================
    if (_currentAttackDir == 1)
    {
        float w = 100.0f;
        float h = 130.0f;
        float startX = pos.x - w / 2 + _bodyOffset.x;
        // Y轴：从头顶往下一点点开始，向上延伸
        float startY = pos.y + _bodySize.height + _bodyOffset.y - 30.0f;
        return Rect(startX, startY, w, h);
    }

    // ================== 下劈判定 ==================
    else if (_currentAttackDir == -1)
    {
        float w = 100.0f;
        float h = 120.0f;
        // X轴：居中
        float startX = pos.x - w / 2 + _bodyOffset.x;

        // Y轴：【关键调整】
        // 让判定框从“脚踝以上”就开始，一直延伸到“脚底以下很深”
        // pos.y + _bodyOffset.y 是脚底板位置
        // 我们让它从脚底板 向上 40像素开始，向下延伸 120像素
        // 这样即使敌人和主角重叠（在身体里），也能砍到
        float startY = pos.y + _bodyOffset.y - h + 40.0f;

        return Rect(startX, startY, w, h);
    }

    // ================== 水平判定 (原逻辑) ==================
    // 1. 参数配置 (根据手感调整)
    else {
        float attackRange = 120.0f; // 刀长
        float attackHeight = 70.0f; // 刀高
        float innerOffset = 20.0f;  // 向身后延伸一点点，防止贴身打不到

        float startY = pos.y + _bodyOffset.y + 10.0f;
        float startX = _isFacingRight ? (pos.x - innerOffset) : (pos.x + innerOffset - attackRange);

        return Rect(startX, startY, attackRange, attackHeight);
    }
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
    Vec2 greenMin = _localBodyRect.origin + centerOffset;
    Vec2 greenMax = greenMin + _localBodyRect.size;
    _debugNode->drawSolidRect(greenMin, greenMax, Color4F(0, 1, 0, 0.4f));

    // B. 画图片轮廓 (空心蓝)
    _debugNode->drawRect(Vec2::ZERO, Vec2(size.width, size.height), Color4F(0, 0, 1, 0.5f));

    // C. 画锚点位置 (品红点)
    _debugNode->drawDot(centerOffset, 5.0f, Color4F::MAGENTA);

    // D. 画攻击判定框 (红色)
    if (_debugNode) // 建议加个判断防止崩溃
    {
        // --- 1. 参数 ---
        float attackRange = 120.0f;
        float attackHeight = 70.0f;
        float innerOffset = 20.0f;

        // --- 2. Y 轴计算 ---
        float startY = 0 + _bodyOffset.y + 10.0f;

        // --- 3. X 轴计算 ---
        float startX;
        if (_isFacingRight) {
            // 这里用到了 size
            startX = (size.width / 2) - innerOffset;
        }
        else {
            // 这里也用到了 size
            startX = (size.width / 2) + innerOffset - attackRange;
        }

        // --- 4. 绘制 ---
        Vec2 origin(startX, startY);
        Vec2 dest(startX + attackRange, startY + attackHeight);

        // 注意：DrawNode 是画在 Player 节点内部的，所以坐标是相对坐标，不要加 getPosition()
        _debugNode->drawRect(origin, dest, Color4F(1, 0, 0, 0.6f));
    }

}

// 1. 修复 LNK2019 报错：canFocus 未实现
bool Player::canFocus() const
{
    // 安全检查，防止 stats 还没初始化就调用崩溃
    if (!_stats) return false;
    return _stats->canFocus();
}

// 2. 修复 gainSoul (转发给组件)
void Player::gainSoul(int amount)
{
    if (_stats) {
        _stats->gainSoul(amount);
    }
}

void Player::gainSoulOnKill()
{
    if (_stats) {
        _stats->gainSoulOnKill();
    }
}

// 3. 修复 UI 回调绑定
void Player::setOnHealthChanged(const std::function<void(int, int)>& callback)
{
    _onHealthChanged = callback; // 保存到 Player 自身
    if (_stats) {
        _stats->onHealthChanged = callback; // 同时设置给 Stats 组件
    }
}

void Player::setOnSoulChanged(const std::function<void(int)>& callback)
{
    _onSoulChanged = callback;
    if (_stats) {
        _stats->onSoulChanged = callback;
    }
}