#ifndef __KEY_BINDING_SCENE_H__
#define __KEY_BINDING_SCENE_H__

#include "cocos2d.h"
#include <map>
#include <string>

USING_NS_CC;

// 键位绑定管理器（单例）
class KeyBindingManager
{
public:
    static KeyBindingManager* getInstance();
    
    // 动作枚举
    enum class Action
    {
        MOVE_UP,
        MOVE_DOWN,
        MOVE_LEFT,
        MOVE_RIGHT,
        JUMP,
        ATTACK,
        FOCUS,
        DASH
    };
    
    // 获取某个动作对应的按键
    EventKeyboard::KeyCode getKeyForAction(Action action) const;
    
    // 设置某个动作的按键
    void setKeyForAction(Action action, EventKeyboard::KeyCode key);
    
    // 获取按键的显示名称
    static std::string getKeyName(EventKeyboard::KeyCode key);
    
    // 获取动作的中文名称
    static std::string getActionName(Action action);
    
    // 保存和加载配置
    void saveToFile();
    void loadFromFile();
    
    // 重置为默认键位
    void resetToDefault();
    
private:
    KeyBindingManager();
    ~KeyBindingManager() = default;
    
    static KeyBindingManager* s_instance;
    std::map<Action, EventKeyboard::KeyCode> _keyBindings;
    
    void initDefaultKeys();
};

// 键位配置场景
class KeyBindingScene : public Scene
{
public:
    static Scene* createScene();
    
    virtual bool init() override;
    
    CREATE_FUNC(KeyBindingScene);
    
private:
    // UI 元素
    Label* _titleLabel;
    std::vector<Label*> _actionLabels;      // 左侧动作名称
    std::vector<Label*> _keyLabels;         // 右侧按键显示
    Sprite* _selector;                       // 选择光标
    
    // 状态
    int _currentSelection;                   // 当前选中项 (0-7)
    bool _isWaitingForKey;                   // 是否正在等待按键输入
    KeyBindingManager::Action _selectedAction; // 当前选中的动作
    
    // 按键映射（用于界面显示）
    std::vector<KeyBindingManager::Action> _actionOrder;
    
    // 方法
    void createUI();
    void createTitle();
    void createActionList();
    void createButtons();
    void createInstructions();
    void updateKeyLabels();
    void updateSelector();
    
    // 输入处理
    void setupKeyboardListener();
    void onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event);
    void onKeyReleased(EventKeyboard::KeyCode keyCode, Event* event);
    
    // 选择处理
    void moveSelectionUp();
    void moveSelectionDown();
    void confirmSelection();
    void startGame();
    void resetKeys();
};

#endif // __KEY_BINDING_SCENE_H__
