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

class HelloWorld : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();

    // 【新增】游戏容器层 (所有移动的东西)
    cocos2d::Layer* _gameLayer;

    virtual bool init();

    virtual void update(float dt) override;

    // 这是默认的关闭按钮回调，用于退出游戏
    void menuCloseCallback(cocos2d::Ref* pSender);

    // 这一行宏是 cocos 必须的    
    CREATE_FUNC(HelloWorld);

private: 
    Player* _player; 

    //存地图里所有的地面矩形框
    std::vector<cocos2d::Rect> _groundRects;

    // 解析地图碰撞框的辅助函数
    void parseMapCollisions(cocos2d::TMXTiledMap* map);

    //键盘状态标志位
    bool _isLeftPressed = false;
    bool _isRightPressed = false;
	bool _isUpPressed = false;
	bool _isDownPressed = false;

    // 辅助函数：根据当前按键状态更新主角速度
    void updatePlayerMovement();
};

#endif // __HELLOWORLD_SCENE_H__