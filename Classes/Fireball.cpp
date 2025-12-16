#include "Fireball.h"

Fireball* Fireball::create(const std::string& firstFrame)
{
    Fireball* fireball = new (std::nothrow) Fireball();
    if (fireball && fireball->init(firstFrame))
    {
        fireball->autorelease();
        return fireball;
    }
    CC_SAFE_DELETE(fireball);
    return nullptr;
}

bool Fireball::init(const std::string& firstFrame)
{
    if (!Sprite::init())
    {
        return false;
    }
    
    // 加载第一帧图片
    auto texture = Director::getInstance()->getTextureCache()->addImage(firstFrame);
    if (texture)
    {
        this->setTexture(texture);
        Size texSize = texture->getContentSize();
        this->setTextureRect(Rect(0, 0, texSize.width, texSize.height));
    }
    else
    {
        CCLOG("Error: Failed to load fireball first frame: %s", firstFrame.c_str());
        return false;
    }
    
    // 设置锚点为中心
    this->setAnchorPoint(Vec2(0.5f, 0.5f));
    
    // 播放动画
    playAnimation();
    
    CCLOG("Fireball created successfully");
    
    return true;
}

void Fireball::playAnimation()
{
    // 创建动画（4帧，循环播放）
    Vector<SpriteFrame*> frames;
    
    for (int i = 1; i <= 4; i++)
    {
        std::string frameName = StringUtils::format("fireball/fireball_%d.png", i);
        auto texture = Director::getInstance()->getTextureCache()->addImage(frameName);
        if (texture)
        {
            Size texSize = texture->getContentSize();
            auto frame = SpriteFrame::createWithTexture(texture, Rect(0, 0, texSize.width, texSize.height));
            if (frame)
            {
                frames.pushBack(frame);
            }
        }
        else
        {
            CCLOG("Warning: Failed to load frame: %s", frameName.c_str());
        }
    }
    
    if (frames.size() > 0)
    {
        // 创建动画：每帧0.1秒
        auto animation = Animation::createWithSpriteFrames(frames, 0.1f);
        auto animate = Animate::create(animation);
        auto repeatForever = RepeatForever::create(animate);
        
        this->runAction(repeatForever);
        CCLOG("Fireball animation started (looping 4 frames)");
    }
    else
    {
        CCLOG("Error: Failed to load any fireball animation frames");
    }
}

void Fireball::loadAnimation()
{
    // 备用方法，如果需要的话
}
