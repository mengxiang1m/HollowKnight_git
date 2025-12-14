#include "HUDLayer.h"
#include "config.h" // 假设你有 Config，没有的话直接写数字

USING_NS_CC;

HUDLayer* HUDLayer::createLayer()
{
    HUDLayer* layer = HUDLayer::create();
    return layer;
}

bool HUDLayer::init()
{
    if (!Layer::init())
    {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();

    // 1. 创建容器
    _healthBarContainer = Node::create();
    // 设置在屏幕左上角 (空洞骑士风格)
    // 注意：Cocos坐标系原点在左下角
    _healthBarContainer->setPosition(Vec2(50, visibleSize.height - 50));
    this->addChild(_healthBarContainer);

    // 2. 初始时不创建具体的血，等 Player 告诉我们有多少血再创建
    // 或者在这里先初始化最大血量的空壳

    return true;
}

void HUDLayer::updateHealth(int currentHp, int maxHp)
{
    // 如果是第一次调用（或者最大血量变了），重新生成所有图标
    if (_heartSprites.size() != maxHp)
    {
        // 清理旧的
        _healthBarContainer->removeAllChildren();
        _heartSprites.clear();

        for (int i = 0; i < maxHp; i++)
        {
            // 这里用 Sprite::create 加载图片
            // 如果你还没图，先用 DrawNode 画个圆代替
            /* auto heart = Sprite::create("ui/heart_full.png");
            */

            // --- 临时替代方案：用 DrawNode 画圆 ---
            auto heart = Sprite::create(); // 空精灵做容器
            auto draw = DrawNode::create();
            draw->drawSolidCircle(Vec2::ZERO, 15, 0, 20, Color4F::WHITE); // 白色代表满血
            draw->setName("drawing"); // 给个名字方便查找
            heart->addChild(draw);
            // ------------------------------------

            // 排列位置：横向排列，间距 40
            heart->setPosition(Vec2(i * 40.0f, 0));
            _healthBarContainer->addChild(heart);
            _heartSprites.push_back(heart);
        }
    }

    // 更新状态 (满血显示亮色/满图，扣血显示暗色/空图)
    for (int i = 0; i < _heartSprites.size(); i++)
    {
        auto heart = _heartSprites[i];

        // 获取画图节点 (如果你用了图片，直接更换 Texture 即可)
        auto draw = dynamic_cast<DrawNode*>(heart->getChildByName("drawing"));

        if (i < currentHp)
        {
            // 这格血是有的 -> 显示满血状态
            // heart->setTexture("ui/heart_full.png"); 
            if (draw) {
                draw->clear();
                draw->drawSolidCircle(Vec2::ZERO, 15, 0, 20, Color4F::WHITE); // 亮白
            }
            heart->setOpacity(255);
        }
        else
        {
            // 这格血没了 -> 显示空血状态
            // heart->setTexture("ui/heart_empty.png");
            if (draw) {
                draw->clear();
                draw->drawCircle(Vec2::ZERO, 15, 0, 20, false, Color4F::GRAY); // 空心灰圈
            }
            heart->setOpacity(100); // 变暗
        }
    }
}