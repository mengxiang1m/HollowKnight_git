#include "PlayerStates.h"
#include "Player.h"  
#include "config.h" 
#include "HelloWorldScene.h"
#include "SimpleAudioEngine.h" // 音频引擎

USING_NS_CC;
using namespace CocosDenshion; // 使用音频命名空间

// 全局变量：用于记录凝聚(Focus)和跑步的循环音效ID，方便停止
static unsigned int g_focusSoundID = 0;
static unsigned int g_runSoundID = 0;

void playRandomSlashSound() {
    int r = cocos2d::random(0, 2);
    const char* soundPath = Config::Audio::SWORD_3; // 默认

    if (r == 0) soundPath = Config::Audio::SWORD_1;
    else if (r == 1) soundPath = Config::Audio::SWORD_2;

    SimpleAudioEngine::getInstance()->playEffect(soundPath);
}

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
    // 1. 检测下落
    if (!player->isOnGround()) {
        player->changeState(new StateFall());
        return;
    }

    // 2. 检测攻击输入
    if (player->isAttackPressed() && player->isAttackReady()) {
        player->startAttackCooldown();//启动攻击冷却
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

    // 3. 检测跳跃输入
    if (player->isJumpPressed() && player->isJumpReady()) {
        player->consumeJumpInput();
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

    if (player->isCastPressed() && player->canCastSpell() && player->isCastReady())
    {
        // 消耗这次按键机会 (防止按住不放连发)
        player->consumeCastInput();

        player->changeState(new StateCast());
        return;
    }
}

void StateIdle::exit(Player* player)
{
}


// ============================================================================
// StateRun (跑步状态)
// ============================================================================

void StateRun::enter(Player* player)
{
    player->playAnimation("run");

    g_runSoundID = SimpleAudioEngine::getInstance()->playEffect(Config::Audio::HERO_RUN, true);
}

void StateRun::update(Player* player, float dt)
{
    int dir = player->getInputX();

    if (dir == 0) {
        player->changeState(new StateIdle());
        return;
    }

    if (player->isJumpPressed() && player->isJumpReady()) {
        player->consumeJumpInput();

        player->changeState(new StateJump());
        return;
    }

    if (player->isAttackPressed() && player->isAttackReady())
    {
        player->startAttackCooldown();//启动攻击冷却

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

    if (!player->isOnGround()) {
        player->changeState(new StateFall());
        return;
    }

    if (player->isFocusInputPressed() && player->canFocus() && dir == 0)
    {
        player->changeState(new StateFocus());
        return;
    }

    if (player->isCastPressed() && player->canCastSpell() && player->isCastReady())
    {
        // 消耗这次按键机会 (防止按住不放连发)
        player->consumeCastInput();

        player->changeState(new StateCast());
        return;
    }
    player->moveInDirection(dir);
}

void StateRun::exit(Player* player)
{
    if (g_runSoundID != 0) {
        SimpleAudioEngine::getInstance()->stopEffect(g_runSoundID);
        g_runSoundID = 0; // 重置 ID
    }
}


// ============================================================================
// StateJump (上升状态)
// ============================================================================

void StateJump::enter(Player* player)
{
    player->playAnimation("jump");
    player->startJump();

    // 播放跳跃音效
    SimpleAudioEngine::getInstance()->playEffect(Config::Audio::HERO_JUMP);
}

void StateJump::update(Player* player, float dt)
{
    int dir = player->getInputX();
    if (dir != 0) player->moveInDirection(dir);
    else player->setVelocityX(0);

    if (player->isAttackPressed() && player->isAttackReady()) {
        player->startAttackCooldown();//启动攻击冷却

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

    if (!player->isJumpPressed()) {
        player->stopJump();
    }

    if (player->getVelocityY() <= 0) {
        player->changeState(new StateFall());
        return;
    }


    if (player->isCastPressed() && player->canCastSpell() && player->isCastReady())
    {
        // 消耗这次按键机会 (防止按住不放连发)
        player->consumeCastInput();

        player->changeState(new StateCast());
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
    int dir = player->getInputX();
    if (dir != 0) {
        player->moveInDirection(dir);
    }
    else {
        player->setVelocityX(0);
    }

    if (player->isAttackPressed() && player->isAttackReady()) {
        player->startAttackCooldown();//启动攻击冷却

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


    if (player->isCastPressed() && player->canCastSpell() && player->isCastReady())
    {
        // 消耗这次按键机会 (防止按住不放连发)
        player->consumeCastInput();

        player->changeState(new StateCast());
        return;
    }

    // 落地检测
    if (player->isOnGround()) {
        // 【修改】播放落地音效 (软落地)
        SimpleAudioEngine::getInstance()->playEffect(Config::Audio::HERO_LAND_SOFT);

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
}


// ============================================================================
// StateSlash (攻击状态 - 通用函数)
// ============================================================================
void StateSlash::enter(Player* player)
{
    player->setAttackDir(0);
    player->playAnimation("slash");
    player->attack();

    //  播放随机攻击音效
    playRandomSlashSound();

    _timer = 0.0f;
    _duration = 6 * Config::Player::ATTACK_COOLDOWN;
}

void StateSlash::update(Player* player, float dt)
{
    _timer += dt;
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
}

// ============================================================================
// StateLookUp 
// ============================================================================
void StateLookUp::enter(Player* player)
{
    if (player->isOnGround()) {
        player->setVelocityX(0);
    }
    player->playAnimation("lookup");
    _timer = 0.0f;
    _duration = 0.6f;
}

void StateLookUp::update(Player* player, float dt)
{
    _timer += dt;
    if (player->isAttackPressed() && player->isAttackReady()) {
        player->startAttackCooldown();//启动攻击冷却

        player->changeState(new StateSlashUp());
        return;
    }
    if (_timer >= _duration) {
        if (player->isOnGround()) player->changeState(new StateIdle());
        else player->changeState(new StateFall());
    }
}

void StateLookUp::exit(Player* player) {}

// ============================================================================
// StateLookDown
// ============================================================================
void StateLookDown::enter(Player* player)
{
    if (player->isOnGround()) {
        player->setVelocityX(0);
    }
    player->playAnimation("lookdown");
    _timer = 0.0f;
    _duration = 0.6f;
}

void StateLookDown::update(Player* player, float dt)
{
    _timer += dt;
    if (_timer >= _duration) {
        if (player->isOnGround()) player->changeState(new StateIdle());
        else player->changeState(new StateFall());
    }
}

void StateLookDown::exit(Player* player) {}

// ============================================================================
// StateSlashUp
// ============================================================================
void StateSlashUp::enter(Player* player) {
    player->setAttackDir(1);
    player->playAnimation("slash_up");
    player->attack();

    //  播放随机攻击音效
    playRandomSlashSound();

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
    player->setAttackDir(-1);
    player->playAnimation("slash_down");
    player->attack();

    // 播放随机攻击音效
    playRandomSlashSound();

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
    player->setVelocityX(0);
    _timer = 0.0f;
    _hasHealed = false;
    _isEnding = false;

    player->playAnimation("focus_loop");
    player->startFocusEffect();

    //  播放蓄力音效 (Config)，并记录ID以便停止
    g_focusSoundID = SimpleAudioEngine::getInstance()->playEffect(Config::Audio::FOCUS_CHARGE, true);
}

void StateFocus::update(Player* player, float dt)
{
    if (_isEnding) return;

    const float TIME_CHARGE = 7 * 0.08f;
    const float TIME_GET = 9 * 0.06f;
    const float TIME_END = 3 * 0.08f;

    auto triggerEnd = [this, player, TIME_END]() {
        this->_isEnding = true;

        // 【新增】被打断时，停止蓄力音效
        if (g_focusSoundID != 0) {
            SimpleAudioEngine::getInstance()->stopEffect(g_focusSoundID);
            g_focusSoundID = 0;
        }

        player->playAnimation("focus_end");
        player->runAction(Sequence::create(
            DelayTime::create(TIME_END),
            CallFunc::create([player]() { player->changeState(new StateIdle()); }),
            nullptr));
        };

    //判断能否凝聚
    if (!_hasHealed)
    {
        if (!player->isFocusInputPressed() || !player->canFocus())
        {
            triggerEnd();
            return;
        }
    }

    if (!_hasHealed)
    {
        _timer += dt;
        if (_timer >= TIME_CHARGE)
        {
            player->executeHeal();
            _hasHealed = true;
            _isEnding = true;

            // 蓄力完成，停止蓄力声，播放回血声
            if (g_focusSoundID != 0) {
                SimpleAudioEngine::getInstance()->stopEffect(g_focusSoundID);
                g_focusSoundID = 0;
            }
            SimpleAudioEngine::getInstance()->playEffect(Config::Audio::FOCUS_HEAL);

            player->playFocusEndEffect();
            player->playAnimation("focus_get");

            auto seq = Sequence::create(
                DelayTime::create(TIME_GET),
                CallFunc::create([player]() {
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
    if (!_hasHealed) {
        player->stopFocusEffect();
    }

    //  双重保险：确保退出状态时音效一定停止
    if (g_focusSoundID != 0) {
        SimpleAudioEngine::getInstance()->stopEffect(g_focusSoundID);
        g_focusSoundID = 0;
    }
}

// ============================================================================
// StateDamaged (受伤/受击状态)
// ============================================================================
void StateDamaged::enter(Player* player)
{
    player->playAnimation("damage");
    _timer = 0.0f;
    _duration = 8 * 0.06f;

    // 受伤音效
    SimpleAudioEngine::getInstance()->playEffect(Config::Audio::HERO_DAMAGE);
}

void StateDamaged::update(Player* player, float dt)
{
    _timer += dt;
    if (_timer < _duration && _timer > 0.1f) {
        player->setVelocityY(0);
    }

    // 增加阻力逻辑
    float vx = player->getVelocityX();
    if (vx != 0) {
        float friction = 1200.0f * dt;
        if (vx > 0) { vx -= friction; if (vx < 0) vx = 0; }
        else if (vx < 0) { vx += friction; if (vx > 0) vx = 0; }
        player->setVelocityX(vx);
    }

    if (_timer >= _duration)
    {
        player->setVelocityX(0);

        if (player->getStats()->isDead())
        {
            player->changeState(new StateDead());
        }
        else
        {
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

    // 死亡音效
    SimpleAudioEngine::getInstance()->playEffect(Config::Audio::HERO_DEATH);

    player->playAnimation("death");

    auto restartSeq = Sequence::create(
        DelayTime::create(3.0f),
        CallFunc::create([]() {
            Director::getInstance()->replaceScene(TransitionFade::create(1.0f, HelloWorld::createScene()));
            }),
        nullptr
    );

    // 使用 Scene 运行动作更安全
    auto runningScene = Director::getInstance()->getRunningScene();
    if (runningScene) runningScene->runAction(restartSeq);
}

void StateDead::update(Player* player, float dt)
{
}

void StateDead::exit(Player* player)
{
}

// ============================================================
// StateCast (施法状态)
// ============================================================
void StateCast::enter(Player* player)
{
    // 1. 施法硬直：完全悬停
    player->setVelocityX(0);
    player->setVelocityY(0);

    // 2. 【阶段一】播放前摇动画 (3帧)
    player->playAnimation("cast_antic");

    // 3. 初始化
    _timer = 0.0f;
    _hasSpawned = false; // 还没发射
}

void StateCast::update(Player* player, float dt)
{
    _timer += dt;

    // 施法全程保持滞空
    player->setVelocityY(0);

    // ==========================================================
    // 时间参数配置
    // ==========================================================
    // 1. 前摇时间 (Antic Duration)
    const float TIME_ANTIC = 3 * 0.05f;
    // 2. 后摇时间 (Release Duration)
    const float TIME_RELEASE = 6 * 0.06f;
    // 总时长
    const float TIME_TOTAL = TIME_ANTIC + TIME_RELEASE;

    // ==========================================================
    // 状态切换逻辑
    // ==========================================================

    // 【阶段转换点】前摇结束，准备发射
    if (!_hasSpawned && _timer >= TIME_ANTIC)
    {
        // 1. 真正发射火球 (扣蓝、生成对象)
        player->executeSpell();

        // 2. 播放发射音效 (配合发射动作)
        SimpleAudioEngine::getInstance()->playEffect(Config::Audio::HERO_CAST);

        // 3. 【关键】切换到后摇动画
        player->playAnimation("cast_release");

        // 4. 给一个反冲力 (向后的瞬间速度)
        // 注意：因为我们在上面每帧都由 setVelocityX(0)，这个反冲力只会在这一帧生效
        // 如果想要持续滑行，需要加一个专门的变量控制，但在空中悬停时，瞬间位移效果更好
        float recoilDir = player->isFacingRight() ? -1.0f : 1.0f;
        player->setPositionX(player->getPositionX() + recoilDir * 10.0f); // 直接修改一点位置模拟后坐力震动

        // 标记已发射
        _hasSpawned = true;
    }

    // ==========================================================
    // 退出条件
    // ==========================================================
    if (_timer >= TIME_TOTAL)
    {
        if (player->isOnGround()) {
            player->changeState(new StateIdle());
        }
        else {
            player->changeState(new StateFall());
        }
    }
}

void StateCast::exit(Player* player)
{
    // 恢复重力接管 (其实切到 Idle/Fall 自然就恢复了)
}