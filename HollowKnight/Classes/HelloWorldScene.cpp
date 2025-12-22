#include "HelloWorldScene.h"
#include "SimpleAudioEngine.h"
#include "Enemy.h"
#include "Zombie.h"
#include "Spike.h"
#include "Buzzer.h"
#include "HUDLayer.h"
#include "Jar.h"
#include "Fireball.h"
#include "KeyBindingScene.h"  // 【新增】键位管理器
#include "Boss.h"  // 【新增】Boss 类
#include "FKFireball.h" // 【新增**
#include "FKShockwave.h"

USING_NS_CC;

Scene* HelloWorld::createScene()
{
    return HelloWorld::create();
}

void HelloWorld::parseMapCollisions(TMXTiledMap* map)
{
    _groundRects.clear(); // 先清空

    // 1. 获取对象层
    auto objectGroup = map->getObjectGroup("collision");

    if (objectGroup == nullptr) {
        CCLOG("Error: 'collision' object group not found in TMX map!");
        return;
    }

    // 2. 获取层里所有的对象
    auto& objects = objectGroup->getObjects();

    for (auto& obj : objects)
    {
        // obj 是一个 ValueMap (键值对字典)
        cocos2d::ValueMap& dict = obj.asValueMap();

        // 3. 读取坐标和宽高
        float x = dict["x"].asFloat();
        float y = dict["y"].asFloat();
        float w = dict["width"].asFloat();
        float h = dict["height"].asFloat();

        // 4. 创建 Rect 并存起来
        Rect rect = Rect(x, y, w, h);
        _groundRects.push_back(rect);

        // CCLOG("Parsed Ground Rect: x=%f, y=%f, w=%f, h=%f", x, y, w, h);
    }
}

bool HelloWorld::init()
{
    if (!Scene::init())
    {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // ============================================================
    // 1. 创建游戏容器层
    // ============================================================
    _gameLayer = Layer::create();

    // 把缩放应用在 GameLayer 上，而不是整个 Scene
    // 这样 UI 就不会被放大，保持高清！
    float zoomScale = 1.5f;
    _gameLayer->setScale(zoomScale);

    // 游戏层 Z序低 (1)，放在下面
    this->addChild(_gameLayer, 1);

    //////////////////////////////////////////////////////////////////////
    // 2. 背景
    //////////////////////////////////////////////////////////////////////
    auto bgLayer = LayerColor::create(Color4B(40, 40, 40, 255)); // 稍微调暗一点，更有氛围
    _gameLayer->addChild(bgLayer, -100);

    //////////////////////////////////////////////////////////////////////
    // 3. 【测试配置】直接加载 Level 2 并将主角放在最右端
    //////////////////////////////////////////////////////////////////////
    _currentLevel = 2;  // 设置为 Level 2
    loadMap("maps/level2.tmx");  // 加载 Level 2 地图

    //////////////////////////////////////////////////////////////////////
    // 4. 在 init 中创建 Level 1 特有的敌人
    // 【注意】现在从 Level 2 开始测试，所以不需要创建 Level 1 的敌人
    //////////////////////////////////////////////////////////////////////

    /*
    // --- 创建 Enemy ---
    auto enemy = Enemy::create("enemies/enemy_walk_1.png");
    if (enemy)
    {
        enemy->setPosition(Vec2(600, 430));
        enemy->setPatrolRange(500, 800);
        enemy->setTag(999);
        _gameLayer->addChild(enemy, 5);

        // 死亡回调 (回魂 + 音效)
        enemy->setOnDeathCallback([=]() {
            if (_player) {
                _player->gainSoulOnKill();
                CCLOG("Soul gained from Enemy!");
            }
            // SimpleAudioEngine::getInstance()->playEffect("audio/enemy_death.wav");
            });

        CCLOG("Enemy spawned!");
    }

    // --- 创建 Zombie 敌人 ---
    auto zombie = Zombie::create("zombie/walk/walk_1.png");
    if (zombie)
    {
        zombie->setPosition(Vec2(1200, 430));
        zombie->setPatrolRange(1000, 1400);
        zombie->setTag(998);
        _gameLayer->addChild(zombie, 5);

        // 死亡回调
        zombie->setOnDeathCallback([=]() {
            if (_player) {
                // 兼容不同写法，这里假设 Player 有 gainSoulOnKill
                _player->gainSoulOnKill();
                CCLOG("Soul gained from Zombie!");
            }
            // SimpleAudioEngine::getInstance()->playEffect("audio/enemy_death.wav");
            });

        CCLOG("Zombie spawned at position (1200, 430)!");
    }

    // --- 创建 Spike 陷阱 ---
    auto textureCache = Director::getInstance()->getTextureCache();
    auto spikeTexture = textureCache->addImage("traps/spike.png");
    Spike* spike = nullptr;

    if (spikeTexture) {
        spike = Spike::create("traps/spike.png");
    }
    else {
        spike = Spike::create("enemies/enemy_walk_1.png"); // Fallback
    }

    if (spike)
    {
        Vec2 spikePos = Vec2(3590.0f, 1000.0f);
        spike->setInitialPosition(spikePos);
        spike->setTag(997);
        spike->setAnchorPoint(Vec2(0.5f, 0.5f));
        spike->setVisible(true);
        spike->setOpacity(255);
        spike->setScale(1.0f);
        spike->setBlendFunc(BlendFunc::ALPHA_PREMULTIPLIED);
        _gameLayer->addChild(spike, 5);
    }

    // --- 创建 Buzzer 飞行敌人 ---
    auto buzzer1 = Buzzer::create("buzzer/idle/idle_1.png");
    if (buzzer1)
    {
        Vec2 buzzer1Pos = Vec2(4000.0f, 900.0f);
        buzzer1->setInitialPosition(buzzer1Pos);
        buzzer1->setTag(996);
        _gameLayer->addChild(buzzer1, 5);

        buzzer1->setOnDeathCallback([=]() {
            if (_player) {
                _player->gainSoulOnKill(); // 调用主角加魂
                CCLOG("Soul gained from Buzzer 1!");
            }
            });
    }

    auto buzzer2 = Buzzer::create("buzzer/idle/idle_1.png");
    if (buzzer2)
    {
        Vec2 buzzer2Pos = Vec2(6000.0f, 700.0f);
        buzzer2->setInitialPosition(buzzer2Pos);
        buzzer2->setTag(995);
        _gameLayer->addChild(buzzer2, 5);

        buzzer2->setOnDeathCallback([=]() {
            if (_player) {
                _player->gainSoulOnKill(); // 调用主角加魂
                CCLOG("Soul gained from Buzzer 2!");
            }
            });
    }
    */

    //////////////////////////////////////////////////////////////////////
    // 5. 创建主角 (Player)
    //////////////////////////////////////////////////////////////////////
    _player = Player::create("Knight/idle/idle_1.png");
    
    // 【新增】初始化 Boss 变量
    _boss = nullptr;
    _bossTriggered = false;

    if (_player)
    {
        // 【测试配置】将主角放在 Level 2 最右侧，方便测试 Level 3 切换
        // Level 2 -> Level 3 切换触发点: X >= 6325
        // 主角初始位置: X = 6200 (距离切换点 125 像素)
        _player->setPosition(Vec2(6200, 1300));
        _gameLayer->addChild(_player, 10);
        CCLOG("========== TEST MODE ==========");
        CCLOG("Player spawned at Level 2 end: (6200, 1300)");
        CCLOG("Level 3 trigger point: X >= 6325");
        CCLOG("Distance to trigger: 125 pixels");
        CCLOG("==============================");
    }
    else
    {
        CCLOG("Error: Failed to create Player!");
    }

    //////////////////////////////////////////////////////////////////////
    // 6. 创建 UI 层
    //////////////////////////////////////////////////////////////////////
    auto hudLayer = HUDLayer::createLayer();
    hudLayer->setTag(900);
    this->addChild(hudLayer, 100);

    if (_player) {
        // 血量监听
        _player->setOnHealthChanged([=](int hp, int maxHp) {
            hudLayer->updateHealth(hp, maxHp);
            });

        // 魂量监听 
        _player->setOnSoulChanged([=](int soul) {
            hudLayer->updateSoul(soul);
            });

        // 初始化 UI
        hudLayer->updateHealth(_player->getHealth(), _player->getMaxHealth());
    }

    //////////////////////////////////////////////////////////////////////
    // 7. 键盘监听器 【修改】使用 KeyBindingManager
    //////////////////////////////////////////////////////////////////////
    auto listener = EventListenerKeyboard::create();
    // 【重要】不要在这里捕获 kbm，而是在每次回调中动态获取

    // --- 按下按键 ---
    listener->onKeyPressed = [=](EventKeyboard::KeyCode code, Event* event) {
        if (_player == nullptr) return;

        // 【修复】每次都重新获取 KeyBindingManager 实例，确保使用最新的键位配置
        auto kbm = KeyBindingManager::getInstance();

        // 使用键位管理器检查按键
        if (code == kbm->getKeyForAction(KeyBindingManager::Action::MOVE_RIGHT))
        {
            _isRightPressed = true;
            updatePlayerMovement();
        }
        else if (code == kbm->getKeyForAction(KeyBindingManager::Action::MOVE_LEFT))
        {
            _isLeftPressed = true;
            updatePlayerMovement();
        }
        else if (code == kbm->getKeyForAction(KeyBindingManager::Action::MOVE_UP))
        {
            _isUpPressed = true;
            updatePlayerMovement();
        }
        else if (code == kbm->getKeyForAction(KeyBindingManager::Action::MOVE_DOWN))
        {
            _isDownPressed = true;
            updatePlayerMovement();
        }
        else if (code == kbm->getKeyForAction(KeyBindingManager::Action::JUMP))
        {
            _player->setJumpPressed(true);
        }
        else if (code == kbm->getKeyForAction(KeyBindingManager::Action::ATTACK))
        {
            _player->setAttackPressed(true);
        }
        else if (code == kbm->getKeyForAction(KeyBindingManager::Action::FOCUS))
        {
            _player->setFocusInput(true);
        }
        };

    // --- 松开按键 ---
    listener->onKeyReleased = [=](EventKeyboard::KeyCode code, Event* event) {
        if (_player == nullptr) return;

        // 【修复】每次都重新获取 KeyBindingManager 实例
        auto kbm = KeyBindingManager::getInstance();

        // 使用键位管理器检查按键
        if (code == kbm->getKeyForAction(KeyBindingManager::Action::FOCUS))
        {
            _player->setFocusInput(false);
        }
        else if (code == kbm->getKeyForAction(KeyBindingManager::Action::MOVE_LEFT))
        {
            _isLeftPressed = false;
            updatePlayerMovement();
        }
        else if (code == kbm->getKeyForAction(KeyBindingManager::Action::MOVE_RIGHT))
        {
            _isRightPressed = false;
            updatePlayerMovement();
        }
        else if (code == kbm->getKeyForAction(KeyBindingManager::Action::MOVE_UP))
        {
            _isUpPressed = false;
            updatePlayerMovement();
        }
        else if (code == kbm->getKeyForAction(KeyBindingManager::Action::MOVE_DOWN))
        {
            _isDownPressed = false;
            updatePlayerMovement();
        }
        else if (code == kbm->getKeyForAction(KeyBindingManager::Action::JUMP))
        {
            _player->setJumpPressed(false);
        }
        else if (code == kbm->getKeyForAction(KeyBindingManager::Action::ATTACK))
        {
            _player->setAttackPressed(false);
        }
        };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    //////////////////////////////////////////////////////////////////////
    // 8. 退出按钮
    //////////////////////////////////////////////////////////////////////
    auto closeItem = MenuItemImage::create(
        "CloseNormal.png",
        "CloseSelected.png",
        CC_CALLBACK_1(HelloWorld::menuCloseCallback, this));

    if (closeItem)
    {
        float x = origin.x + visibleSize.width - closeItem->getContentSize().width / 2;
        float y = origin.y + closeItem->getContentSize().height / 2;
        closeItem->setPosition(Vec2(x, y));
    }

    auto menu = Menu::create(closeItem, NULL);
    menu->setPosition(Vec2::ZERO);
    this->addChild(menu, 100);

    //////////////////////////////////////////////////////////////////////
    // 9. 调试 UI
    //////////////////////////////////////////////////////////////////////
    _coordLabel = Label::createWithSystemFont("Player: (0, 0)", "Arial", 24);
    _coordLabel->setColor(Color3B::YELLOW);
    _coordLabel->setPosition(Vec2(visibleSize.width / 2, visibleSize.height - 50));
    _coordLabel->setAnchorPoint(Vec2(0.5f, 1.0f));
    this->addChild(_coordLabel, 200);

    _spikeDebugLabel = Label::createWithSystemFont("Spike: Loading...", "Arial", 20);
    _spikeDebugLabel->setColor(Color3B::RED);
    _spikeDebugLabel->setPosition(Vec2(visibleSize.width / 2, visibleSize.height - 100));
    _spikeDebugLabel->setAnchorPoint(Vec2(0.5f, 1.0f));
    this->addChild(_spikeDebugLabel, 200);

    _debugDrawNode = DrawNode::create();
    this->addChild(_debugDrawNode, 150);

    this->scheduleUpdate();

    return true;
}

void HelloWorld::updatePlayerMovement()
{
    if (!_player) return;

    // 左键(-1) + 右键(+1)
    int dirX = 0, dirY = 0;
    if (_isLeftPressed)  dirX -= 1;
    if (_isRightPressed) dirX += 1;

    if (_isUpPressed) dirY += 1;
    if (_isDownPressed) dirY -= 1;

    _player->setInputDirectionX(dirX);
    _player->setInputDirectionY(dirY);
}

// 每帧更新：实现相机跟随、碰撞检测、关卡切换
void HelloWorld::update(float dt)
{
    if (!_player || !_gameLayer) return;

    auto map = _gameLayer->getChildByTag(123);
    if (!map) return;

    // ========================================
    // 0. 检测玩家位置，触发场景切换
    // ========================================
    // Level 1 -> Level 2
    if (_currentLevel == 1 && !_isTransitioning)
    {
        Vec2 playerPos = _player->getPosition();
        if (playerPos.x >= 6500.0f)
        {
            CCLOG("Player reached level1 end! Triggering level switch...");
            switchToLevel2();
            return;
        }
    }

    // Level 2 -> Level 3
    if (_currentLevel == 2 && !_isTransitioning)
    {
        Vec2 playerPos = _player->getPosition();
        if (playerPos.x >= 6325.0f)
        {
            CCLOG("Player reached level2 end! Triggering level 3 switch...");
            switchToLevel3();
            return;
        }
    }

    // ========================================
    // 1. 更新玩家位置 (包含 Jar 平台逻辑)
    // ========================================
    std::vector<Rect> dynamicGroundRects = _groundRects;  // 复制原始地面碰撞

    // 【修复】添加安全检查：只在 Level 2 时处理罐子
    if (_currentLevel == 2 && !_jars.empty())
    {
        // 将所有未被摧毁的罐子顶部平台添加到碰撞检测中
        for (auto jar : _jars)
        {
            if (jar && !jar->isDestroyed())
            {
                Rect topPlatform = jar->getTopPlatformBox();
                if (!topPlatform.equals(Rect::ZERO))
                {
                    dynamicGroundRects.push_back(topPlatform);
                }
            }
        }
    }

    _player->update(dt, dynamicGroundRects);  // 使用包含罐子平台的碰撞列表

    // ========================================
    // 2. 获取玩家位置并更新坐标显示
    // ========================================
    Vec2 playerPos = _player->getPosition();

    if (_coordLabel)
    {
        char coordText[100];
        sprintf(coordText, "Player: (%.0f, %.0f)", playerPos.x, playerPos.y);
        _coordLabel->setString(coordText);
    }

    // ========================================
    // 3. 相机立即跟随玩家
    // ========================================
    Size visibleSize = Director::getInstance()->getVisibleSize();
    Size mapSize = map->getContentSize();
    float scaleValue = _gameLayer->getScale();

    float targetX = visibleSize.width * 0.5f - playerPos.x * scaleValue;
    float targetY = visibleSize.height * 0.5f - playerPos.y * scaleValue;

    float scaledMapWidth = mapSize.width * scaleValue;
    float scaledMapHeight = mapSize.height * scaleValue;

    float minX = -(scaledMapWidth - visibleSize.width);
    float maxX = 0.0f;
    float minY = -(scaledMapHeight - visibleSize.height);
    float maxY = 0.0f;

    if (scaledMapWidth > visibleSize.width) {
        targetX = std::max(minX, std::min(targetX, maxX));
    }
    else {
        targetX = (visibleSize.width - scaledMapWidth) * 0.5f;
    }

    if (scaledMapHeight > visibleSize.height) {
        targetY = std::max(minY, std::min(targetY, maxY));
    }
    else {
        targetY = (visibleSize.height - scaledMapHeight) * 0.5f;
    }

    _gameLayer->setPosition(targetX, targetY);

    // ========================================
    // 3. Enemy 碰撞检测逻辑
    // ========================================
    auto enemy = dynamic_cast<Enemy*>(_gameLayer->getChildByTag(999));
    if (enemy)
    {
        // 防止在此帧内因死亡被回收
        enemy->retain();
        Rect enemyBox = enemy->getHitbox();
        if (!enemyBox.equals(Rect::ZERO))
        {
            bool isEnemyHit = false;
            // A. 攻击检测
            if (_player->isAttackPressed())
            {
                Rect attackBox = _player->getAttackHitbox();
                if (attackBox.intersectsRect(enemyBox))
                {
                    CCLOG("HIT! Player hit the Enemy!");
                    enemy->takeDamage(1, _player->getPosition());  // 传入位置用于击退
                    isEnemyHit = true;
                    if (_player->getAttackDir() == -1) {
                        _player->pogoJump();
                    }
                }
            }
            // B. 碰撞检测
            if (!isEnemyHit) {
                Rect playerBox = _player->getCollisionBox();
                if (playerBox.intersectsRect(enemyBox))
                {
                    if (!_player->isInvincible())
                    {
                        CCLOG(" Player collided with Enemy!");
                        _player->takeDamage(1, enemy->getPosition(), _groundRects);
                        enemy->onCollideWithPlayer(_player->getPosition());
                    }
                }
            }
        }
        // 解锁
        enemy->release();
    }

    // ========================================
    // 4. Zombie 碰撞检测
    // ========================================
    auto zombie = dynamic_cast<Zombie*>(_gameLayer->getChildByTag(998));
    if (zombie)
    {
        zombie->retain();

        zombie->update(dt, playerPos, _groundRects);
        Rect zombieBox = zombie->getHitbox();

        if (!zombieBox.equals(Rect::ZERO))
        {
            bool isZombieHit = false;
            if (_player->isAttackPressed())
            {
                Rect attackBox = _player->getAttackHitbox();
                if (attackBox.intersectsRect(zombieBox))
                {
                    CCLOG("HIT! Player hit the Zombie!");
                    zombie->takeDamage(1, _player->getPosition());
                    isZombieHit = true;
                    if (_player->getAttackDir() == -1) {
                        _player->pogoJump();
                    }
                }
            }

            if (!isZombieHit)
            {
                Rect playerBox = _player->getCollisionBox();
                if (playerBox.intersectsRect(zombieBox))
                {
                    if (!_player->isInvincible())
                    {
                        CCLOG("Player collided with Zombie body!");
                        _player->takeDamage(1, zombie->getPosition(), _groundRects);
                        zombie->onCollideWithPlayer(_player->getPosition());
                    }
                }
            }
        }
        zombie->release();
    }

    // ========================================
    // 5. Spike 陷阱检测
    // ========================================
    auto spike = dynamic_cast<Spike*>(_gameLayer->getChildByTag(997));
    if (spike)
    {
        spike->update(dt, playerPos, _groundRects);

        if (_spikeDebugLabel)
        {
            char debugText[200];
            int state = (int)spike->getHitbox().equals(Rect::ZERO) ? -1 : 0;
            sprintf(debugText, "Spike:(%.0f,%.0f) State:%d Visible:%d",
                spike->getPosition().x, spike->getPosition().y,
                state, spike->isVisible());
            _spikeDebugLabel->setString(debugText);
        }

        Rect spikeBox = spike->getHitbox();
        if (!spikeBox.equals(Rect::ZERO))
        {
            Rect playerBox = _player->getCollisionBox();
            if (playerBox.intersectsRect(spikeBox))
            {
                if (!_player->isInvincible())
                {
                    CCLOG("Player hit by Spike!");
                    _player->takeDamage(1, spike->getPosition(), _groundRects);
                }
            }
        }
    }
    else
    {
        if (_spikeDebugLabel) {
            _spikeDebugLabel->setString("Spike: NOT FOUND (tag 997)");
            _spikeDebugLabel->setColor(Color3B::RED);
        }
    }

    // ========================================
    // 6. Buzzer 飞行敌人检测
    // ========================================
    // Buzzer 1 (tag 996)
    auto buzzer1 = dynamic_cast<Buzzer*>(_gameLayer->getChildByTag(996));
    if (buzzer1)
    {
        buzzer1->retain();
        buzzer1->update(dt, playerPos);
        Rect buzzer1Box = buzzer1->getHitbox();
        if (!buzzer1Box.equals(Rect::ZERO))
        {
            bool isBuzzer1Hit = false;
            // 攻击
            if (_player->isAttackPressed())
            {
                Rect attackBox = _player->getAttackHitbox();
                if (attackBox.intersectsRect(buzzer1Box))
                {
                    CCLOG("HIT! Player hit Buzzer 1!");
                    buzzer1->takeDamage(1, _player->getPosition());
                    isBuzzer1Hit = true;
                    if (_player->getAttackDir() == -1) _player->pogoJump();
                }
            }
            // 碰撞
            if (!isBuzzer1Hit)
            {
                Rect playerBox = _player->getCollisionBox();
                if (playerBox.intersectsRect(buzzer1Box))
                {
                    if (!_player->isInvincible())
                    {
                        CCLOG("Player collided with Buzzer 1!");
                        _player->takeDamage(1, buzzer1->getPosition(), _groundRects);
                        buzzer1->onCollideWithPlayer(_player->getPosition());
                    }
                }
            }
        }
        buzzer1->release();
    }
    // Buzzer 2 (tag 995)
    auto buzzer2 = dynamic_cast<Buzzer*>(_gameLayer->getChildByTag(995));
    if (buzzer2)
    {
        buzzer2->retain();
        buzzer2->update(dt, playerPos);
        Rect buzzer2Box = buzzer2->getHitbox();
        if (!buzzer2Box.equals(Rect::ZERO))
        {
            bool isBuzzer2Hit = false;
            if (_player->isAttackPressed())
            {
                Rect attackBox = _player->getAttackHitbox();
                if (attackBox.intersectsRect(buzzer2Box))
                {
                    CCLOG("HIT! Player hit Buzzer 2!");
                    buzzer2->takeDamage(1, _player->getPosition());
                    isBuzzer2Hit = true;
                    if (_player->getAttackDir() == -1) _player->pogoJump();
                }
            }
            if (!isBuzzer2Hit)
            {
                Rect playerBox = _player->getCollisionBox();
                if (playerBox.intersectsRect(buzzer2Box))
                {
                    if (!_player->isInvincible())
                    {
                        CCLOG("Player collided with Buzzer 2!");
                        _player->takeDamage(1, buzzer2->getPosition(), _groundRects);
                        buzzer2->onCollideWithPlayer(_player->getPosition());
                    }
                }
            }
        }
        buzzer2->release();
    }

    // ========================================
    // 7. Jar 罐子碰撞检测
    // ========================================
    // 【修复】添加安全检查：只在 Level 2 且罐子列表不为空时处理
    if (_currentLevel == 2 && !_jars.empty())
    {
        // 使用迭代器遍历，安全删除无效的罐子指针
        for (auto it = _jars.begin(); it != _jars.end(); )
        {
            auto jar = *it;

            // 1. 检查指针有效性或是否已被销毁
            // 如果 jar 已经被销毁 (isDestroyed) 并且其引用计数降为1（只被 _jars 列表持有），
            // 那么它很快就会被引擎回收，我们应该将它从列表中移除。
            if (!jar || (jar->isDestroyed() && jar->getReferenceCount() <= 1))
            {
                it = _jars.erase(it); // 如果对象已失效或即将失效，从列表中移除
                CCLOG("Removed an invalid or destroyed jar from the list.");
                continue;
            }

            if (jar->isDestroyed())
            {
                it++;
                continue;
            }
            // A. 玩家攻击罐子
            if (_player->isAttackPressed())
            {
                Rect attackBox = _player->getAttackHitbox();
                Rect jarBox = jar->getCollisionBox();

                if (!jarBox.equals(Rect::ZERO) && attackBox.intersectsRect(jarBox))
                {
                    CCLOG("Player hit the jar!");
                    jar->takeDamage();
                    // 罐子也是可以下劈的
                    if (_player->getAttackDir() == -1) {
                        _player->pogoJump();
                    }
                }
            }

            // B. 玩家与罐子的物理碰撞（像墙一样阻挡）
            if (!jar->isDestroyed())
            {
                Rect playerBox = _player->getCollisionBox();
                Rect jarBox = jar->getCollisionBox();

                if (!jarBox.equals(Rect::ZERO) && playerBox.intersectsRect(jarBox))
                {
                    // 简单的推出处理
                    float overlapLeft = (playerBox.getMaxX() - jarBox.getMinX());
                    float overlapRight = (jarBox.getMaxX() - playerBox.getMinX());

                    if (overlapLeft < overlapRight) {
                        _player->setPositionX(jarBox.getMinX() - playerBox.size.width / 2);
                    }
                    else {
                        _player->setPositionX(jarBox.getMaxX() + playerBox.size.width / 2);
                    }
                    _player->setVelocityX(0);
                }
            }
            it++;
        }
    }
    
    // ========================================
    // 8. Boss 战斗逻辑 (Level 3)
    // ========================================
    if (_currentLevel == 3 && _boss)
    {
        // A. 触发 Boss (玩家到达 X >= 1000)
        if (!_bossTriggered && playerPos.x >= 1000.0f)
        {
            // Boss 自动开始，无需 trigger() 方法
            _bossTriggered = true;
            CCLOG("========== BOSS TRIGGERED ==========");
        }
        
        // B. 更新 Boss AI
        if (_bossTriggered)
        {
            _boss->updateBoss(dt, playerPos, _groundRects);
            
            // C. Boss 碰撞检测
            _boss->retain();
            
            // 身体碰撞
            Rect bossBodyBox = _boss->getBodyHitbox();
            if (!bossBodyBox.equals(Rect::ZERO))
            {
                bool isBossHit = false;
                
                // 玩家攻击 Boss
                if (_player->isAttackPressed())
                {
                    Rect attackBox = _player->getAttackHitbox();
                    if (attackBox.intersectsRect(bossBodyBox))
                    {
                        CCLOG("HIT! Player hit the Boss!");
                        _boss->takeDamage(1);  // 普通攻击
                        isBossHit = true;
                        
                        // 下劈反弹
                        if (_player->getAttackDir() == -1) {
                            _player->pogoJump();
                        }
                    }
                }
                
                // Boss 碰撞玩家（身体）
                if (!isBossHit)
                {
                    Rect playerBox = _player->getCollisionBox();
                    if (playerBox.intersectsRect(bossBodyBox))
                    {
                        if (!_player->isInvincible())
                        {
                            CCLOG("Player collided with Boss body!");
                            _player->takeDamage(1, _boss->getPosition(), _groundRects);
                        }
                    }
                }
            }
            
            // 锤子碰撞 (jumpAttack 时)
            Rect bossHammerBox = _boss->getHammerHitbox();
            if (!bossHammerBox.equals(Rect::ZERO))
            {
                Rect playerBox = _player->getCollisionBox();
                if (playerBox.intersectsRect(bossHammerBox))
                {
                    if (!_player->isInvincible())
                    {
                        CCLOG("Player hit by Boss hammer!");
                        _player->takeDamage(1, _boss->getPosition(), _groundRects);
                    }
                }
            }
            
            _boss->release();
        }
    }

    // ========================================
    // 9. FKFireball 碰撞检测 (Level 3)
    // ========================================
    if (_currentLevel == 3)
    {
        auto children = _gameLayer->getChildren();
        for (auto child : children)
        {
            auto fireball = dynamic_cast<FKFireball*>(child);
            if (fireball)
            {
                fireball->update(dt, _groundRects);
                Rect fireballBox = fireball->getCollisionBox();
                if (!fireballBox.equals(Rect::ZERO))
                {
                    Rect playerBox = _player->getCollisionBox();
                    if (playerBox.intersectsRect(fireballBox))
                    {
                        if (!_player->isInvincible())
                        {
                            CCLOG("Player hit by FKFireball!");
                            _player->takeDamage(1, fireball->getPosition(), _groundRects);
                            fireball->removeFromParent(); // 火球碰到玩家后消失
                        }
                    }
                }
            }
            else if (auto shockwave = dynamic_cast<FKShockwave*>(child))
            {
                shockwave->update(dt, _groundRects);
                Rect swBox = shockwave->getCollisionBox();
                if (!swBox.equals(Rect::ZERO))
                {
                    Rect playerBox = _player->getCollisionBox();
                    if (playerBox.intersectsRect(swBox))
                    {
                        if (!_player->isInvincible())
                        {
                            CCLOG("Player hit by FKShockwave!");
                            _player->takeDamage(1, shockwave->getPosition(), _groundRects);
                            shockwave->removeFromParent();
                        }
                    }
                }
            }
        }
    }
}


void HelloWorld::menuCloseCallback(Ref* pSender)
{
    Director::getInstance()->end();
}

// ========================================
// 加载地图的通用方法
// ========================================
void HelloWorld::loadMap(const std::string& mapPath)
{
    // 【修复建议】在移除旧地图之前，首先清理依赖于它的对象
    if (_currentLevel == 3)
    {
        // 清理 Level 2 的敌人和对象
        // 【修复】不再遍历 _jars 向量，因为它可能包含悬空指针。
        // 改为通过 tag 从 _gameLayer 中安全地查找并移除。
        for (int i = 0; i < 3; ++i)
        {
            auto jarNode = _gameLayer->getChildByTag(990 - i);
            if (jarNode)
            {
                jarNode->removeFromParent();
            }
        }

        // 【修复】立即清空向量，防止悬空指针问题
        _jars.clear();

        // 清除 Fireball
        auto oldFireball = _gameLayer->getChildByTag(987);
        if (oldFireball) oldFireball->removeFromParent();
    }

    // 1. 清除旧地图
    auto oldMap = _gameLayer->getChildByTag(123);
    if (oldMap) oldMap->removeFromParent();

    // 2. 清除旧的碰撞框绘制节点
    auto oldDrawNode = dynamic_cast<DrawNode*>(_gameLayer->getChildByTag(1000));
    if (oldDrawNode) oldDrawNode->removeFromParent();

    // 3. 清除旧的敌人
    if (_currentLevel == 2)
    {
        // 清理 Level 1 的敌人
        auto enemy = _gameLayer->getChildByTag(999);
        if (enemy) enemy->removeFromParent();

        auto zombie = _gameLayer->getChildByTag(998);
        if (zombie) zombie->removeFromParent();

        auto spike = _gameLayer->getChildByTag(997);
        if (spike) spike->removeFromParent();

        auto buzzer1 = _gameLayer->getChildByTag(996);
        if (buzzer1) buzzer1->removeFromParent();

        auto buzzer2 = _gameLayer->getChildByTag(995);
        if (buzzer2) buzzer2->removeFromParent();
    }
    
    // 【新增】清理旧的 Boss
    if (_boss)
    {
        _boss->removeFromParent();
        _boss = nullptr;
        _bossTriggered = false;
    }

    // 4. 加载新地图
    auto map = TMXTiledMap::create(mapPath);
    if (map == nullptr) {
        CCLOG("Error: Failed to load %s", mapPath.c_str());
        return;
    }

    map->setAnchorPoint(Vec2(0, 0));
    map->setPosition(Vec2(0, 0));
    map->setTag(123);
    _gameLayer->addChild(map, -99);

    // 5. 解析碰撞数据
    this->parseMapCollisions(map);

    // ============================================================
    // 【修改】Level 3 偏移量统一管理
    // 定义一个 mapOffset 变量，同时应用于背景图和碰撞框
    // ============================================================
    Vec2 mapOffset = Vec2::ZERO;
    if (_currentLevel == 3)
    {
        // 这里设置你想要的偏移量，例如 (0, 300)
        mapOffset = Vec2(300, 250);

        // 【关键】修正碰撞框位置，使其跟随偏移量移动
        for (auto& rect : _groundRects)
        {
            rect.origin += mapOffset;
        }
    }

    // 6. 绘制调试碰撞框 (现在会绘制修正后的位置)
    auto drawNode = DrawNode::create();
    drawNode->setTag(1000);
    _gameLayer->addChild(drawNode, 999);

    for (const auto& rect : _groundRects)
    {
        drawNode->drawRect(rect.origin, rect.origin + rect.size, Color4F::RED);
    }

    CCLOG("========== Map Loaded: %s ==========", mapPath.c_str());

    // 7. 根据关卡创建特定的敌人和对象
    if (_currentLevel == 2)
    {
        _jars.clear();

        float startX = 1700.0f;
        float spacing = 700.0f;
        float jarY = 346.0f;

        for (int i = 0; i < 3; i++)
        {
            float jarX = startX + i * spacing;
            auto jar = Jar::create("warm/jar.png", Vec2(jarX, jarY));

            if (jar)
            {
                jar->setTag(990 - i);
                _gameLayer->addChild(jar, 5);
                _jars.push_back(jar);
            }
        }

        // 创建 Fireball
        auto fireball = Fireball::create("fireball/fireball_1.png");
        if (fireball)
        {
            fireball->setPosition(Vec2(5529.0f, 650.0f));
            fireball->setTag(987);
            _gameLayer->addChild(fireball, 5);
        }
    }
    else if (_currentLevel == 3)
    {
        // ============================================================
        // Level 3 特殊处理：手动加载背景图
        // ============================================================
        auto bg = Sprite::create("maps/GameAsset/fight.png"); // 确保路径对应 Resources/maps/GameAsset/fight.png
        if (bg)
        {
            bg->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);

            // 【用户请求】使用上面定义的 mapOffset 来设置背景图位置
            // 这样背景图和碰撞框就同步移动了！
            bg->setPosition(mapOffset);

            // 添加到 map 节点的最底层 (-1)，这样它永远在 map 的其他元素（如碰撞）后面
            map->addChild(bg, -1);

            // 强制同步地图大小为背景图大小
            map->setContentSize(bg->getContentSize());

            CCLOG("Success: Level 3 background loaded manually.");
        }
        else
        {
            CCLOG("Error: Failed to load Level 3 background 'maps/GameAsset/fight.png'");
        }

        // ============================================================
        // 【新增】创建 Boss
        // ============================================================
        // 地板 Y 坐标是 856（从 level3.tmx collision 可以看到）
        // Boss 应该站在地板上，所以 Y = 856
        float groundY = 856.0f + mapOffset.y;
        _boss = Boss::create(Vec2(1650 + mapOffset.x, groundY + 200));  // +200 让它从空中掉落
        if (_boss)
        {
            _boss->setTag(980);  // Boss tag
            _gameLayer->addChild(_boss, 6);
            _bossTriggered = false;

            // 【新增】设置火球回调
            _boss->setFireballCallback([this, mapOffset](const Vec2& pos) {
                auto fireball = FKFireball::create("boss/rampageAttack/fk-fireball.png");
                if (fireball)
                {
                    // 【修复】将 Boss 局部随机坐标转换为世界坐标
                    fireball->setPosition(Vec2(pos.x, pos.y));
                    _gameLayer->addChild(fireball, 7); // 确保在 Boss 上面
                }
            });
            _boss->setShockwaveCallback([this](const Vec2& pos, float dir) {
                auto shockwave = FKShockwave::create("boss/shockwaveAttack/fk-shockwave.png", dir);
                if (shockwave)
                {
                    shockwave->setPosition(pos);
                    _gameLayer->addChild(shockwave, 7);
                }
            });
            
            CCLOG("Boss created at (%.0f, %.0f) for falling", 1650 + mapOffset.x, groundY + 200);
        }
        else
        {
            CCLOG("Error: Failed to create Boss!");
        }

        // 可以在这里添加 Level 3 的敌人
        CCLOG("Level 3 loaded - Ready for battle!");
    }
}

// ========================================
// 切换到Level2的方法
// ========================================
void HelloWorld::switchToLevel2()
{
    if (_isTransitioning || _currentLevel == 2) return;

    _isTransitioning = true;
    CCLOG("========== Switching to Level 2 ==========");

    auto blackLayer = LayerColor::create(Color4B::BLACK);
    blackLayer->setOpacity(0);
    this->addChild(blackLayer, 999);

    auto fadeIn = FadeTo::create(0.5f, 255);

    auto switchMap = CallFunc::create([this]() {
        _currentLevel = 2;
        loadMap("maps/level2.tmx");

        if (_player)
        {
            _player->setPosition(Vec2(400, 1300));
            _player->setVelocityX(0);
        }
        });

    auto delay = DelayTime::create(0.3f);
    auto fadeOut = FadeTo::create(0.5f, 0);

    auto cleanup = CallFunc::create([this, blackLayer]() {
        blackLayer->removeFromParent();
        _isTransitioning = false;
        CCLOG("========== Level 2 loaded successfully ==========");
        });

    blackLayer->runAction(Sequence::create(fadeIn, switchMap, delay, fadeOut, cleanup, nullptr));
}

// ========================================
// 切换到Level3的方法
// ========================================
void HelloWorld::switchToLevel3()
{
    if (_isTransitioning || _currentLevel == 3) return;

    _isTransitioning = true;
    CCLOG("========== Switching to Level 3 ==========");

    auto blackLayer = LayerColor::create(Color4B::BLACK);
    blackLayer->setOpacity(0);
    this->addChild(blackLayer, 999);

    auto fadeIn = FadeTo::create(0.5f, 255);

    auto switchMap = CallFunc::create([this]() {
        _currentLevel = 3;
        loadMap("maps/level3.tmx");

        if (_player)
        {
            _player->setPosition(Vec2(500, 1300));
            _player->setVelocityX(0);
        }
        });

    auto delay = DelayTime::create(0.3f);
    auto fadeOut = FadeTo::create(0.5f, 0);

    auto cleanup = CallFunc::create([this, blackLayer]() {
        blackLayer->removeFromParent();
        _isTransitioning = false;
        CCLOG("========== Level 3 loaded successfully ==========");
        });

    blackLayer->runAction(Sequence::create(fadeIn, switchMap, delay, fadeOut, cleanup, nullptr));
}