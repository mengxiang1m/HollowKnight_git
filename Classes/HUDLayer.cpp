#include "HUDLayer.h"
#include "config.h" 

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

    float scale = 2.5f; 
    _healthBarContainer->setScale(scale);

    // 设置在屏幕左上角
    _healthBarContainer->setPosition(Vec2(100, visibleSize.height - 180));
    this->addChild(_healthBarContainer);

    return true;
}

void HUDLayer::updateHealth(int currentHp, int maxHp)
{
    // --------------------------------------------------------
     // A. 初始化血条 (如果数量不对，重新创建)
     // --------------------------------------------------------
    if (_heartSprites.size() != maxHp)
    {
        _healthBarContainer->removeAllChildren();
        _heartSprites.clear();

        for (int i = 0; i < maxHp; i++)
        {
            // 默认创建满血图
            auto heart = Sprite::create("HUDanim/full/full.png");

            // 间距调整
            heart->setPosition(Vec2(i * 50.0f, 0));

            // 【关键】设置初始 Tag 为 1 (满血状态)
            heart->setTag(1);

            _healthBarContainer->addChild(heart);
            _heartSprites.push_back(heart);
        }
    }

    // --------------------------------------------------------
    // B. 更新状态
    // --------------------------------------------------------
    for (int i = 0; i < _heartSprites.size(); i++)
    {
        auto heart = _heartSprites[i];
        int currentTag = heart->getTag(); // 获取当前记录的状态

        // 情况 1: 这一格应该有血
        if (i < currentHp)
        {
            // 如果之前不是满血，或者是第一次刷新
            if (currentTag != 1)
            {
                heart->setTexture("HUDanim/full/full.png");
                heart->setOpacity(255);
                heart->setTag(1); // 标记为满血
            }
        }
        // 情况 2: 这一格应该没血
        else
        {
            // 检查：它之前是否满血
            // 如果 Tag 是 1，说明它刚刚还在，现在没了 ,触发破碎
            if (currentTag == 1)
            {
                // 1. 播放破碎动画 (盖在上面)
                playBreakAnimation(heart);

                // 2. 自身立刻切换为空血槽 (作为背景垫在下面)
                heart->setTexture("HUDanim/empty/empty.png");

                // 3. 标记为空血
                heart->setTag(0);
            }
            // 如果 Tag 已经是 0，说明早就碎了，不需要再播动画
            else
            {
                // 确保显示空血图 (防止切图切错了)
                if (heart->getTexture() == nullptr ||
                    heart->getTexture()->getPath() != "HUDanim/empty/empty.png")
                {
                    heart->setTexture("HUDanim/empty/empty.png");
                }
            }
        }
    }
}

void HUDLayer::playBreakAnimation(Sprite* heartSprite)
{
    // 1. 加载动画帧
    Vector<SpriteFrame*> frames;
    for (int i = 1; i <= 6; i++)
    {
        std::string name = StringUtils::format("HUDanim/break/break_%d.png", i);
        auto sprite = Sprite::create(name);
        if (sprite) {
            frames.pushBack(sprite->getSpriteFrame());
        }
    }

    if (frames.empty()) return;

    // 2. 创建动画 (速度快一点，0.04秒一帧)
    auto animation = Animation::createWithSpriteFrames(frames, 0.06f);
    auto animate = Animate::create(animation);

    // 3. 创建临时 Sprite
    // 因为所有图片尺寸一样，直接对齐坐标即可，无需偏移
    auto effectSprite = Sprite::create();
    effectSprite->setPosition(heartSprite->getPosition());

    // Z序设为 20，保证盖在空血槽上面
    _healthBarContainer->addChild(effectSprite, 20);

    // 4. 播放并销毁
    auto seq = Sequence::create(
        animate,
        RemoveSelf::create(),
        nullptr
    );

    effectSprite->runAction(seq);
}