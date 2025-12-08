#include "HelloWorldScene.h"
#include "SimpleAudioEngine.h"
#include "Enemy.h"
#include "Zombie.h"  // ← 添加这一行

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

    // 【修改】调整放大倍数为 1.5 倍
    float zoomScale = 1.5f;  // 从 2.0f 改为 1.5f
    this->setScale(zoomScale);

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    //////////////////////////////////////////////////////////////////////
    // 1. 背景
    //////////////////////////////////////////////////////////////////////
    auto bgLayer = LayerColor::create(Color4B(40, 40, 40, 255));
    this->addChild(bgLayer, -100);

    //////////////////////////////////////////////////////////////////////
    // 2. 加载地图
    //////////////////////////////////////////////////////////////////////
    auto map = TMXTiledMap::create("maps/level1.tmx");

    if (map == nullptr)
    {
        CCLOG("Error: Failed to load maps/level1.tmx");
    }
    else
    {
        map->setAnchorPoint(Vec2(0, 0));
        map->setPosition(Vec2(0, 0));
        map->setTag(123);
        this->addChild(map, -99);

        //解析碰撞数据
        this->parseMapCollisions(map);

        // =================【调试代码开始】=================
       // 创建一个 DrawNode 用来画红框
        auto drawNode = DrawNode::create();
        this->addChild(drawNode, 999);

        for (const auto& rect : _groundRects)
        {
            drawNode->drawRect(rect.origin, rect.origin + rect.size, Color4F::RED);
        }
        // =================【调试代码结束】=================
        //////////////////////////////////////////////////////////////////////
        // 
        // 3. 创建敌人
        //////////////////////////////////////////////////////////////////////
        auto enemy = Enemy::create("enemies/enemy_walk_1.png");
        if (enemy)
        {
            enemy->setPosition(Vec2(600, 430));
            enemy->setPatrolRange(500, 800);
            enemy->setTag(999);
            this->addChild(enemy, 5);
            CCLOG("Enemy spawned!");
        }

        // ========================================
        // 【新增】创建 Zombie 敌人
        // ========================================
        auto zombie = Zombie::create("zombie/walk/walk_1.png");
        if (zombie)
        {
            zombie->setPosition(Vec2(900, 430));
            zombie->setPatrolRange(800, 1200);
            zombie->setTag(998);
            this->addChild(zombie, 5);
            CCLOG("Zombie spawned!");
        }
    }

    //////////////////////////////////////////////////////////////////////
    // 4. 创建主角 (Player)
    //////////////////////////////////////////////////////////////////////
    // 文件名如果找不到会用默认的，确保 Knight/idle_1.png 存在
    _player = Player::create("Knight/idle/idle_1.png");

    if (_player)
    {
        // 设置出生点 (根据你的地图调整)
        _player->setPosition(Vec2(400, 730));
        this->addChild(_player, 10); // Z序 10，保证在最前面
        CCLOG("Player created successfully!");
    }
    else
    {
        CCLOG("Error: Failed to create Player!");
    }

    //////////////////////////////////////////////////////////////////////
    // 5. 键盘监听器
    //////////////////////////////////////////////////////////////////////
    auto listener = EventListenerKeyboard::create();

    // --- 按下按键 ---
    listener->onKeyPressed = [=](EventKeyboard::KeyCode code, Event* event) {

        // 如果主角不存在，直接返回，防止崩溃
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

        case EventKeyboard::KeyCode::KEY_Z:
        case EventKeyboard::KeyCode::KEY_SPACE: // 空格跳跃
            _player->startJump();  // 【修改】从 jump() 改为 startJump()
            break;

        case EventKeyboard::KeyCode::KEY_J:
        case EventKeyboard::KeyCode::KEY_X:  // 【新增】支持 X 键攻击
        {
            _player->attack();

            Rect attackBox = _player->getAttackHitbox();

            // 攻击普通敌人
            auto enemy = dynamic_cast<Enemy*>(this->getChildByTag(999));
            if (enemy)
            {
                if (attackBox.intersectsRect(enemy->getHitbox()))
                {
                    CCLOG("HIT! Player hit the Enemy!");
                    enemy->takeDamage(1);
                }
            }

            // 攻击 Zombie
            auto zombie = dynamic_cast<Zombie*>(this->getChildByTag(998));
            if (zombie)
            {
                if (attackBox.intersectsRect(zombie->getHitbox()))
                {
                    CCLOG("HIT! Player hit the Zombie!");
                    zombie->takeDamage(1);
                }
            }
        }
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

        case EventKeyboard::KeyCode::KEY_Z:
        case EventKeyboard::KeyCode::KEY_SPACE:
            _player->stopJump();  // 【新增】松开跳跃键
            break;
        }
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    //////////////////////////////////////////////////////////////////////
    // 6. 退出按钮
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
    this->addChild(menu, 100); // UI 放在最顶层

    // 开启 Update，用于相机跟随
    this->scheduleUpdate();

    return true;
}

void HelloWorld::update(float dt)
{
    if (!_player) return;
    
    auto map = this->getChildByTag(123);
    if (!map) return;

    // ========================================
    // 1. 首先更新玩家位置
    // ========================================
    _player->update(dt, _groundRects);

    // ========================================
    // 2. 【修复】相机立即跟随玩家
    // ========================================
    Vec2 playerPos = _player->getPosition();

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Size mapSize = map->getContentSize();
    float scaleValue = this->getScale();
    
    // 【关键修复】相机偏移需要乘以缩放因子
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
    } else {
        targetX = (visibleSize.width - scaledMapWidth) * 0.5f;
    }
    
    if (scaledMapHeight > visibleSize.height) {
        targetY = std::max(minY, std::min(targetY, maxY));
    } else {
        targetY = (visibleSize.height - scaledMapHeight) * 0.5f;
    }
    
    // 【直接应用】立即设置相机位置
    this->setPosition(targetX, targetY);

    // ========================================
    // 普通敌人更新和碰撞检测
    // ========================================
    auto enemy = dynamic_cast<Enemy*>(this->getChildByTag(999));
    if (enemy)
    {
        // 【修复】先获取碰撞箱，如果为空则跳过碰撞检测
        Rect enemyBox = enemy->getHitbox();
        
        // 只有当敌人碰撞箱有效时才进行碰撞检测
        if (!enemyBox.equals(Rect::ZERO))
        {
            Rect playerBox = _player->getCollisionBox();

            if (playerBox.intersectsRect(enemyBox))
            {
                enemy->onCollideWithPlayer(playerPos);
                
                if (!_player->isInvincible())
                {
                    CCLOG("⚠ Player collided with Enemy!");
                    _player->takeDamage(1);
                }
            }
        }
    }

    // ========================================
    // Zombie 敌人更新和碰撞检测
    // ========================================
    auto zombie = dynamic_cast<Zombie*>(this->getChildByTag(998));
    if (zombie)
    {
        zombie->update(dt, playerPos);

        // 【修复】先获取碰撞箱，如果为空则跳过碰撞检测
        Rect zombieBox = zombie->getHitbox();
        
        // 只有当僵尸碰撞箱有效时才进行碰撞检测
        if (!zombieBox.equals(Rect::ZERO))
        {
            Rect playerBox = _player->getCollisionBox();

            if (playerBox.intersectsRect(zombieBox))
            {
                zombie->onCollideWithPlayer(playerPos);
                
                if (!_player->isInvincible())
                {
                    CCLOG("⚠ Player collided with Zombie!");
                    _player->takeDamage(1);
                }
            }
        }
    }
}

void HelloWorld::menuCloseCallback(Ref* pSender)
{
    Director::getInstance()->end();
}

// 【新增】处理玩家移动的辅助函数
void HelloWorld::updatePlayerMovement()
{
    if (!_player) return;

    if (_isLeftPressed && _isRightPressed) {
        // 同时按住左右键，停止移动
        _player->stopMove();
    }
    else if (_isLeftPressed) {
        _player->moveLeft();
    }
    else if (_isRightPressed) {
        _player->moveRight();
    }
    else {
        // 两个都没按，停止移动
        _player->stopMove();
    }
}