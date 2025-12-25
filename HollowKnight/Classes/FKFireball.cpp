#include "FKFireball.h"

USING_NS_CC;

const float FK_GRAVITY = -1000.0f;

FKFireball* FKFireball::create(const std::string& imagePath)
{
    FKFireball* pRet = new(std::nothrow) FKFireball();
    if (pRet && pRet->init(imagePath))
    {
        pRet->autorelease();
        return pRet;
    }
    else
    {
        delete pRet;
        pRet = nullptr;
        return nullptr;
    }
}

bool FKFireball::init(const std::string& imagePath)
{
    if (!Node::init())
    {
        return false;
    }

    _sprite = Sprite::create(imagePath);
    if (_sprite)
    {
        this->addChild(_sprite);
    }

    _velocity = Vec2(0, 0);

    return true;
}

void FKFireball::update(float dt, const std::vector<Rect>& groundRects)
{
    _velocity.y += FK_GRAVITY * dt;
    this->setPosition(this->getPosition() + _velocity * dt);

    Rect bbox = getCollisionBox();
    for (const auto& ground : groundRects)
    {
        if (bbox.intersectsRect(ground))
        {
            this->removeFromParent();
            return;
        }
    }
}

Rect FKFireball::getCollisionBox() const
{
    if (!_sprite) return Rect::ZERO;

    Size size = _sprite->getContentSize();
    Vec2 anchor = _sprite->getAnchorPoint();
    Vec2 pos = this->getPosition();

    float originX = pos.x - size.width * anchor.x;
    float originY = pos.y - size.height * anchor.y;
    return Rect(originX, originY, size.width, size.height);
}
