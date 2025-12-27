#include "FKShockwave.h"

USING_NS_CC;

namespace
{
    const float FK_SHOCKWAVE_SPEED = 1200.0f;
}

FKShockwave* FKShockwave::create(const std::string& imagePath, float direction)
{
    FKShockwave* pRet = new(std::nothrow) FKShockwave();
    if (pRet && pRet->init(imagePath, direction))
    {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
}

bool FKShockwave::init(const std::string& imagePath, float direction)
{
    if (!Node::init())
    {
        return false;
    }

    _sprite = Sprite::create(imagePath);
    if (_sprite)
    {
        // --- 修改开始: 将图片尺寸缩小一半 ---
        _sprite->setScale(0.45f);
        // --- 修改结束 ---

        _sprite->setAnchorPoint(Vec2(0.5f, 0.25f));
        this->addChild(_sprite);
    }

    _dir = (direction >= 0) ? 1.0f : -1.0f;
    _velocity = Vec2(FK_SHOCKWAVE_SPEED * _dir, 0.0f);

    return true;
}

void FKShockwave::update(float dt, const std::vector<Rect>& groundRects)
{
    Vec2 delta = _velocity * dt;
    this->setPosition(this->getPosition() + delta);
    _lifeDistance += std::abs(delta.x);

    Rect bbox = getCollisionBox();

    // 只在撞到垂直墙体时移除；允许贴地滑行
    for (const auto& rect : groundRects)
    {
        float overlapY = std::min(bbox.getMaxY(), rect.getMaxY()) - std::max(bbox.getMinY(), rect.getMinY());
        if (overlapY <= 5.0f) continue; // 垂直方向没有有效重叠

        if (_dir > 0)
        {
            if (bbox.getMaxX() >= rect.getMinX() && bbox.getMinX() < rect.getMinX())
            {
                this->removeFromParent();
                return;
            }
        }
        else
        {
            if (bbox.getMinX() <= rect.getMaxX() && bbox.getMaxX() > rect.getMaxX())
            {
                this->removeFromParent();
                return;
            }
        }
    }

    // 兜底寿命，防止无限存在
    if (_lifeDistance > 4000.0f)
    {
        this->removeFromParent();
    }
}

Rect FKShockwave::getCollisionBox() const
{
    if (!_sprite) return Rect::ZERO;

    Size size = _sprite->getContentSize();
    float scaleX = _sprite->getScaleX();
    float scaleY = _sprite->getScaleY();

    // 计算实际显示的大小
    float finalWidth = size.width * scaleX;
    float finalHeight = size.height * scaleY;

    // --- 修改开始: 缩小碰撞箱 ---
    // 在视觉大小的基础上，进一步缩小碰撞判定区域，使其更易于躲避
    float hitBoxRatioW = 0.6f; // 宽度缩小为 60%
    float hitBoxRatioH = 0.7f; // 高度缩小为 70%

    float hitWidth = finalWidth * hitBoxRatioW;
    float hitHeight = finalHeight * hitBoxRatioH;

    Vec2 anchor = _sprite->getAnchorPoint();
    Vec2 pos = this->getPosition();

    // 计算视觉中心
    float centerX = pos.x + finalWidth * (0.5f - anchor.x);
    float centerY = pos.y + finalHeight * (0.5f - anchor.y);

    // 基于视觉中心生成缩小的碰撞框
    return Rect(centerX - hitWidth / 2, centerY - hitHeight / 2, hitWidth, hitHeight);
    // --- 修改结束 ---
}