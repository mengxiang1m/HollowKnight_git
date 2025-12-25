#include "Jar.h"

Jar* Jar::create(const std::string& jarImage, const Vec2& position)
{
    Jar* jar = new (std::nothrow) Jar();
    if (jar && jar->init(jarImage, position))
    {
        jar->autorelease();
        return jar;
    }
    CC_SAFE_DELETE(jar);
    return nullptr;
}

bool Jar::init(const std::string& jarImage, const Vec2& position)
{
    if (!Node::init())
    {
        return false;
    }
    
    _isDestroyed = false;
    
    // 1. 创建罐子精灵
    _jarSprite = Sprite::create(jarImage);
    if (!_jarSprite)
    {
        CCLOG("Error: Failed to load jar image: %s", jarImage.c_str());
        return false;
    }
    
    _jarSprite->setAnchorPoint(Vec2(0.5f, 0.0f)); // 底部对齐
    this->addChild(_jarSprite, 1);
    
    // 2. 创建幼虫精灵（在罐子上方）
    _grubSprite = Sprite::create("warm/attach/attach_1.png");
    if (!_grubSprite)
    {
        CCLOG("Error: Failed to load grub image");
        return false;
    }
    
    _grubSprite->setAnchorPoint(Vec2(0.5f, 0.0f));
    
    // 幼虫位置：罐子内部，向下调整
    float jarHeight = _jarSprite->getContentSize().height;
    _grubSprite->setPosition(Vec2(0, jarHeight * 0.2f)); // 【修改】降低高度，让幼虫看起来在罐子内部
    
    this->addChild(_grubSprite, 2); // 幼虫在罐子上层
    
    // 3. 设置节点位置
    this->setPosition(position);
    
    // 4. 播放幼虫附着动画
    playGrubAttachAnimation();
    
    CCLOG("Jar created at position (%.0f, %.0f)", position.x, position.y);
    
    return true;
}

Rect Jar::getCollisionBox() const
{
    if (_isDestroyed || !_jarSprite)
    {
        return Rect::ZERO;
    }
    
    // 罐子的碰撞箱（侧面，防止穿过）
    Size jarSize = _jarSprite->getContentSize();
    Vec2 worldPos = this->getPosition();
    
    // 碰撞箱稍微缩小一点，更精确
    float boxWidth = jarSize.width * 0.8f;
    float boxHeight = jarSize.height * 0.9f;
    
    return Rect(
        worldPos.x - boxWidth / 2,
        worldPos.y,
        boxWidth,
        boxHeight
    );
}

Rect Jar::getTopPlatformBox() const
{
    if (_isDestroyed || !_jarSprite)
    {
        return Rect::ZERO;
    }
    
    // 罐子顶部平台（用于站立）
    Size jarSize = _jarSprite->getContentSize();
    Vec2 worldPos = this->getPosition();
    
    // 顶部平台：比罐子宽度略窄，高度很薄
    float platformWidth = jarSize.width * 0.9f;  // 顶部宽度
    float platformHeight = 20.0f;  // 平台厚度
    float topY = worldPos.y + jarSize.height * 0.85f;  // 罐子顶部位置
    
    return Rect(
        worldPos.x - platformWidth / 2,
        topY,
        platformWidth,
        platformHeight
    );
}

void Jar::takeDamage()
{
    if (_isDestroyed) return;
    
    CCLOG("Jar broken!");
    _isDestroyed = true;
    
    // 罐子消失
    if (_jarSprite)
    {
        _jarSprite->runAction(Sequence::create(
            FadeOut::create(0.2f),
            RemoveSelf::create(),
            nullptr
        ));

        // 在罐子精灵真正被移除后将指针置空，避免悬挂指针导致后续访问崩溃
        // 使用延迟调用在父节点上清理指针（避免在被移除的精灵上运行回调）
        this->runAction(Sequence::create(
            DelayTime::create(0.21f),
            CallFunc::create([this]() {
                this->_jarSprite = nullptr;
            }),
            nullptr
        ));
    }
    
    // 播放幼虫释放动画
    playGrubFreeAnimation();
}

void Jar::playGrubAttachAnimation()
{
    if (!_grubSprite) return;
    
    // 创建 attach 动画（5帧，循环播放）
    Vector<SpriteFrame*> frames;
    for (int i = 1; i <= 5; i++)
    {
        std::string frameName = StringUtils::format("warm/attach/attach_%d.png", i);
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
    }
    
    if (frames.size() > 0)
    {
        auto animation = Animation::createWithSpriteFrames(frames, 0.15f); // 每帧0.15秒
        auto animate = Animate::create(animation);
        auto repeatForever = RepeatForever::create(animate);
        
        _grubSprite->runAction(repeatForever);
        CCLOG("Grub attach animation started (looping)");
    }
    else
    {
        CCLOG("Warning: Failed to load attach animation frames");
    }
}

void Jar::playGrubFreeAnimation()
{
    if (!_grubSprite) return;
    
    // 停止之前的动画
    _grubSprite->stopAllActions();
    
    // 创建 free 动画（6帧，播放一次）
    Vector<SpriteFrame*> frames;
    for (int i = 1; i <= 6; i++)
    {
        std::string frameName = StringUtils::format("warm/free/free_%d.png", i);
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
    }
    
    if (frames.size() > 0)
    {
        float frameDuration = 0.1f;
        float freeAnimDuration = frames.size() * frameDuration;
        float fadeDuration = 0.3f;
        float totalGrubRemovalDelay = freeAnimDuration + fadeDuration;

        auto animation = Animation::createWithSpriteFrames(frames, frameDuration); // 每帧0.1秒
        auto animate = Animate::create(animation);
        
        // 播放完后淡出并移除
        auto fadeOut = FadeOut::create(fadeDuration);
        auto remove = RemoveSelf::create();
        
        _grubSprite->runAction(Sequence::create(
            animate,
            fadeOut,
            remove,
            nullptr
        ));
        
        CCLOG("Grub free animation started (once)");
        
        // 在幼虫精灵真正被移除后将指针置空，避免悬挂指针
        this->runAction(Sequence::create(
            DelayTime::create(totalGrubRemovalDelay + 0.05f),
            CallFunc::create([this]() {
                this->_grubSprite = nullptr;
            }),
            nullptr
        ));
        
        // 延迟后移除整个节点
        this->runAction(Sequence::create(
            DelayTime::create(1.5f),
            RemoveSelf::create(),
            nullptr
        ));
    }
    else
    {
        CCLOG("Warning: Failed to load free animation frames");
    }
}

void Jar::onJarBroken()
{
    // 可以在这里添加破碎音效、粒子特效等
}
