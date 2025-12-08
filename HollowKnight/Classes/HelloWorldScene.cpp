#include "HelloWorldScene.h"
#include "SimpleAudioEngine.h"
#include "Enemy.h"

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

    //////////////////////////////////////////////////////////////////////
    // 1. 背景
    //////////////////////////////////////////////////////////////////////
    auto bgLayer = LayerColor::create(Color4B(40, 40, 40, 255)); // 稍微调暗一点，更有氛围
    this->addChild(bgLayer, -100);

    //////////////////////////////////////////////////////////////////////
    // 2. 加载地图
    //////////////////////////////////////////////////////////////////////
    // 确保你的 Resources/maps/level1.tmx 文件存在
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
        this->addChild(drawNode, 999); // Z序设高一点，保证画在最上面

        for (const auto& rect : _groundRects)
        {
            // 画红色空心矩形
            // rect.origin 是左下角，rect.origin + rect.size 是右上角
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
            // 放在地图的一个平台上
            enemy->setPosition(Vec2(600, 430));
            enemy->setPatrolRange(500, 800);
            enemy->setTag(999);
            this->addChild(enemy, 5); // Z序 5

            CCLOG("Enemy spawned!");
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
            _player->moveRight(); // 调用 Player 里的函数
            break;

        case EventKeyboard::KeyCode::KEY_A:
        case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
            _player->moveLeft();  // 调用 Player 里的函数
            break;

        case EventKeyboard::KeyCode::KEY_Z:
        case EventKeyboard::KeyCode::KEY_SPACE: // 空格跳跃
            _player->jump();
            break;

        case EventKeyboard::KeyCode::KEY_J:
        {
            // 调用主角攻击动画
            _player->attack();

            // ========================================
            // 攻击判定逻辑 (移植到这里)
            // ========================================
            // 获取主角实时的攻击判定框
            Rect attackBox = _player->getAttackHitbox();

            // 找敌人
            auto enemy = dynamic_cast<Enemy*>(this->getChildByTag(999));
            if (enemy)
            {
                if (attackBox.intersectsRect(enemy->getHitbox()))
                {
                    CCLOG("HIT! Player hit the Enemy!");
                    enemy->takeDamage(1);

                    // 可选：添加一点打击特效或震屏
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
        case EventKeyboard::KeyCode::KEY_D:
        case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
            _player->stopMove(); // 松手停止
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

// 每帧更新：实现相机跟随
// HelloWorldScene.cpp

void HelloWorld::update(float dt)
{
    auto map = this->getChildByTag(123);
    if (_player && map)
    {
        // === 相机跟随逻辑（保持不变）===
        Size visibleSize = Director::getInstance()->getVisibleSize();
        Size mapSize = map->getContentSize();
        Vec2 playerPos = _player->getPosition();

        float screenRatioX = 0.3f;
        float targetX = visibleSize.width * screenRatioX - playerPos.x;

        float minX = -(mapSize.width - visibleSize.width);
        float maxX = 0.0f;

        if (mapSize.width >= visibleSize.width) {
            targetX = std::max(minX, std::min(targetX, maxX));
        }
        else {
            targetX = 0.0f;
        }

        float targetY = 0.0f;
        float minY = -(mapSize.height - visibleSize.height);
        float maxY = 0.0f;

        if (mapSize.height >= visibleSize.height) {
            targetY = std::max(minY, std::min(targetY, maxY));
        }
        else {
            targetY = 0.0f;
        }

        this->setPosition(targetX, targetY);

        // === 玩家物理更新 ===
        if (_player)
        {
            _player->update(dt, _groundRects);
        }

        // ========================================
        // 【新增】敌人与玩家碰撞检测
        // ========================================
        auto enemy = dynamic_cast<Enemy*>(this->getChildByTag(999));
        if (enemy && _player)
        {
            // 获取双方的碰撞箱
            Rect playerBox = _player->getCollisionBox();
            Rect enemyBox = enemy->getHitbox();

            // 判断是否碰撞
            if (playerBox.intersectsRect(enemyBox))
            {
                // 只有在玩家非无敌状态时才造成伤害
                if (!_player->isInvincible())
                {
                    CCLOG("⚠ Player collided with Enemy! Taking damage...");
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