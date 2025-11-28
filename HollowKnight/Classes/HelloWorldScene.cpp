#include "HelloWorldScene.h"

USING_NS_CC;

Scene* HelloWorld::createScene()
{
    return HelloWorld::create();
}

// -----------------------------------------------------------------------
// 核心初始化函数
// -----------------------------------------------------------------------
bool HelloWorld::init()
{
    //////////////////////////////
    // 1. 父类初始化检查
    if (!Scene::init())
    {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    //////////////////////////////////////////////////////////////////////
    // 2. Member B 任务: 搭建地图环境
    //////////////////////////////////////////////////////////////////////

    // [辅助背景] 添加一个灰色底板，防止黑色的石头在黑色背景下看不见
    auto bgLayer = LayerColor::create(Color4B(128, 128, 128, 255));
    this->addChild(bgLayer, -100); // 放在最底层

    // [加载地图] 
    // Cocos2d-x 会自动在 Resources/maps/ 下寻找 level1.tmx
    // 注意：前提是你已经把 TMX 文件复制到了 Debug.win32/maps/ 下
    auto map = TMXTiledMap::create("maps/level1.tmx");

    if (map == nullptr)
    {
        // 错误处理：如果路径不对，打印红字
        CCLOG("Error: Failed to load maps/level1.tmx");
    }
    else
    {
        // 设置锚点和位置
        map->setAnchorPoint(Vec2(0, 0));
        map->setPosition(Vec2(0, 0));

        // [关键] 设置一个 Tag (标签)，方便我们在键盘事件里找回这个地图对象
        map->setTag(123);

        // 添加到场景，z-order 设为 -99，作为背景
        this->addChild(map, -99);

        CCLOG("Success: Map loaded!");

        // 检查物理层是否存在 (验证 Member B 的物理任务)
        auto objectGroup = map->getObjectGroup("Collisions");
        if (objectGroup) {
            CCLOG("Info: Found 'Collisions' layer.");
        }
    }

    //////////////////////////////////////////////////////////////////////
    // 3. 调试功能: 键盘控制地图移动 (模拟摄像机)
    //////////////////////////////////////////////////////////////////////

    // 创建键盘监听器
    auto listener = EventListenerKeyboard::create();

    // 绑定按键按下事件 (使用 Lambda 表达式)
    listener->onKeyPressed = [=](EventKeyboard::KeyCode code, Event* event) {

        // 1. 获取地图对象 (通过刚才设置的 Tag: 123)
        auto nodeMap = this->getChildByTag(123);

        // 如果地图没加载成功，直接返回，防崩溃
        if (nodeMap == nullptr) return;

        // 2. 获取当前位置
        Vec2 currentPos = nodeMap->getPosition();
        float speed = 50.0f; // 每次按键移动的距离

        switch (code)
        {
            // 向右看 (D 或 右箭头) -> 地图应该向左移
        case EventKeyboard::KeyCode::KEY_D:
        case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
            nodeMap->setPosition(currentPos.x - speed, currentPos.y);
            break;

            // 向左看 (A 或 左箭头) -> 地图应该向右移
        case EventKeyboard::KeyCode::KEY_A:
        case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
            // 限制：不要移出左边界太多 (可选)
            if (currentPos.x + speed <= 0) {
                nodeMap->setPosition(currentPos.x + speed, currentPos.y);
            }
            break;

            // 向上看 (W 或 上箭头) -> 地图应该向下移
        case EventKeyboard::KeyCode::KEY_W:
        case EventKeyboard::KeyCode::KEY_UP_ARROW:
            nodeMap->setPosition(currentPos.x, currentPos.y - speed);
            break;

            // 向下看 (S 或 下箭头) -> 地图应该向上移
        case EventKeyboard::KeyCode::KEY_S:
        case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
            nodeMap->setPosition(currentPos.x, currentPos.y + speed);
            break;

        default:
            break;
        }

        // 打印坐标，方便调试
        CCLOG("Map Pos: (%f, %f)", nodeMap->getPosition().x, nodeMap->getPosition().y);
        };

    // 将监听器添加到事件分发器
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);


    //////////////////////////////////////////////////////////////////////
    // 4. 标准退出按钮 (保留)
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