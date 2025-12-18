#include "HelloWorldScene.h"
#include "SimpleAudioEngine.h"
#include "Enemy.h"
#include "Zombie.h"
#include "Spike.h"
#include "Buzzer.h"
#include "HUDLayer.h"
#include "Jar.h"
#include "Fireball.h" 
#include "PlayerAnimator.h"
#include "config.h"

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

    PlayerAnimator::preloadSounds();

    // 播放背景音乐 (使用 Config)
    auto audio = CocosDenshion::SimpleAudioEngine::getInstance();
    audio->playBackgroundMusic(Config::Audio::BGM_DIRTMOUTH, true);
    audio->setBackgroundMusicVolume(0.5f);
    audio->setEffectsVolume(0.8f);

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
    // 3. 使用loadMap方法加载level1
    //////////////////////////////////////////////////////////////////////
    loadMap("maps/level1.tmx");

    //////////////////////////////////////////////////////////////////////
    // 4. 在 init 中创建 Level 1 特有的敌人
    //////////////////////////////////////////////////////////////////////

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

    //////////////////////////////////////////////////////////////////////
    // 5. 创建主角 (Player)
    //////////////////////////////////////////////////////////////////////
    _player = Player::create("Knight/idle/idle_1.png");

    if (_player)
    {
        _player->setPosition(Vec2(450, 1300));  
        _gameLayer->addChild(_player, 10);
        CCLOG("Player created successfully!");
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
    // 7. 键盘监听器
    //////////////////////////////////////////////////////////////////////
    auto listener = EventListenerKeyboard::create();

    // --- 按下按键 ---
    listener->onKeyPressed = [=](EventKeyboard::KeyCode code, Event* event) {
        if (_player == nullptr) return;

        switch (code)
        {
        case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
            _isRightPressed = true;
            updatePlayerMovement();
            break;

        case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
            _isLeftPressed = true;
            updatePlayerMovement();
            break;

        case EventKeyboard::KeyCode::KEY_UP_ARROW:
            _isUpPressed = true;
            updatePlayerMovement();
            break;

        case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
            _isDownPressed = true;
            updatePlayerMovement();
            break;

        case EventKeyboard::KeyCode::KEY_Z:
            _player->setJumpPressed(true);
            break;

        case EventKeyboard::KeyCode::KEY_X:
            _player->setAttackPressed(true);
            break;

        case EventKeyboard::KeyCode::KEY_A:
            _player->setFocusInput(true);
            break;
        }
        };

    // --- 松开按键 ---
    listener->onKeyReleased = [=](EventKeyboard::KeyCode code, Event* event) {
        if (_player == nullptr) return;

        switch (code)
        {
        case EventKeyboard::KeyCode::KEY_A:
            // 凝聚释放 
            _player->setFocusInput(false);

            break;

        case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
            _isLeftPressed = false;
            updatePlayerMovement();
            break;

        case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
            _isRightPressed = false;
            updatePlayerMovement();
            break;
        case EventKeyboard::KeyCode::KEY_UP_ARROW:
            _isUpPressed = false;
            updatePlayerMovement();
            break;
        case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
            _isDownPressed = false;
            updatePlayerMovement();
            break;

        case EventKeyboard::KeyCode::KEY_Z:
            _player->setJumpPressed(false);
            break;

        case EventKeyboard::KeyCode::KEY_X:
            _player->setAttackPressed(false);
            break;
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
    // 0. 检测玩家位置，触发场景切换 (Level 1 -> 2)
    // ========================================
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

    // ========================================
    // 1. 更新玩家位置 (包含 Jar 平台逻辑)
    // ========================================
    std::vector<Rect> dynamicGroundRects = _groundRects;  // 复制原始地面碰撞

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
                    enemy->takeDamage(1,  _player->getPosition());  // 传入位置用于击退
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
                    zombie->takeDamage(1,  _player->getPosition());
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
                    buzzer1->takeDamage(1,  _player->getPosition());
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
                        _player->takeDamage(1, buzzer1->getPosition(),  _groundRects);
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
   // 使用迭代器遍历，安全删除无效的罐子指针
    for (auto it = _jars.begin(); it != _jars.end(); )
    {
        auto jar = *it;

        // 1. 检查指针有效性
        if (!jar || jar->getReferenceCount() == 0)
        {
            it = _jars.erase(it); // 如果对象已失效，从列表中移除
            continue;
        }

        if (!jar || jar->isDestroyed())
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

void HelloWorld::menuCloseCallback(Ref* pSender)
{
    Director::getInstance()->end();
}

// ========================================
// 加载地图的通用方法
// ========================================
void HelloWorld::loadMap(const std::string& mapPath)
{
    // 1. 清除旧地图
    auto oldMap = _gameLayer->getChildByTag(123);
    if (oldMap) oldMap->removeFromParent();

    // 2. 清除旧的碰撞框绘制节点
    auto oldDrawNode = dynamic_cast<DrawNode*>(_gameLayer->getChildByTag(1000));
    if (oldDrawNode) oldDrawNode->removeFromParent();

    // 3. 清除旧的敌人（切换到 Level 2 时清理 Level 1 的）
    if (_currentLevel == 2)
    {
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

        auto oldFireball = _gameLayer->getChildByTag(987);
        if (oldFireball) oldFireball->removeFromParent();
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

    // 6. 绘制调试碰撞框
    auto drawNode = DrawNode::create();
    drawNode->setTag(1000);
    _gameLayer->addChild(drawNode, 999);

    for (const auto& rect : _groundRects)
    {
        drawNode->drawRect(rect.origin, rect.origin + rect.size, Color4F::RED);
    }

    CCLOG("========== Map Loaded: %s ==========", mapPath.c_str());

    // 【新增】如果是 level2，创建罐子和幼虫
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