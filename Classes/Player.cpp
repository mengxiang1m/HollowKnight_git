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
    // 直接在这里加载图片
    if (!this->initWithFile("Knight/idle/idle_1.png"))
    {
        CCLOG("Error: Failed to load 'Knight/idle_1.png' in Player::init");
        return false;
    }

    // 设置锚点为 "底边中心"
    this->setAnchorPoint(Vec2(0.5f, 0.0f));

    // 获取加载后的正确尺寸
    Size size = this->getContentSize();
    CCLOG("Player init: Texture size = (%f, %f)", size.width, size.height);

    // ============================================================
    // 【核心修正 3】定义固定的物理碰撞箱 (Physics Body)
    // ============================================================

    // 1. 宽度收缩：调整为宽度的 80%，避免太窄
    float bodyW = size.width * 0.8f;

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
    // 4. 初始化运动参数
    // ============================================================
    _moveSpeed = 300.0f;
    _velocity = Vec2::ZERO;
    _gravity = 2000.0f;
    _jumpForce = 700.0f;

    _isFacingRight = true;
    _isAttacking = false;
    _isOnGround = false;
    _currentState = State::IDLE;

    initAnimations();

    // ============================================================
    // 5. 初始化调试绘图节点
    // ============================================================
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
    // 1. 处理 X 轴：先移动，立即修正
    updateMovementX(dt);
    updateCollisionX(platforms);

    // 2. 处理 Y 轴：先移动，立即修正
    updateMovementY(dt);
    updateCollisionY(platforms);

    // 3. 逻辑层：状态机流转
    updateStateMachine();

    // 4. 渲染调试框
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
            if (overlapX > playerRect.size.width * 0.8f)
            {
                // 下落中 (或者静止在地面)
                if (_velocity.y <= 0)
                {
                    // 增加容错范围，应对高速下落
                    float tolerance = 40.0f;
                    float overlapY = platform.getMaxY() - playerRect.getMinY();

                    if (overlapY > 0 && overlapY <= tolerance)
                    {
                        // === 落地修正 ===
                        float newY = platform.getMaxY() - _bodyOffset.y;
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
    if (_currentState == State::ATTACKING) {
        // 简化的本地坐标攻击框示意
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
    if (_isAttacking) return;

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

void Player::changeState(State newState)
{
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
    case State::FALLING:
        if (_fallAnim) newAction = RepeatForever::create(Animate::create(_fallAnim));

        // ... 其他状态
    default: break;
    }

    if (newAction) {
        newAction->setTag(101);
        this->runAction(newAction);
    }

    // 【核心修正 5】强制重置锚点，防止动画帧修改它
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

void Player::jump()
{
    if (_isOnGround && !_isAttacking)
    {
        _velocity.y = _jumpForce;
        _isOnGround = false;
    }
}

void Player::attack()
{
    if (_isAttacking || !_isOnGround) return;

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

    // 1. 参数配置 (根据手感调整)
    float attackRange = 80.0f; // 刀长
    float attackHeight = 60.0f; // 刀高
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
//  6. 资源加载 (需要你自己实现)
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
        if (sprite) dashFrames.pushBack(sprite->getSpriteFrame());
    }

    if (!fallFrames.empty())
    {
        _fallAnim = Animation::createWithSpriteFrames(fallFrames, 0.15f);
        _fallAnim->retain();
    }
}