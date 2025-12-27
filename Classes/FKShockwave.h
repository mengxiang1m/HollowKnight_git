#ifndef __FK_SHOCKWAVE_H__
#define __FK_SHOCKWAVE_H__

#include "cocos2d.h"

class FKShockwave : public cocos2d::Node
{
public:
    static FKShockwave* create(const std::string& imagePath, float direction);
    virtual bool init(const std::string& imagePath, float direction);

    void update(float dt, const std::vector<cocos2d::Rect>& groundRects);
    cocos2d::Rect getCollisionBox() const;

private:
    cocos2d::Sprite* _sprite = nullptr;
    cocos2d::Vec2 _velocity;
    float _dir = 1.0f;
    float _lifeDistance = 0.0f; // traveled distance guard
};

#endif // __FK_SHOCKWAVE_H__
