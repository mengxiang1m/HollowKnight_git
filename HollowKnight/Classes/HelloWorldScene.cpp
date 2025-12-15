#include "HelloWorldScene.h"
#include "SimpleAudioEngine.h"
#include "Enemy.h"
#include "Zombie.h"
#include "Spike.h"
#include "Buzzer.h"
#include "HUDLayer.h"
#include "Jar.h"
#include "Fireball.h"  // 【新增】

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

        CCLOG("Parsed Ground Rect: x=%f, y=%f, w=%f, h=%f", x, y, w, h);
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
    // 1. 【核心修改】创建游戏容器层
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
    // 3. 【重构】使用loadMap方法加载level1
    //////////////////////////////////////////////////////////////////////
    loadMap("maps/level1.tmx");

    //////////////////////////////////////////////////////////////////////
    // 4. 【重构】只在level1创建敌人
    //////////////////////////////////////////////////////////////////////
    // 创建 Enemy
    auto enemy = Enemy::create("enemies/enemy_walk_1.png");
    if (enemy)
    {
        enemy->setPosition(Vec2(600, 430));
        enemy->setPatrolRange(500, 800);
        enemy->setTag(999);
        _gameLayer->addChild(enemy, 5);
        CCLOG("Enemy spawned!");
    }

    // 创建 Zombie 敌人
    auto zombie = Zombie::create("zombie/walk/walk_1.png");
    if (zombie)
    {
        zombie->setPosition(Vec2(1200, 430));
        zombie->setPatrolRange(1000, 1400);
        zombie->setTag(998);
        _gameLayer->addChild(zombie, 5);
        CCLOG("Zombie spawned at position (1200, 430)!");
    }

    // 创建 Spike 陷阱
    auto textureCache = Director::getInstance()->getTextureCache();
    auto spikeTexture = textureCache->addImage("traps/spike.png");
    
    Spike* spike = nullptr;
    
    if (spikeTexture)
    {
        CCLOG("========== TEXTURE PRE-LOAD ==========");
        CCLOG("Texture loaded successfully!");
        CCLOG("Texture size: %.0fx%.0f", 
              spikeTexture->getContentSize().width,
              spikeTexture->getContentSize().height);
        CCLOG("Texture pixel format: %d", (int)spikeTexture->getPixelFormat());
        CCLOG("=====================================");
        
        spike = Spike::create("traps/spike.png");
    }
    else
    {
        CCLOG("========== TEXTURE LOAD FAILED ==========");
        CCLOG("Failed to load texture: traps/spike.png");
        CCLOG("Trying alternative: Using enemy texture as fallback");
        CCLOG("=========================================");
        
        spike = Spike::create("enemies/enemy_walk_1.png");
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
        spike->setColor(Color3B::WHITE);
        spike->setBlendFunc(BlendFunc::ALPHA_PREMULTIPLIED);
        
        _gameLayer->addChild(spike, 5);
        
        CCLOG("========== SPIKE DEBUG INFO ==========");
        CCLOG("Spike created successfully!");
        CCLOG("Spike position set to: (%.2f, %.2f)", spikePos.x, spikePos.y);
        CCLOG("=====================================");
    }

    // 创建 Buzzer 飞行敌人
    auto buzzer1 = Buzzer::create("buzzer/idle/idle_1.png");
    if (buzzer1)
    {
        Vec2 buzzer1Pos = Vec2(4000.0f, 900.0f);
        buzzer1->setInitialPosition(buzzer1Pos);
        buzzer1->setTag(996);
        _gameLayer->addChild(buzzer1, 5);
        CCLOG("Buzzer 1 spawned at position (4000, 900)!");
    }
    
    auto buzzer2 = Buzzer::create("buzzer/idle/idle_1.png");
    if (buzzer2)
    {
        Vec2 buzzer2Pos = Vec2(6000.0f, 700.0f);
        buzzer2->setInitialPosition(buzzer2Pos);
        buzzer2->setTag(995);
        _gameLayer->addChild(buzzer2, 5);
        CCLOG("Buzzer 2 spawned at position (6000, 700)!");
    }

    //////////////////////////////////////////////////////////////////////
    // 5. 创建主角 (Player)
    //////////////////////////////////////////////////////////////////////
    _player = Player::create("Knight/idle/idle_1.png");

    if (_player)
    {
        _player->setPosition(Vec2(6400, 1300));  // 【修改】玩家初始位置接近 level1 终点，方便测试
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

    // 连接 Player 和 HUD
    _player->setOnHealthChanged([=](int hp, int maxHp) {
        hudLayer->updateHealth(hp, maxHp);
    });

    hudLayer->updateHealth(_player->getHealth(), _player->getMaxHealth());

    //////////////////////////////////////////////////////////////////////
    // 7. 键盘监听器
    //////////////////////////////////////////////////////////////////////
    auto listener = EventListenerKeyboard::create();

    // --- 按下按键 ---
    listener->onKeyPressed = [=](EventKeyboard::KeyCode code, Event* event) {
        if (_player == nullptr) return;

        switch (code)
        {
        case EventKeyboard::KeyCode::KEY_D:
        case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
            _isRightPressed = true;
            updatePlayerMovement();
            break;

        case EventKeyboard::KeyCode::KEY_A:
        case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
            _isLeftPressed = true;
            updatePlayerMovement();
            break;

        case EventKeyboard::KeyCode::KEY_W:
        case EventKeyboard::KeyCode::KEY_UP_ARROW:
            _isUpPressed = true;
            updatePlayerMovement();
            break;

        case EventKeyboard::KeyCode::KEY_S:
        case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
            _isDownPressed = true;
            updatePlayerMovement();
            break;

        case EventKeyboard::KeyCode::KEY_Z:
        case EventKeyboard::KeyCode::KEY_SPACE:
            _player->setJumpPressed(true);
            break;

        case EventKeyboard::KeyCode::KEY_J:
        case EventKeyboard::KeyCode::KEY_X:
            _player->setAttackPressed(true);
            break;
        }
    };

    // --- 松开按键 ---
    listener->onKeyReleased = [=](EventKeyboard::KeyCode code, Event* event) {
        if (_player == nullptr) return;

        switch (code)
        {
        case EventKeyboard::KeyCode::KEY_A:
        case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
            _isLeftPressed = false;
            updatePlayerMovement();
            break;
        case EventKeyboard::KeyCode::KEY_D:
        case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
            _isRightPressed = false;
            updatePlayerMovement();
            break;
        case EventKeyboard::KeyCode::KEY_W:
        case EventKeyboard::KeyCode::KEY_UP_ARROW:
            _isUpPressed = false;
            updatePlayerMovement();
            break;
        case EventKeyboard::KeyCode::KEY_S:
        case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
            _isDownPressed = false;
            updatePlayerMovement();
            break;

        case EventKeyboard::KeyCode::KEY_Z:
        case EventKeyboard::KeyCode::KEY_SPACE:
            _player->setJumpPressed(false);
            break;

        case EventKeyboard::KeyCode::KEY_J:
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
    // 9. 创建坐标显示标签
    //////////////////////////////////////////////////////////////////////
    _coordLabel = Label::createWithSystemFont("Player: (0, 0)", "Arial", 24);
    _coordLabel->setColor(Color3B::YELLOW);
    _coordLabel->setPosition(Vec2(visibleSize.width / 2, visibleSize.height - 50));
    _coordLabel->setAnchorPoint(Vec2(0.5f, 1.0f));
    this->addChild(_coordLabel, 200);

    //////////////////////////////////////////////////////////////////////
    // 10. 创建Spike调试信息标签
    //////////////////////////////////////////////////////////////////////
    _spikeDebugLabel = Label::createWithSystemFont("Spike: Loading...", "Arial", 20);
    _spikeDebugLabel->setColor(Color3B::RED);
    _spikeDebugLabel->setPosition(Vec2(visibleSize.width / 2, visibleSize.height - 100));
    _spikeDebugLabel->setAnchorPoint(Vec2(0.5f, 1.0f));
    this->addChild(_spikeDebugLabel, 200);

    //////////////////////////////////////////////////////////////////////
    // 11. 创建调试DrawNode
    //////////////////////////////////////////////////////////////////////
    _debugDrawNode = DrawNode::create();
    this->addChild(_debugDrawNode, 150);

    // 开启 Update，用于相机跟随
    this->scheduleUpdate();

    return true;
}

void HelloWorld::updatePlayerMovement()
{
    if (!_player) return;

    // 1. 计算输入方向
    // 左键(-1) + 右键(+1)
    // 如果同时按住，结果为 0 (停)；只按左是 -1；只按右是 1
    int dirX = 0,dirY=0;
    if (_isLeftPressed)  dirX -= 1;
    if (_isRightPressed) dirX += 1;

    if (_isUpPressed) dirY+= 1;
    if (_isDownPressed) dirY -= 1;
 
    // 2. 将“意图”传给 Player
    // Player 内部的状态机 (StateRun/StateIdle) 会自动读取这个值
    // 如果是 0，状态机自动切回 Idle；如果是 -1/1，状态机自动切为 Run 并移动
    _player->setInputDirectionX(dirX);
    _player->setInputDirectionY(dirY);
}

// 每帧更新：实现相机跟随
void HelloWorld::update(float dt)
{
    if (!_player || !_gameLayer) return;

    auto map = _gameLayer->getChildByTag(123);
    if (!map) return;

    // ========================================
    // 0. 【新增】检测玩家位置，触发场景切换
    // ========================================
    if (_currentLevel == 1 && !_isTransitioning)
    {
        Vec2 playerPos = _player->getPosition();
        if (playerPos.x >= 6500.0f)
        {
            CCLOG("Player reached level1 end! Triggering level switch...");
            switchToLevel2();
            return;  // 立即返回，避免在切换过程中继续更新
        }
    }

    // ========================================
    // 1. 首先更新玩家位置
    // ========================================
    // 【新增】动态添加罐子顶部平台到地面碰撞检测
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

    // 【新增】更新坐标显示
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
    // 获取缩放比例 (现在是 _gameLayer 在缩放)
    float scaleValue = _gameLayer->getScale();

    // 相机偏移需要乘以缩放因子
  // 因为 Scene 被缩放了，setPosition 的单位也被缩放了
    float targetX = visibleSize.width * 0.5f - playerPos.x * scaleValue;
    float targetY = visibleSize.height * 0.5f - playerPos.y * scaleValue;

    // 【边界限制】确保相机不超出地图边界（同样需要考虑缩放）
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

    // ============================================================
    // 【修改】只移动游戏层
    // ============================================================
    _gameLayer->setPosition(targetX, targetY);

    // ========================================
		// 3. 【修复】修改 Enemy 碰撞检测逻辑
        // ========================================
        auto enemy = dynamic_cast<Enemy*>(_gameLayer->getChildByTag(999));
        if (enemy)
        {
            // 获取碰撞箱，如果为空则跳过碰撞检测
            Rect enemyBox = enemy->getHitbox();

            // 只有当敌人碰撞箱有效时才进行碰撞检测
            if (!enemyBox.equals(Rect::ZERO))
            {
				bool isEnemyHit = false;
				// 1.【修复】首先检测攻击是否命中敌人
                if (_player->isAttackPressed())
                {
                    Rect attackBox = _player->getAttackHitbox();

                    if (attackBox.intersectsRect(enemyBox))
                    {
                        CCLOG("HIT! Player hit the Enemy!");
                        enemy->takeDamage(1, _player->getPosition());  // 传入玩家位置
						isEnemyHit = true;  // 标记敌人被击中

						// 如果敌人被击中，玩家执行 pogo 跳跃
                        if (_player->getAttackDir() == -1) {
                            _player->pogoJump();
                        }
                    }
                }
				// 2.【修复】检测敌人是否与玩家碰撞
                if (!isEnemyHit) {  // 只有当敌人未被攻击命中时才检测碰撞
                    Rect playerBox = _player->getCollisionBox();

                    if (playerBox.intersectsRect(enemyBox))
                    {
                        if (!_player->isInvincible())
                        {
                            CCLOG(" Player collided with Enemy!");
                            _player->takeDamage(1, _groundRects); // 【修改】传入平台数据
							enemy->onCollideWithPlayer(_player->getPosition()); // 让敌人反应碰撞
                        }
                    }
                }
            }
        }

        // ========================================
        // 4.【修复】Zombie 敌人更新和碰撞检测
        // ========================================
        auto zombie = dynamic_cast<Zombie*>(_gameLayer->getChildByTag(998));
        if (zombie)
        {
            zombie->update(dt, playerPos, _groundRects);
            // 先获取碰撞箱，如果为空则跳过碰撞检测
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
                        zombie->takeDamage(1, _player->getPosition());  // 传入玩家位置
                        isZombieHit = true;

                        if (_player->getAttackDir() == -1) {
                            _player->pogoJump();
                        }
                    }
                }

                // B. Body Collision
                if (!isZombieHit)
                {
                    Rect playerBox = _player->getCollisionBox();
                    if (playerBox.intersectsRect(zombieBox))
                    {
                        if (!_player->isInvincible())
                        {
                            CCLOG("Player collided with Zombie body!");
                            _player->takeDamage(1, _groundRects); // 【修改】传入平台数据
                            zombie->onCollideWithPlayer(_player->getPosition());
                        }
                    }
                }
            }
        }

        // ========================================
        // 5.【新增】Spike 陷阱更新和碰撞检测
        // ========================================
        auto spike = dynamic_cast<Spike*>(_gameLayer->getChildByTag(997));
        if (spike)
        {
            // 更新刺的状态（检测玩家是否在下方，应用重力等）
            spike->update(dt, playerPos, _groundRects);

            // 【新增】更新屏幕上的Spike调试信息
            if (_spikeDebugLabel)
            {
                char debugText[200];
                int state = (int)spike->getHitbox().equals(Rect::ZERO) ? -1 : 0;
                sprintf(debugText, "Spike:(%.0f,%.0f) State:%d Visible:%d", 
                    spike->getPosition().x, spike->getPosition().y,
                    state, spike->isVisible());
                _spikeDebugLabel->setString(debugText);
            }

            // 【调试】在Scene坐标系中绘制Spike的位置
            if (_debugDrawNode)
            {
                _debugDrawNode->clear(); // 清除之前的绘制
                
                
            }

            // 获取刺的碰撞箱
            Rect spikeBox = spike->getHitbox();

            if (!spikeBox.equals(Rect::ZERO))
            {
                // 检测玩家是否与刺碰撞
                Rect playerBox = _player->getCollisionBox();
                if (playerBox.intersectsRect(spikeBox))
                {
                    if (!_player->isInvincible())
                    {
                        CCLOG("Player hit by Spike!");
                        _player->takeDamage(1, _groundRects); // 【修改】传入平台数据
                        // 刺造成伤害后可以选择消失或保持
                        // spike->changeState(Spike::State::DEAD);
                    }
                }
            }
        }
        else
        {
            // Spike不存在
            if (_spikeDebugLabel)
            {
                _spikeDebugLabel->setString("Spike: NOT FOUND (tag 997)");
                _spikeDebugLabel->setColor(Color3B::RED);
            }
        }

        // ========================================
        // 6.【新增】Buzzer 飞行敌人更新和碰撞检测
        // ========================================
        // Buzzer 1 (tag 996)
        auto buzzer1 = dynamic_cast<Buzzer*>(_gameLayer->getChildByTag(996));
        if (buzzer1)
        {
            // 更新Buzzer AI
            buzzer1->update(dt, playerPos);
            
            // 获取碰撞箱
            Rect buzzer1Box = buzzer1->getHitbox();
            
            if (!buzzer1Box.equals(Rect::ZERO))
            {
                bool isBuzzer1Hit = false;
                
                // A. 攻击检测
                if (_player->isAttackPressed())
                {
                    Rect attackBox = _player->getAttackHitbox();
                    if (attackBox.intersectsRect(buzzer1Box))
                    {
                        CCLOG("HIT! Player hit Buzzer 1!");
                        buzzer1->takeDamage(1, _player->getPosition());
                        isBuzzer1Hit = true;
                        
                        // 下劈时pogo跳跃
                        if (_player->getAttackDir() == -1) {
                            _player->pogoJump();
                        }
                    }
                }
                
                // B. 身体碰撞
                if (!isBuzzer1Hit)
                {
                    Rect playerBox = _player->getCollisionBox();
                    if (playerBox.intersectsRect(buzzer1Box))
                    {
                        if (!_player->isInvincible())
                        {
                            CCLOG("Player collided with Buzzer 1!");
                            _player->takeDamage(1, _groundRects);
                            buzzer1->onCollideWithPlayer(_player->getPosition()); // 【新增】让Buzzer击退
                        }
                    }
                }
            }
        }
        
        // Buzzer 2 (tag 995)
        auto buzzer2 = dynamic_cast<Buzzer*>(_gameLayer->getChildByTag(995));
        if (buzzer2)
        {
            // 更新Buzzer AI
            buzzer2->update(dt, playerPos);
            
            // 获取碰撞箱
            Rect buzzer2Box = buzzer2->getHitbox();
            
            if (!buzzer2Box.equals(Rect::ZERO))
            {
                bool isBuzzer2Hit = false;
                
                // A. 攻击检测
                if (_player->isAttackPressed())
                {
                    Rect attackBox = _player->getAttackHitbox();
                    if (attackBox.intersectsRect(buzzer2Box))
                    {
                        CCLOG("HIT! Player hit Buzzer 2!");
                        buzzer2->takeDamage(1, _player->getPosition());
                        isBuzzer2Hit = true;
                        
                        // 下劈时pogo跳跃
                        if (_player->getAttackDir() == -1) {
                            _player->pogoJump();
                        }
                    }
                }
                
                // B. 身体碰撞
                if (!isBuzzer2Hit)
                {
                    Rect playerBox = _player->getCollisionBox();
                    if (playerBox.intersectsRect(buzzer2Box))
                    {
                        if (!_player->isInvincible())
                        {
                            CCLOG("Player collided with Buzzer 2!");
                            _player->takeDamage(1, _groundRects);
                            buzzer2->onCollideWithPlayer(_player->getPosition()); // 【新增】让Buzzer击退
                        }
                    }
                }
            }
        }

        // ========================================
        // 7.【新增】Jar 罐子碰撞检测
        // ========================================
        for (auto jar : _jars)
        {
            if (!jar || jar->isDestroyed()) continue;
            
            // A. 玩家攻击罐子
            if (_player->isAttackPressed())
            {
                Rect attackBox = _player->getAttackHitbox();
                Rect jarBox = jar->getCollisionBox();
                
                if (!jarBox.equals(Rect::ZERO) && attackBox.intersectsRect(jarBox))
                {
                    CCLOG("Player hit the jar!");
                    jar->takeDamage();
                    
                    // 如果是下劈，执行 pogo 跳跃
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
                    // 简单的推出处理（防止玩家穿过罐子）
                    Vec2 playerPos = _player->getPosition();
                    
                    // 判断从哪个方向碰撞
                    float overlapLeft = (playerBox.getMaxX() - jarBox.getMinX());
                    float overlapRight = (jarBox.getMaxX() - playerBox.getMinX());
                    
                    if (overlapLeft < overlapRight)
                    {
                        // 从左边碰撞，推到左边
                        _player->setPositionX(jarBox.getMinX() - playerBox.size.width / 2);
                    }
                    else
                    {
                        // 从右边碰撞，推到右边
                        _player->setPositionX(jarBox.getMaxX() + playerBox.size.width / 2);
                    }
                    
                    // 停止水平速度
                    _player->setVelocityX(0);
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
    // 1. 清除旧地图
    auto oldMap = _gameLayer->getChildByTag(123);
    if (oldMap)
    {
        oldMap->removeFromParent();
        CCLOG("Old map removed");
    }

    // 2. 清除旧的碰撞框绘制节点
    auto oldDrawNode = dynamic_cast<DrawNode*>(_gameLayer->getChildByTag(1000));
    if (oldDrawNode)
    {
        oldDrawNode->removeFromParent();
        CCLOG("Old DrawNode removed");
    }

    // 3. 清除旧的敌人（level1的敌人）
    if (_currentLevel == 2)
    {
        // 移除 Enemy
        auto enemy = _gameLayer->getChildByTag(999);
        if (enemy) {
            enemy->removeFromParent();
            CCLOG("Enemy removed for level2");
        }

        // 移除 Zombie
        auto zombie = _gameLayer->getChildByTag(998);
        if (zombie) {
            zombie->removeFromParent();
            CCLOG("Zombie removed for level2");
        }

        // 移除 Spike
        auto spike = _gameLayer->getChildByTag(997);
        if (spike) {
            spike->removeFromParent();
            CCLOG("Spike removed for level2");
        }

        // 移除 Buzzer 1
        auto buzzer1 = _gameLayer->getChildByTag(996);
        if (buzzer1) {
            buzzer1->removeFromParent();
            CCLOG("Buzzer1 removed for level2");
        }

        // 移除 Buzzer 2
        auto buzzer2 = _gameLayer->getChildByTag(995);
        if (buzzer2) {
            buzzer2->removeFromParent();
            CCLOG("Buzzer2 removed for level2");
        }
        
        // 【新增】移除旧的 Fireball（如果存在）
        auto oldFireball = _gameLayer->getChildByTag(987);
        if (oldFireball) {
            oldFireball->removeFromParent();
            CCLOG("Old Fireball removed for level2");
        }
    }

    // 4. 加载新地图
    auto map = TMXTiledMap::create(mapPath);
    if (map == nullptr)
    {
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
    drawNode->setTag(1000);  // 给DrawNode设置tag，方便后续删除
    _gameLayer->addChild(drawNode, 999);

    for (const auto& rect : _groundRects)
    {
        drawNode->drawRect(rect.origin, rect.origin + rect.size, Color4F::RED);
    }

    CCLOG("========== Map Loaded ==========");
    CCLOG("Map: %s", mapPath.c_str());
    CCLOG("Collision rects count: %lu", _groundRects.size());
    CCLOG("================================");

    // 【新增】如果是 level2，创建罐子和幼虫
    if (_currentLevel == 2)
    {
        // 清空旧罐子列表
        _jars.clear();
        
        // 在 X=1700 到 X=3100 之间均匀放置 3 个罐子
        // 间距 = (3100 - 1700) / 2 = 700
        float startX = 1700.0f;
        float spacing = 700.0f;
        float jarY = 346.0f;  // 【修改】罐子放在地面上（level2 地面高度）
        
        for (int i = 0; i < 3; i++)
        {
            float jarX = startX + i * spacing;
            auto jar = Jar::create("warm/jar.png", Vec2(jarX, jarY));
            
            if (jar)
            {
                jar->setTag(990 - i);  // tags: 990, 989, 988
                _gameLayer->addChild(jar, 5);
                _jars.push_back(jar);
                
                CCLOG("Jar %d created at position (%.0f, %.0f)", i + 1, jarX, jarY);
            }
        }
        
        CCLOG("========== Level 2 Jars Created ==========");
        CCLOG("Total jars: %lu", _jars.size());
        CCLOG("==========================================");
        
        // 【新增】创建 Fireball 对象
        auto fireball = Fireball::create("fireball/fireball_1.png");
        if (fireball)
        {
            fireball->setPosition(Vec2(5529.0f, 650.0f));
            fireball->setTag(987);  // 给 fireball 一个 tag
            _gameLayer->addChild(fireball, 5);
            CCLOG("Fireball created at position (5529, 500)");
        }
        else
        {
            CCLOG("Error: Failed to create Fireball");
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

    // 1. 创建黑屏遮罩
    auto blackLayer = LayerColor::create(Color4B::BLACK);
    blackLayer->setOpacity(0);
    this->addChild(blackLayer, 999);  // 最高层级，覆盖所有内容

    // 2. 黑屏淡入动画序列
    auto fadeIn = FadeTo::create(0.5f, 255);  // 0.5秒渐黑
    
    auto switchMap = CallFunc::create([this]() {
        // 在黑屏时切换地图
        CCLOG("Loading level2.tmx...");
        
        // 先更新关卡编号（这样loadMap就知道要清理敌人）
        _currentLevel = 2;
        
        // 加载level2地图
        loadMap("maps/level2.tmx");
        
        // 重置玩家位置到level2的起点（最左端）
        if (_player)
        {
            _player->setPosition(Vec2(400, 1300));  // level2起点
            _player->setVelocityX(0);  // 重置水平速度
            CCLOG("Player repositioned to level2 start: (400, 1300)");
        }
    });
    
    auto delay = DelayTime::create(0.3f);  // 保持黑屏0.3秒
    
    auto fadeOut = FadeTo::create(0.5f, 0);  // 0.5秒渐亮
    
    auto cleanup = CallFunc::create([this, blackLayer]() {
        blackLayer->removeFromParent();
        _isTransitioning = false;
        CCLOG("========== Level 2 loaded successfully ==========");
    });

    // 3. 执行动作序列
    blackLayer->runAction(Sequence::create(
        fadeIn,      // 渐黑
        switchMap,   // 切换地图
        delay,       // 保持黑屏
        fadeOut,     // 渐亮
        cleanup,     // 清理
        nullptr
    ));
}
