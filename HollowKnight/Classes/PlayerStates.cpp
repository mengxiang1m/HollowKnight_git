#include "PlayerStates.h"
#include "Player.h"  // 必须引入，否则无法调用 player 的方法
#include "config.h" // 假设你把数值都放在了这里

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

    // 6. 执行物理移动 (这是 Run 状态的核心职责)
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
    // 在空中允许左右移动
    int dir = player->getInputX();
    if (dir != 0) {
        // 【修改】跳跃时按左右键，速度提升1.3倍
        player->moveInDirection(dir);
        player->setVelocityX(player->getVelocityX() * 1.3f);
    }
    else {
        player->setVelocityX(0); // 不按键停滞，不受惯性
    }
    
    // 2. 攻击
    if (player->isAttackPressed()) {
        if (player->getInputY() == 1) {
            player->changeState(new StateSlashUp());
            return;
        }
        // 2. 按住下 (S键) + 在空中 -> 下劈
        else if (player->getInputY() == -1 && !player->isOnGround()) {
            player->changeState(new StateSlashDown());
            return;
        }
        // 3. 默认 -> 水平斩
        else {
            player->changeState(new StateSlash());
            return;
        }
    }

    // 3. 跳跃逻辑 (Variable Jump Height)
    // 如果玩家松开跳跃按键，则调用 stopJump 截断上升
    if (!player->isJumpPressed()) {
        player->stopJump();
    }

    // 4. 转为下落
    // 当垂直速度不再大于0 (开始下降)，切换到落体
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
        // 【修改】下落时按左右键，速度也提升1.3倍
        player->moveInDirection(dir);
        player->setVelocityX(player->getVelocityX() * 1.3f);
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
        // 3. 默认 -> 水平斩
        else {
            player->changeState(new StateSlash());
            return;
        }
    }

    // 3. 着陆检测
    if (player->isOnGround()) {
        // 这里不跳转，而是按玩家输入决定
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
// StateDamaged (受伤状态)
// ============================================================================
void StateDamaged::enter(Player* player)
{
    // 播放受伤动画
    player->playAnimation("damage");

    // 可以在这里重置计时器，或者依赖 Player 的 scheduleOnce
    // 为了符合 State 模式，我们在 Update 里计时
    _timer = 0.0f;
    _duration = 0.4f; // 受伤硬直时间
}

void StateDamaged::update(Player* player, float dt)
{
    _timer += dt;

    // 受伤期间完全无法控制角色 (硬直)

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

void StateDamaged::exit(Player* player)
{
    // 开启无敌闪烁 (如果 Player 内部没做的话)
    // player->startInvincibility();
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