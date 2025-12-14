#ifndef __HUD_LAYER_H__
#define __HUD_LAYER_H__

#include "cocos2d.h"
#include <vector>

class HUDLayer : public cocos2d::Layer
{
public:
    static HUDLayer* createLayer(); // 自定义创建函数
    virtual bool init() override;

    // 核心接口：更新血量显示
    // currentHp: 当前血量, maxHp: 最大血量
    void updateHealth(int currentHp, int maxHp);

    CREATE_FUNC(HUDLayer);

private:
    // 存储所有的血量图标精灵
    std::vector<cocos2d::Sprite*> _heartSprites;

    // UI 容器节点 (方便整体移动)
    cocos2d::Node* _healthBarContainer;
};

#endif // __HUD_LAYER_H__