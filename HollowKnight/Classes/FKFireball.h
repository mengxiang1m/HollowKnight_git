#ifndef __FK_FIREBALL_H__
#define __FK_FIREBALL_H__

#include "cocos2d.h"

class FKFireball : public cocos2d::Node
{
public:
    static FKFireball* create(const std::string& imagePath);
    virtual bool init(const std::string& imagePath);

    void update(float dt, const std::vector<cocos2d::Rect>& groundRects);
    cocos2d::Rect getCollisionBox() const;

private:
    cocos2d::Sprite* _sprite;
    cocos2d::Vec2 _velocity;
};

#endif // __FK_FIREBALL_H__
