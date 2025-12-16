#ifndef __FIREBALL_H__
#define __FIREBALL_H__

#include "cocos2d.h"

USING_NS_CC;

class Fireball : public Sprite
{
public:
    static Fireball* create(const std::string& firstFrame);
    
    virtual bool init(const std::string& firstFrame);
    
    // 播放循环动画
    void playAnimation();
    
private:
    // 动画相关
    void loadAnimation();
};

#endif // __FIREBALL_H__
