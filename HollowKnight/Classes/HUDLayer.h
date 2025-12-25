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

	// 核心接口：更新灵魂显示
    void updateSoul(int currentSoul); 

    CREATE_FUNC(HUDLayer);

private:
	int _lastSoul = -1; // 记录上一次的灵魂值，避免重复更新
    // 存储所有的血量图标精灵
    std::vector<cocos2d::Sprite*> _heartSprites;


    // UI 容器节点 (方便整体移动)
    cocos2d::Node* _healthBarContainer;

    // ==========================================
    // 【核心修改】拆分为两个精灵
    // ==========================================
    cocos2d::Sprite* _soulOrb;   // 下层：液体 (负责动画)
    cocos2d::Sprite* _soulFrame; // 上层：框 (静止不动)

    // 播放破碎动画
    void playBreakAnimation(cocos2d::Sprite* heartSprite);

    // 根据灵魂值创建 "动画+等待" 的循环动作
    cocos2d::Action* createSoulAction(int soulValue);

    // ==========================================
    // 【新增】开场动画相关
    // ==========================================
    void playOpeningSequence();       // 启动整个流程
    void spawnNextHealth(int index, int maxHp); // 递归生成血条
};

#endif // __HUD_LAYER_H__