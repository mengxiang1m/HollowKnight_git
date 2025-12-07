#include "Player.h"

USING_NS_CC;

Player* Player::create(const std::string& filename)
{
    Player* player = new (std::nothrow) Player();
    std::string initFile = filename.empty() ? "Knight/idle_1.png" : filename;

    if (player && player->initWithFile(initFile) && player->init())
    {
        player->autorelease();
        return player;
    }
    CC_SAFE_DELETE(player);
    return nullptr;
}

bool Player::init()
{
    //坐标点 (Position) 位于图片的 "底边中点"
    this->setAnchorPoint(Vec2(0.5f, 0.0f));

    // 2. 初始化物理参数 (按需调整手感)
    _moveSpeed = 300.0f;
    _velocity = Vec2::ZERO;
    _gravity = 2000.0f;   // 重力感，大一点下落快，手感好
    _jumpForce = 700.0f;  // 跳跃力度

    // 3. 初始化状态
    _isFacingRight = false;
    _isAttacking = false;
    _isOnGround = false;
    _currentState = State::IDLE;

    _idleAnim = nullptr;
    _dashAnim = nullptr;
    _runAnim = nullptr;
    _attackAnim = nullptr;

    // 4. 加载动画资源
    initAnimations();

	// 5.  主角的DrawNode 
    _debugNode = DrawNode::create();
    this->addChild(_debugNode, 999); // 放在最上层

    changeState(State::IDLE);

    // 1. 获取图片原始尺寸
    Size originalSize= this->getContentSize();

    // ============================================================
    // 【核心配置】 定义完美的“骑士肉体”
    // ============================================================
    // 底部30像素是空的，切掉
    float bottomPadding = 30.0f;

    // 宽度设定：比图片稍窄
    // 这样不仅贴合身体，还能避免在通过狭窄地形时卡住
    float widthShrink = 10.0f;

    // --- A. 设置固定大小 ---
    // 宽 = 原图宽 - 缩进
    // 高 = 原图高 - 底部留白
    _bodySize = Size(originalSize.width - widthShrink, originalSize.height - bottomPadding);

    // --- B. 设置偏移量 ---
    // 这里的偏移是相对于锚点位置（假设锚点在图片底边中心）
    // Y轴：因为底部有30像素空白，所以物理框要向上抬 30
    // X轴：通常为0（居中）
    _bodyOffset = Vec2(0, bottomPadding);
    // 注意：这里不需要 scheduleUpdate，因为 update 是由 HelloWorldScene 手动调用的
    return true;
}

cocos2d::Rect Player::getCollisionBox() const
{
    // 获取主角脚底坐标 (世界坐标)
    Vec2 pos = this->getPosition();

    // 计算碰撞箱的左下角坐标 (Origin)
    //  Sprite 锚点是 (0.5, 0)，即 pos 是脚底中心
    float x = pos.x - _bodySize.width * 0.5f + _bodyOffset.x;
    float y = pos.y + _bodyOffset.y; // 脚底对齐

    return cocos2d::Rect(x, y, _bodySize.width, _bodySize.height);
}

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
}

// =================================================================
//  输入控制 (只改变物理参数，不直接改状态)
// =================================================================

void Player::moveLeft()
{
    if (_isAttacking) return; // 攻击硬直中不能移动

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
    // 松开按键只归零速度，状态切换交给 updateStateMachine
    _velocity.x = 0;
}

void Player::jump()
{
    // 只有在地面上才能跳
    if (_isOnGround && !_isAttacking)
    {
        _velocity.y = _jumpForce;
        _isOnGround = false; // 离地瞬间
        // 这里不需要手动 changeState，下一帧物理更新会自动检测到速度向上而切到 JUMPING
        CCLOG("Jump!");
    }
}

void Player::attack()
{
    if (_isAttacking || !_isOnGround) return; // 防止空中攻击或连点

    // 攻击是特殊状态，需要强制切换
    changeState(State::ATTACKING);
    _velocity.x = 0; // 攻击时站定

    // 模拟攻击动画结束 (0.3秒后)
    this->scheduleOnce([this](float dt) {
        _isAttacking = false;
        changeState(State::IDLE); // 恢复待机
        }, 0.3f, "attack_end_key");
}

// =================================================================
//  核心循环 (每帧调用)
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

    // 专门用于渲染调试框的函数
     drawDebugRects();
}

void Player::drawDebugRects()
{
    if (!_debugNode) return;
    _debugNode->clear();

    _debugNode->drawDot(Vec2::ZERO, 5.0f, cocos2d::Color4F::MAGENTA);
    // 1. 画物理碰撞箱 (绿色实心 - 逻辑真理)
    // 这里的 getCollisionBox 返回的是世界坐标，我们需要转回 Node 坐标系画
    // 或者直接让 debugNode 画在世界位置（比较麻烦），最简单的是算出相对位置

    Rect worldBox = getCollisionBox();
    Vec2 myPos = this->getPosition();

    // 计算相对于 Player 锚点(0,0) 的矩形
    Vec2 origin = Vec2(0,0);
    Vec2 dest = origin + Vec2(_bodySize.width,_bodySize.height);

    _debugNode->drawSolidRect(origin, dest, Color4F(0, 1, 0, 0.4f)); // 半透明绿

    // 2. 画图片轮廓 (蓝色空心 - 仅供参考)
    Size contentSize = this->getContentSize();

    _debugNode->drawRect(
        Vec2(0, 0),
        Vec2(contentSize.width , contentSize.height),
        Color4F::BLUE
    );

    // 3. 画攻击判定框 (红色 - 如果有)
    if (_currentState == State::ATTACKING) {
        Rect attackBox = getAttackHitbox(); // 假设这个函数返回世界坐标
        // 转本地坐标
        Vec2 atOrigin = attackBox.origin - myPos;
        Vec2 atDest = atOrigin + attackBox.size;
        _debugNode->drawSolidRect(atOrigin, atDest, Color4F(1, 0, 0, 0.5f));
    }
}

void Player::updateMovementX(float dt)
{
    // 仅应用位移，不改变速度
    float dx = _velocity.x * dt;
    this->setPositionX(this->getPositionX() + dx);
}

void Player::updateCollisionX(const std::vector<cocos2d::Rect>& platforms)
{
    // 获取当前(移动后)的碰撞箱
    Rect playerRect = getCollisionBox();

    for (const auto& wall : platforms)
    {
        if (playerRect.intersectsRect(wall))
        {
            // 发生碰撞，计算修正
            // 策略：根据速度方向决定将主角推到墙的哪一侧

            if (_velocity.x > 0) // 向右撞墙
            {
                // 修正 X 坐标：墙的左边 - 主角宽度的一半
                float newX = wall.getMinX() - _bodySize.width * 0.5f - _bodyOffset.x;
                this->setPositionX(newX);
            }
            else if (_velocity.x < 0) // 向左撞墙
            {
                // 修正 X 坐标：墙的右边 + 主角宽度的一半
                float newX = wall.getMaxX() + _bodySize.width * 0.5f - _bodyOffset.x;
                this->setPositionX(newX);
            }

            // 撞墙后速度归零（可选，防止持续贴墙导致的状态误判）
            // _velocity.x = 0; 

            // 优化：X轴碰撞通常只需要修正一次即可退出（除非你正好卡在两个砖块中间）
            // break; 
        }
    }
}

void Player::updateMovementY(float dt)
{
    // 1. 应用重力
    _velocity.y -= _gravity * dt;
    // 限制最大下落速度 (Terminal Velocity)
    if (_velocity.y < -1500.0f) _velocity.y = -1500.0f;

    // 2. 应用位移
    float dy = _velocity.y * dt;
    this->setPositionY(this->getPositionY() + dy);
}

void Player::updateCollisionY(const std::vector<cocos2d::Rect>& platforms)
{
    // 默认设为false，只有真正踩到地才设为true
    _isOnGround = false;

    Rect playerRect = getCollisionBox();

    for (const auto& platform : platforms)
    {
        if (playerRect.intersectsRect(platform))
        {
            // 区分是 落地 还是 顶头
            // 这是一个简单的判定：看当前速度方向，或者看重叠区域的相对位置

            if (_velocity.y <= 0) // 下落中
            {
                // 【核心修复2】判定是否真的是“踩”在上面
                // 只有当 物理框的脚底 (MinY) 接近 平台顶部 (MaxY) 时才算落地
                // 这里的 tolerance 是允许陷入的深度，不能太大，否则侧面撞墙会被吸上去
                float tolerance = 20.0f; // 容错范围
                float overlapY = platform.getMaxY() - playerRect.getMinY();

                // 如果陷入深度在容错范围内，说明是踩在上面
                if (overlapY > 0 && overlapY <= tolerance)
                {
                    // === 落地修正 ===

                    // 【核心修复1】减去 _bodyOffset.y (30像素)
                    // 让物理框的底部贴合平台，而不是图片的底部贴合平台
                    float newY = platform.getMaxY() - _bodyOffset.y;
                    this->setPositionY(newY);

                    _velocity.y = 0;
                    _isOnGround = true;
                }
            }
            else if (_velocity.y > 0) // 上升中 (顶头)
            {
                // === 顶头修正 ===
                // 新的Y = 平台底部 - 主角身高
                float newY = platform.getMinY() - _bodySize.height;
                this->setPositionY(newY- _bodyOffset.y);

                _velocity.y = 0; // 撞头后失去向上动能
            }
        }
    }
}

void Player::updateStateMachine()
{
    // 如果正在攻击，锁定状态，不接受物理状态的切换
    if (_isAttacking) return;

    State targetState = _currentState;

    // === 根据物理条件判断目标状态 ===

    if (_isOnGround)
    {
        // 在地面：根据水平速度决定是走路还是站
        if (std::abs(_velocity.x) > 10.0f) {
            targetState = State::RUNNING;
        }
        else {
            targetState = State::IDLE;
        }
    }
    else
    {
        // 在空中：根据垂直速度决定是跳还是落
        if (_velocity.y > 0) {
            targetState = State::JUMPING;
        }
        else {
            targetState = State::FALLING;
        }
    }

    // === 执行切换 ===
    if (targetState != _currentState)
    {
        changeState(targetState);
    }
}

void Player::changeState(State newState)
{
    // 清理旧状态 (OnExit)
    // 例如：停止旧动画
    // this->stopAllActions(); // 如果想保留攻击特效，可能需要更精细的控制，暂时全停

    // 也可以只停止 Tag 为动画的 Action，避免停止 scheduleOnce
    // 这里为了简单，如果不是攻击状态，我们重置动画
    if (_currentState != State::ATTACKING) {
        this->stopActionByTag(101); // 假设 101 是动画动作的 Tag
    }

    _currentState = newState;
    _isAttacking = (newState == State::ATTACKING);

    // 进入新状态 (OnEnter)
    Action* newAction = nullptr;

    switch (_currentState)
    {
    case State::IDLE:
        if (_idleAnim) {
            newAction = RepeatForever::create(Animate::create(_idleAnim));
        }
        break;

    case State::RUNNING:
        if (_runAnim) {
            newAction = RepeatForever::create(Animate::create(_runAnim));
        }
        break;

    case State::DASHING:
        if (_dashAnim) {
            newAction = RepeatForever::create(Animate::create(_dashAnim));
        }
        break;

    case State::JUMPING:
        // 播放起跳帧
        break;

    case State::FALLING:
        // 播放下落帧
        break;

    case State::ATTACKING:
        // 攻击逻辑已在 attack() 函数中处理了，这里可以放专门的动画
        break;
    }

    if (newAction) {
        newAction->setTag(101); // 打个标签方便管理
        this->runAction(newAction);
    }

}

cocos2d::Rect Player::getAttackHitbox() const
{
    // 如果没有处于攻击状态，返回一个空矩形（打不到任何人）
    // 不用在外部判断 state == ATTACKING ，直接调这个函数
    if (_currentState != State::ATTACKING) {
        return cocos2d::Rect::ZERO;
    }

    Size size = this->getContentSize();
    // 根据朝向调整判定框位置
    float dir = _isFacingRight ? 1.0f : -1.0f;

    // 在前方生成一个矩形
    return Rect(
        getPositionX() + (dir * size.width * 0.5f),
        getPositionY() - size.height * 0.5f,
        80, // 攻击距离
        60  // 攻击高度
    );// ==========================================
    // 攻击框配置 (Hitbox Config)
    // ==========================================
    // 刀的攻击距离（向前延伸多少）
    float attackRange = 100.0f;
    // 刀的覆盖高度
    float attackHeight = 80.0f;
    // 稍微还要往后一点点（防止贴脸打不到）
    float offsetInner = 20.0f;

    // 获取主角当前位置（作为基准点）
    Vec2 pos = this->getPosition();

    // 根据朝向计算
    float originX;
    if (_isFacingRight) {
        // 向右：从主角中心偏左一点点开始，向右延伸
        originX = pos.x - offsetInner;
    }
    else {
        // 向左：从主角中心偏右一点点开始，向左延伸（注意 Rect 原点在左下角）
        // 原点 X = 主角X + 内部偏移 - 总宽度
        originX = pos.x + offsetInner - attackRange;
    }

    // Y轴：假设攻击判定在胸口高度
    // pos.y 是脚底（如果锚点是0.5, 0），或者是中心（如果锚点是0.5, 0.5）
    // 这里假设 pos.y 是经过物理修正后的中心点
    float originY = pos.y - (attackHeight * 0.5f);

    return cocos2d::Rect(originX, originY, attackRange, attackHeight);
}