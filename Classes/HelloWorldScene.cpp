#include "HelloWorldScene.h"
#include "SimpleAudioEngine.h"
#include "Enemy.h"
#include "Zombie.h"

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

    // 调整放大倍数为 1.5 倍
    float zoomScale = 1.5f;  
    this->setScale(zoomScale);

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

        // 创建 Zombie 敌人
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
    _player = Player::create("Knight/idle/idle_1.png");

    if (_player)
    {
        // 设置出生点 (根据地图调整)
        _player->setPosition(Vec2(400, 1300));
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
            updatePlayerMovement(); // 更新状态
            break;

        case EventKeyboard::KeyCode::KEY_A:
        case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
            _isLeftPressed = true;
            updatePlayerMovement(); // 更新状态            
            break;

        case EventKeyboard::KeyCode::KEY_W:
        case EventKeyboard::KeyCode::KEY_UP_ARROW:
            _isUpPressed = true;
            updatePlayerMovement(); // 更新状态
            break;

        case EventKeyboard::KeyCode::KEY_S:
        case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
            _isDownPressed = true;
            updatePlayerMovement(); // 更新状态
            break;

        case EventKeyboard::KeyCode::KEY_Z:
        case EventKeyboard::KeyCode::KEY_SPACE: // 空格跳跃
            _player->setJumpPressed(true);
            break;

        case EventKeyboard::KeyCode::KEY_J:
        case EventKeyboard::KeyCode::KEY_X:
        {
            // 调用主角攻击动画
            _player->setAttackPressed(true);
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
            updatePlayerMovement(); // 重新计算移动方向
            break;
        case EventKeyboard::KeyCode::KEY_D:
        case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
            _isRightPressed = false;
            updatePlayerMovement(); // 重新计算移动方向
            break;
        case EventKeyboard::KeyCode::KEY_W:
        case EventKeyboard::KeyCode::KEY_UP_ARROW:
            _isUpPressed = false;
            updatePlayerMovement(); // 重新计算移动方向
            break;
        case EventKeyboard::KeyCode::KEY_S:
        case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
            _isDownPressed = false;
            updatePlayerMovement(); // 重新计算移动方向
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
    auto map = this->getChildByTag(123);
    if (_player && map)
    {
        if (!_player) return;

        auto map = this->getChildByTag(123);
        if (!map) return;

        // ========================================
       // 1. 首先更新玩家位置
       // ========================================
        _player->update(dt, _groundRects);

        // ========================================
    // 2. 相机立即跟随玩家
    // ========================================
        Vec2 playerPos = _player->getPosition();

        Size visibleSize = Director::getInstance()->getVisibleSize();
        Size mapSize = map->getContentSize();
        float scaleValue = this->getScale();

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

        // 【直接应用】立即设置相机位置
        this->setPosition(targetX, targetY);

        // ========================================
		// 3. 【修复】修改 Enemy 碰撞检测逻辑
        // ========================================
        auto enemy = dynamic_cast<Enemy*>(this->getChildByTag(999));
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
                        enemy->takeDamage(1);
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
                            _player->takeDamage(1);
							enemy->onCollideWithPlayer(_player->getPosition()); // 让敌人反应碰撞
                        }
                    }
                }
            }
        }

        // ========================================
        // 4.【修复】Zombie 敌人更新和碰撞检测
        // ========================================
        auto zombie = dynamic_cast<Zombie*>(this->getChildByTag(998));
        if (zombie)
        {
            zombie->update(dt, playerPos);
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
                        zombie->takeDamage(1);
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
                            _player->takeDamage(1);
                            zombie->onCollideWithPlayer(_player->getPosition());
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
