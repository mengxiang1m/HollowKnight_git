#include "PlayerStates.h"
#include "Player.h"  // 必须引入，否则无法调用 player 的方法
#include "config.h" // 假设你把数值都放在了这里
#include "HelloWorldScene.h"

USING_NS_CC;

// ============================================================================
// StateIdle (待机状态)
// ============================================================================

void StateIdle::enter(Player* player)
{
    // 播放待机动画
    player->playAnimation("idle");

    // 待机时水平速度归零 (避免滑步)
    player->setVelocityX(0);
}

void StateIdle::update(Player* player, float dt)
{
    // 1. 检测下落 (比如脚下的平台移动了/消失了)
    if (!player->isOnGround()) {
        player->changeState(new StateFall());
        return;
    }

    // 2. 检测攻击输入
    if (player->isAttackPressed()) { 
        if (player->getInputY() == 1) {
            player->changeState(new StateSlashUp());
            return;
        }
        // 2. 按住下 (S键) 且 在空中 -> 下劈
        else if (player->getInputY() == -1 && !player->isOnGround()) {
            player->changeState(new StateSlashDown());
            return;
        }
        // 3. 默认 -> 水平劈
        else {
            player->changeState(new StateSlash());
            return;
        }
    }

    // 3. 检测跳跃输入
    if (player->isJumpPressed()) {
        player->changeState(new StateJump());
        return;
    }

    // 4. 检测移动输入
    if (player->getInputX() != 0) {
        player->changeState(new StateRun());
        return;
    }

    if (player->getInputY() == 1) {
        player->changeState(new StateLookUp());
        return;
    }

    if (player->getInputY() == -1) {
        player->changeState(new StateLookDown());
        return;
    }
    // 5. 检测进入凝聚
    if (player->isFocusInputPressed() && player->canFocus())
    {
        player->changeState(new StateFocus());
        return;
    }
}

void StateIdle::exit(Player* player)
{
    // 可以在这里清理一些东西，目前为空
}


// ============================================================================
// StateRun (跑步状态)
// ============================================================================

void StateRun::enter(Player* player)
{
    player->playAnimation("run");
}

void StateRun::update(Player* player, float dt)
{
    // 1. 获取输入方向 (-1, 0, 1)
    int dir = player->getInputX();

    // 2. 状态切换：没有输入 -> 待机
    if (dir == 0) {
        player->changeState(new StateIdle());
        return;
    }

    // 3. 状态切换：跳跃
    if (player->isJumpPressed()) {
        player->changeState(new StateJump());
        return;
    }

    // 4. 状态切换：攻击
    if (player->isAttackPressed())
    {
        if (player->getInputY() == 1) {
            player->changeState(new StateSlashUp());
            return;
        }
        else if (player->getInputY() == -1 && !player->isOnGround()) {
            player->changeState(new StateSlashDown());
            return;
        }
        else {
            player->changeState(new StateSlash());
            return;
        }
    }

    // 5. 状态切换：下落 (跑出平台边缘)
    if (!player->isOnGround()) {
        player->changeState(new StateFall());
        return;
    }

	// 6. 状态切换：凝聚
    if (player->isFocusInputPressed() && player->canFocus() && dir == 0)
    {
        player->changeState(new StateFocus());
        return;
    }

    // 7. 执行物理移动 (这是 Run 状态的核心职责)
    player->moveInDirection(dir);
}

void StateRun::exit(Player* player)
{
}


// ============================================================================
// StateJump (上升状态)
// ============================================================================

void StateJump::enter(Player* player)
{
    player->playAnimation("jump");

    // 调用 Player 封装好的起跳物理逻辑 (施加向上的力)
    player->startJump();
}

void StateJump::update(Player* player, float dt)
{
    // 1. 空中移动控制 (Air Control)
    // 在空中允许玩家左右移动
    int dir = player->getInputX();
    if (dir != 0) player->moveInDirection(dir);
    else player->setVelocityX(0); // 空中松手停下，看你手感需求
    // 2. 攻击
    if (player->isAttackPressed()) {
        if (player->getInputY() == 1) {
            player->changeState(new StateSlashUp());
            return;
        }
        // 2. 按住下 (S键) 且 在空中 -> 下劈
        else if (player->getInputY() == -1 && !player->isOnGround()) {
            player->changeState(new StateSlashDown());
            return;
        }
        // 3. 默认 -> 水平劈
        else {
            player->changeState(new StateSlash());
            return;
        }
    }

    // 3. 变高跳逻辑 (Variable Jump Height)
    // 如果玩家中途松开跳跃键，调用 stopJump 截断上升力
    if (!player->isJumpPressed()) {
        player->stopJump();
    }

    // 4. 转为下落
    // 当垂直速度不再大于0 (开始下落)，或者碰头了
    if (player->getVelocityY() <= 0) {
        player->changeState(new StateFall());
        return;
    }
}

void StateJump::exit(Player* player)
{
}


// ============================================================================
// StateFall (下落状态)
// ============================================================================

void StateFall::enter(Player* player)
{
    player->playAnimation("fall");
}

void StateFall::update(Player* player, float dt)
{
    // 1. 空中移动控制
    int dir = player->getInputX();
    if (dir != 0) {
        player->moveInDirection(dir);
    }
    else {
        player->setVelocityX(0);
    }

    // 2. 攻击
    if (player->isAttackPressed()) {
        if (player->getInputY() == 1) {
            player->changeState(new StateSlashUp());
            return;
        }
        // 2. 按住下 (S键) 且 在空中 -> 下劈
        else if (player->getInputY() == -1 && !player->isOnGround()) {
            player->changeState(new StateSlashDown());
            return;
        }
        // 3. 默认 -> 水平劈
        else {
            player->changeState(new StateSlash());
            return;
        }
    }

    // 3. 落地检测
    if (player->isOnGround()) {
        // 落地瞬间，如果有按键则切跑，没按键切待机
        if (dir != 0) {
            player->changeState(new StateRun());
        }
        else {
            player->changeState(new StateIdle());
        }
        return;
    }
}

void StateFall::exit(Player* player)
{
    // 可以在这里播放落地音效或粒子特效
    // player->playEffect("land_dust.plist");
}


// ============================================================================
// StateSlash (攻击状态)
// ============================================================================

void StateSlash::enter(Player* player)
{
    player->setAttackDir(0); // 确保水平
    //  播放动画，并设置回调
    player->playAnimation("slash");
    // 因为涉及特效 Sprite 的显隐、同步等复杂逻辑。
    // 这里我们假设 Player 有一个方法能知道攻击什么时候结束。
    player->attack();

    _timer = 0.0f;
    // 攻击动画时长
   _duration = 6 * Config::Player::ATTACK_COOLDOWN;

}

void StateSlash::update(Player* player, float dt)
{
    // 计时
    _timer += dt;

    // 攻击状态下通常不允许左右移动，或者移动很慢
    // 这里我们锁定移动，不读取 inputX

    // 攻击结束
    if (_timer >= _duration) {
        if (player->isOnGround()) {
            player->changeState(new StateIdle());
        }
        else {
            player->changeState(new StateFall());
        }
    }
}

void StateSlash::exit(Player* player)
{
    // 确保攻击特效被关闭
    // player->hideSlashEffect();
}

 // ============================================================================
// StateLookUp 
// ============================================================================
void StateLookUp::enter(Player* player)
{
    if (player->isOnGround()) {
        player->setVelocityX(0);
    }

    // 播放动画
    player->playAnimation("lookup");

    _timer = 0.0f;
    _duration = 0.6f; 
}

void StateLookUp::update(Player* player, float dt)
{
    _timer += dt;

    if (player->isAttackPressed()) {
        // 既然在仰望，那肯定是上劈
        player->changeState(new StateSlashUp());
        return;
    }

    // 计时结束，恢复状态
    if (_timer >= _duration) {
        if (player->isOnGround()) {
            player->changeState(new StateIdle());
        }
        else {
            player->changeState(new StateFall());
        }
    }
}

void StateLookUp::exit(Player* player)
{
    // 开启无敌闪烁 (如果 Player 内部没做的话)
    // player->startInvincibility();
}

// ============================================================================
// StateLookDown
// ============================================================================
void StateLookDown::enter(Player* player)
{
    if (player->isOnGround()) {
        player->setVelocityX(0);
    }

    // 播放动画
    player->playAnimation("lookdown");

    _timer = 0.0f;
    _duration = 0.6f;
}

void StateLookDown::update(Player* player, float dt)
{
    _timer += dt;

    // 计时结束，恢复状态
    if (_timer >= _duration) {
        if (player->isOnGround()) {
            player->changeState(new StateIdle());
        }
        else {
            player->changeState(new StateFall());
        }
    }
}

void StateLookDown::exit(Player* player)
{
    // 开启无敌闪烁 (如果 Player 内部没做的话)
    // player->startInvincibility();
}

// ============================================================================
// StateSlashUp
// ============================================================================
void StateSlashUp::enter(Player* player) {
    player->setAttackDir(1); // 标记向上
    player->playAnimation("slash_up");
    player->attack(); // Player内部选择特效

    if (player->isOnGround()) player->setVelocityX(0);
    _timer = 0.0f; 
    _duration = 6 * Config::Player::ATTACK_COOLDOWN;
}

void StateSlashUp::update(Player* player, float dt) {
    _timer += dt;
    if (_timer >= _duration) {
        if (player->isOnGround()) player->changeState(new StateIdle());
        else player->changeState(new StateFall());
    }
}

void StateSlashUp::exit(Player* player) { player->setAttackDir(0); }

// ============================================================================
// StateSlashDown
// ============================================================================
void StateSlashDown::enter(Player* player) {
    player->setAttackDir(-1); // 标记向下
    player->playAnimation("slash_down");
    player->attack();
    _timer = 0.0f;
    _duration = 6 * Config::Player::ATTACK_COOLDOWN;
}

void StateSlashDown::update(Player* player, float dt) {
    _timer += dt;
    if (_timer >= _duration) {
        if (player->isOnGround()) player->changeState(new StateIdle());
        else player->changeState(new StateFall());
    }
}

void StateSlashDown::exit(Player* player) { player->setAttackDir(0); }

// ============================================================
//  StateFocus (凝聚)
// ============================================================
void StateFocus::enter(Player* player)
{
    CCLOG("State: Enter Focus");
    player->setVelocityX(0); // 锁死移动
    _timer = 0.0f;
    _hasHealed = false;
    _isEnding = false;

    // 播放第一阶段：蓄力循环 (Loop)
    player->playAnimation("focus_loop");

    player->startFocusEffect();
}

void StateFocus::update(Player* player, float dt)
{
    // 如果已经进入收尾阶段 (End)，等待动画播放完毕切回 Idle
    if (_isEnding)
    {
        // 检查当前动画是否播放完毕 (这里简化处理，通常需要回调或检测帧)
        // 工业级做法：绑定动画结束回调 (AnimationCallback)
        // 这里假设我们通过时间或状态判断，如果动画播完：
        // player->changeState(new StateIdle()); 
        return;
    }

    // 时间配置
    const float TIME_CHARGE = 7 * 0.08f;
    const float TIME_GET = 9 * 0.06f;
    const float TIME_END = 3 * 0.08f;

    // 中断检测
    auto triggerEnd = [this, player, TIME_END]() {
        this->_isEnding = true;
        player->playAnimation("focus_end");
        player->runAction(Sequence::create(
            DelayTime::create(TIME_END),
            CallFunc::create([player]() { player->changeState(new StateIdle()); }),
            nullptr));
        };

    // 1. 松手中断
    if (!_hasHealed)
    {
        if (!player->isFocusInputPressed())
        {
            triggerEnd();
            return;
        }
        // 2. 没魂中断
        if (!player->canFocus() )
        {
            triggerEnd();
            return;
        }
    }

    // 3. 蓄力中
    if (!_hasHealed)
    {
        _timer += dt;
        if (_timer >= TIME_CHARGE)
        {
            // --- 蓄力完成，回血 ---
            player->executeHeal();
            _hasHealed = true; // 标记本次回血完成
            _isEnding = true;  // 标记进入收尾，防止重复执行

            player->playFocusEndEffect();
            player->playAnimation("focus_get");

            auto seq = Sequence::create(
                DelayTime::create(TIME_GET),
                CallFunc::create([player]() {
                    // 动画播完，切回 Idle。
                    // 如果此时玩家还按着键，StateIdle update 第一帧就会再次切回 Focus
                    // 完美实现"连续回血"
                    player->changeState(new StateIdle());
                    }),
                nullptr
            );
            seq->setTag(103);
            player->runAction(seq);
        }
    }
}

void StateFocus::exit(Player* player)
{
    // 清理工作
    if (!_hasHealed) {
        player->stopFocusEffect();
    }
}

// ============================================================================
// StateDamaged (受伤/受击状态)
// ============================================================================
void StateDamaged::enter(Player* player)
{
    // 1. 播放受击(Recoil)动画
    player->playAnimation("damage");

    // 2. 设定硬直时间
    _timer = 0.0f;
    _duration = 8*0.06f;
}

void StateDamaged::update(Player* player, float dt)
{
    _timer += dt;
    if (_timer < _duration&&_timer>0.1f) {
        player->setVelocityY(0);
    }
    // 【物理处理】
    // 在 StateDamaged 期间，我们不响应左右移动键。
    // 但是 Player::update() 里的 updateMovementX/Y 依然在运行。
    // 所以 takeDamage 里设置的 _velocity 会让主角自动向后飞，并受重力影响下落。
    // 可以在这里加一点阻力让它停得更快，或者直接依赖物理引擎。

    // 【时间结束：进行审判】
    if (_timer >= _duration)
    {
        // 强制水平刹车 (防止滑步)
        player->setVelocityX(0);

        // 1. 检查是否死亡 (延迟判断)
        if (player->getStats()->isDead())
        {
            // 血量归零，受击动作演完了，该去死了
            player->changeState(new StateDead());
        }
        else
        {
            // 2. 没死，恢复正常
            if (player->isOnGround()) {
                player->changeState(new StateIdle());
            }
            else {
                player->changeState(new StateFall());
            }
        }
    }
}

void StateDamaged::exit(Player* player)
{
    // 退出时再次确保速度归零（双保险）
    player->setVelocityX(0);
}
// ============================================================
//  StateDead (死亡状态)
// ============================================================
void StateDead::enter(Player* player)
{
    CCLOG("State: Enter Dead");
    player->stopAllActions();
    player->setVelocityX(0);

    // 播放死亡动画
    player->playAnimation("death");

    // 3. 游戏结束/重启逻辑
     // 比如3秒后重启场景
    auto restartSeq = Sequence::create(
        DelayTime::create(3.0f),
        CallFunc::create([]() {
            Director::getInstance()->replaceScene(HelloWorld::createScene());
            }),
        nullptr
    );
    player->runAction(restartSeq);
}

void StateDead::update(Player* player, float dt)
{
    // 死亡状态下，只受重力影响，不响应任何输入
    // 可以在这里写 坠落逻辑
}

void StateDead::exit(Player* player)
{
    // 死人通常不会退出状态，除非复活
}
