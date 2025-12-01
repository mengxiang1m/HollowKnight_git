#include "HelloWorldScene.h"
#include "Enemy.h"

USING_NS_CC;

Scene* HelloWorld::createScene()
{
    return HelloWorld::create();
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
    auto bgLayer = LayerColor::create(Color4B(128, 128, 128, 255));
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

        CCLOG("Success: Map loaded!");

        auto objectGroup = map->getObjectGroup("Collisions");
        if (objectGroup) {
            CCLOG("Info: Found 'Collisions' layer.");
        }

        //////////////////////////////////////////////////////////////////////
        // 3. 创建敌人
        //////////////////////////////////////////////////////////////////////
        
        auto enemy = Enemy::create("enemies/enemy_walk_1.png");
        if (enemy)
        {
            enemy->setPosition(Vec2(400, 430));
            enemy->setPatrolRange(300, 800);
            enemy->setTag(999);
            this->addChild(enemy, 10);
            
            CCLOG("Enemy spawned at position (400, 430)!");
        }
        else
        {
            CCLOG("Error: Failed to create enemy!");
        }
    }

    //////////////////////////////////////////////////////////////////////
    // 4. 键盘监听器（地图移动 + 基于碰撞的攻击）
    //////////////////////////////////////////////////////////////////////
    auto listener = EventListenerKeyboard::create();

    listener->onKeyPressed = [=](EventKeyboard::KeyCode code, Event* event) {

        auto nodeMap = this->getChildByTag(123);

        // ========================================
        // 地图移动逻辑（保持不变）
        // ========================================
        if (nodeMap != nullptr)
        {
            Vec2 currentPos = nodeMap->getPosition();
            float speed = 50.0f;

            switch (code)
            {
            case EventKeyboard::KeyCode::KEY_D:
            case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
                nodeMap->setPosition(currentPos.x - speed, currentPos.y);
                break;

            case EventKeyboard::KeyCode::KEY_A:
            case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
                if (currentPos.x + speed <= 0) {
                    nodeMap->setPosition(currentPos.x + speed, currentPos.y);
                }
                break;

            case EventKeyboard::KeyCode::KEY_W:
            case EventKeyboard::KeyCode::KEY_UP_ARROW:
                nodeMap->setPosition(currentPos.x, currentPos.y - speed);
                break;

            case EventKeyboard::KeyCode::KEY_S:
            case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
                nodeMap->setPosition(currentPos.x, currentPos.y + speed);
                break;

            default:
                break;
            }

            CCLOG("Map Pos: (%f, %f)", nodeMap->getPosition().x, nodeMap->getPosition().y);
        }

        // ========================================
        // 【修改】按 J 键攻击 - 基于碰撞检测
        // ========================================
        if (code == EventKeyboard::KeyCode::KEY_J)
        {
            CCLOG("⚔ Player pressed J - Creating attack hitbox...");

            // ========================================
            // 1. 定义玩家位置和攻击方向
            // ========================================
            // 假设玩家在屏幕中央（你后续可以替换为真实的玩家位置）
            Vec2 playerPos = Vec2(400, 430);
            bool facingRight = true; // 假设玩家面向右边（你后续可以根据实际情况调整）

            // ========================================
            // 2. 创建攻击判定框（AttackBox）
            // ========================================
            float attackWidth = 80.0f;  // 攻击范围的宽度
            float attackHeight = 60.0f; // 攻击范围的高度
            float attackOffsetX = facingRight ? 40.0f : -40.0f; // 攻击框相对玩家的偏移

            // 攻击框的位置
            Vec2 attackBoxPos = Vec2(
                playerPos.x + attackOffsetX,
                playerPos.y
            );

            // 创建攻击判定区域（Rect）
            Rect attackBox = Rect(
                attackBoxPos.x - attackWidth / 2,
                attackBoxPos.y - attackHeight / 2,
                attackWidth,
                attackHeight
            );

            CCLOG("  Attack Box: (%.0f, %.0f, %.0f, %.0f)", 
                  attackBox.origin.x, attackBox.origin.y, 
                  attackBox.size.width, attackBox.size.height);

            // ========================================
            // 3. 【可选】可视化攻击范围（调试用）
            // ========================================
            auto debugBox = DrawNode::create();
            debugBox->drawSolidRect(
                Vec2(attackBox.origin.x, attackBox.origin.y),
                Vec2(attackBox.origin.x + attackBox.size.width, 
                     attackBox.origin.y + attackBox.size.height),
                Color4F(1.0f, 0.0f, 0.0f, 0.3f) // 半透明红色
            );
            this->addChild(debugBox, 1000);

            // 0.2秒后移除可视化框
            auto removeAction = Sequence::create(
                DelayTime::create(0.2f),
                RemoveSelf::create(),
                nullptr
            );
            debugBox->runAction(removeAction);

            // ========================================
            // 4. 碰撞检测：检查攻击框是否与敌人重叠
            // ========================================
            auto enemy = dynamic_cast<Enemy*>(this->getChildByTag(999));
            
            if (enemy)
            {
                Rect enemyHitbox = enemy->getHitbox();
                
                CCLOG("  Enemy Hitbox: (%.0f, %.0f, %.0f, %.0f)",
                      enemyHitbox.origin.x, enemyHitbox.origin.y,
                      enemyHitbox.size.width, enemyHitbox.size.height);

                // 判断两个矩形是否相交
                if (attackBox.intersectsRect(enemyHitbox))
                {
                    CCLOG("  ✓✓✓ HIT! Attack box intersects with enemy!");
                    enemy->takeDamage(1); // 造成伤害
                }
                else
                {
                    CCLOG("  ✗ MISS! Attack box does not hit enemy.");
                }
            }
            else
            {
                CCLOG("  ✗ No enemy found to attack");
            }
        }
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    //////////////////////////////////////////////////////////////////////
    // 5. 退出按钮
    //////////////////////////////////////////////////////////////////////
    auto closeItem = MenuItemImage::create(
        "CloseNormal.png",
        "CloseSelected.png",
        CC_CALLBACK_1(HelloWorld::menuCloseCallback, this));

    if (closeItem == nullptr ||
        closeItem->getContentSize().width <= 0 ||
        closeItem->getContentSize().height <= 0)
    {
        CCLOG("Problem loading 'CloseNormal.png' and 'CloseSelected.png'");
    }
    else
    {
        float x = origin.x + visibleSize.width - closeItem->getContentSize().width / 2;
        float y = origin.y + closeItem->getContentSize().height / 2;
        closeItem->setPosition(Vec2(x, y));
    }

    auto menu = Menu::create(closeItem, NULL);
    menu->setPosition(Vec2::ZERO);
    this->addChild(menu, 1);

    return true;
}

void HelloWorld::menuCloseCallback(Ref* pSender)
{
    Director::getInstance()->end();
}