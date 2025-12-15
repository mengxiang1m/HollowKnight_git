#ifndef __JAR_H__
#define __JAR_H__

#include "cocos2d.h"

USING_NS_CC;

class Jar : public Node
{
public:
    static Jar* create(const std::string& jarImage, const Vec2& position);
    
    virtual bool init(const std::string& jarImage, const Vec2& position);
    
    // 获取碰撞箱
    Rect getCollisionBox() const;
    
    // 【新增】获取顶部平台碰撞箱（用于站立）
    Rect getTopPlatformBox() const;
    
    // 受到攻击
    void takeDamage();
    
    // 检查是否已被摧毁
    bool isDestroyed() const { return _isDestroyed; }
    
private:
    Sprite* _jarSprite;      // 罐子精灵
    Sprite* _grubSprite;     // 幼虫精灵
    bool _isDestroyed;       // 是否已被摧毁
    
    // 播放幼虫附着动画（循环）
    void playGrubAttachAnimation();
    
    // 播放幼虫释放动画（一次）
    void playGrubFreeAnimation();
    
    // 罐子破碎后的清理
    void onJarBroken();
};

#endif // __JAR_H__
