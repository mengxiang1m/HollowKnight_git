/****************************************************************************
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
 
 http://www.cocos2d-x.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"
#include "Player.h"
#include "Spike.h"

class HelloWorld : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();

    // 游戏容器层（所有游戏对象的动态）
    cocos2d::Layer* _gameLayer;

    virtual bool init();

    virtual void update(float dt) override;

    // 处理默认的关闭按钮回调，退出游戏
    void menuCloseCallback(cocos2d::Ref* pSender);

    // 和一系列宏和 cocos 引擎相关    
    CREATE_FUNC(HelloWorld);

private: 
    Player* _player; 

    //地图切片中的地面碰撞框
    std::vector<cocos2d::Rect> _groundRects;

    // 【新增】罐子列表
    std::vector<class Jar*> _jars;

    // 解析地图碰撞框的辅助函数
    void parseMapCollisions(cocos2d::TMXTiledMap* map);

    //输入状态标志位
    bool _isLeftPressed = false;
    bool _isRightPressed = false;
	bool _isUpPressed = false;
	bool _isDownPressed = false;

    // 根据输入更新玩家移动速度
    void updatePlayerMovement();

    // 【新增】坐标显示标签
    cocos2d::Label* _coordLabel;
    
    // 【新增】调试用DrawNode
    cocos2d::DrawNode* _debugDrawNode;
    
    // 【新增】Spike调试信息标签
    cocos2d::Label* _spikeDebugLabel;

    // ========================================
    // 【新增】场景切换相关成员变量
    // ========================================
    int _currentLevel = 1;           // 当前关卡编号
    bool _isTransitioning = false;   // 是否正在场景切换

    // ========================================
    // 【新增】场景切换方法
    // ========================================
    void loadMap(const std::string& mapPath);  // 加载地图
    void switchToLevel2();                      // 切换到level2
};

#endif // __HELLOWORLD_SCENE_H__