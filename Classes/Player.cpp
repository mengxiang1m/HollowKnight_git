#include "Player.h"

USING_NS_CC;

// 定义一个微调偏移量，如果觉得脚陷进地里，把这个改大 (例如 10.0f)
const float VISUAL_OFFSET = -30.0f;

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
    if (!Sprite::init()) return false;

    // 1. 初始化物理参数 (按需调整手感)
    _moveSpeed = 300.0f;
    _velocity = Vec2::ZERO;
    _gravity = 2000.0f;   // 重力感，大一点下落快，手感好
    _jumpForce = 700.0f;  // 跳跃力度

    // 2. 初始化状态
    _isFacingRight = true;
    _isAttacking = false;
    _isOnGround = false;
    _currentState = State::IDLE;

    _idleAnim = nullptr;
    _dashAnim = nullptr;
    _runAnim = nullptr;
    _attackAnim = nullptr;

    // 3. 加载动画资源
    initAnimations();

	// 4.  主角的DrawNode 
    _debugNode = DrawNode::create();
    this->addChild(_debugNode, 999); // 放在最上层

    changeState(State::IDLE);
    // 注意：这里不需要 scheduleUpdate，因为 update 是由 HelloWorldScene 手动调用的
    return true;
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

cocos2d::Rect Player::getBodyRect() const
{
    // 定义物理尺寸 (宽40，高120) - 这是逻辑大小，跟图片留白无关
    Size bodySize = this->getContentSize();;

    // 2. 获取主角脚底坐标 (世界坐标)
    Vec2 footPos = this->getPosition();

    return Rect(
        footPos.x ,
        footPos.y-VISUAL_OFFSET,
        bodySize.width,
        bodySize.height+VISUAL_OFFSET
    );
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
    // 1. 物理层：计算重力和位移
    updatePhysics(dt);

    // 2. 碰撞层：修正位置，判断是否落地
    updateCollision(platforms);

    // 3. 逻辑层：状态机流转
    updateStateMachine();
}

void Player::updatePhysics(float dt)
{
    // 应用重力 (始终向下)
    _velocity.y -= _gravity * dt;

    // 限制最大下落速度 (防止穿模)
    if (_velocity.y < -1000.0f) _velocity.y = -1000.0f;

    // 应用位移 (X 和 Y 都在这里暂定，Collision 会修正 Y)
    // 之前你的代码里漏了 X 轴的位移应用，导致只有动画没有移动，这里补上了
    this->setPositionX(this->getPositionX() + _velocity.x * dt);
    this->setPositionY(this->getPositionY() + _velocity.y * dt);
}

void Player::updateCollision(const std::vector<cocos2d::Rect>& platforms)
{
    // 默认假设我们在空中 (每帧重置，依靠碰撞检测来"救"回地面)
    // 这样能自动处理走下悬崖的情况
    bool wasOnGround = _isOnGround;
    _isOnGround = false;

    // 只有下落时才检测地面碰撞
    if (_velocity.y <= 0)
    {
        cocos2d::Rect playerRect = this->getBoundingBox();
        float playerHalfHeight = this->getContentSize().height * this->getAnchorPoint().y;

        for (const auto& platformRect : platforms)
        {
            if (playerRect.intersectsRect(platformRect))
            {
                // 宽松判定：只要中心点在砖块之上，就算踩中
                if (this->getPositionY() > platformRect.getMidY())
                {
                    // === 修正逻辑 ===
                    float platformTop = platformRect.getMaxY();
                    float newY = platformTop + playerHalfHeight + VISUAL_OFFSET;

                    // 1. 修正 Y 坐标
                    this->setPositionY(newY);

                    // 2. 物理重置
                    _velocity.y = 0;
                    _isOnGround = true;

                    // 3. 落地瞬间的逻辑 (可选: 播放落地音效)
                    // if (!wasOnGround) { CCLOG("Landed!"); }

                    break; // 找到一个落脚点即可
                }
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

// 1. 清空上一帧的画
    if (_debugNode) // 确保你在 init 里创建了 _debugNode
    {
        _debugNode->clear();

        // 2. 获取刚才写好的物理框 (这是世界坐标)
        cocos2d::Rect worldBody = getBodyRect();

        // 3. 把世界坐标转回本地坐标 (Local Space)
        // 原理：本地原点(0,0) 就是主角的中心
        // 所以框的本地位置 = 框的世界位置 - 主角的世界位置
        Size bodySize = worldBody.size;
        


        Vec2 bodyOrigin = Vec2(0, -VISUAL_OFFSET);
        Vec2 bodyDest = bodyOrigin + bodySize;

        _debugNode->drawSolidRect(bodyOrigin, bodyDest, Color4F(0, 1, 0, 0.5f)); // 半透明绿


        // =======================================================
    // 2. 画图片素材框 (Blue - 蓝色空心)
    // =======================================================
    // 这是图片的真实尺寸 (包含透明区域)
        Size texSize = this->getContentSize();

        // 因为锚点是 (0.5, 0)，本地原点(0,0)就在底边中心
        // 所以图片的左下角坐标是 (-宽/2, 0)
        Vec2 texOrigin = Vec2(0, 0);
        Vec2 texDest = texOrigin + texSize;

        // 画一个蓝色的空心框框住图片
        _debugNode->drawRect(texOrigin, texDest, Color4F::BLUE);

        // =======================================================
        // 3. 画锚点 (Red - 红色圆点)
        // =======================================================
        // 永远在 (0,0)，代表脚底逻辑点
        _debugNode->drawDot(Vec2::ZERO, 3.0f, Color4F::RED);
 
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