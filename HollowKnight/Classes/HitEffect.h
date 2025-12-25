#pragma once
#include "cocos2d.h"
#include <string>

class HitEffect : public cocos2d::Node {
public:
    // 播放受击特效（动画三帧）
    // parent: 父节点
    // center: 特效中心点（世界坐标）
    // size: 期望特效宽高
    // duration: 总持续时间
    static void play(cocos2d::Node* parent, const cocos2d::Vec2& center, float size, float duration = 0.18f);
};
