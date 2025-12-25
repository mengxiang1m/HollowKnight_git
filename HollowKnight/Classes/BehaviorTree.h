#ifndef __BEHAVIOR_TREE_H__
#define __BEHAVIOR_TREE_H__

#include "cocos2d.h"
#include <vector>
#include <memory>
#include <functional>

// ============================================================
// 行为树节点状态
// ============================================================
enum class BTNodeStatus
{
    SUCCESS,    // 成功
    FAILURE,    // 失败
    RUNNING     // 运行中
};

// ============================================================
// 黑板系统 - 用于共享数据
// ============================================================
class Blackboard
{
public:
    void setBool(const std::string& key, bool value) { _bools[key] = value; }
    void setInt(const std::string& key, int value) { _ints[key] = value; }
    void setFloat(const std::string& key, float value) { _floats[key] = value; }
    void setVec2(const std::string& key, const cocos2d::Vec2& value) { _vec2s[key] = value; }
    
    bool getBool(const std::string& key, bool defaultValue = false) const {
        auto it = _bools.find(key);
        return it != _bools.end() ? it->second : defaultValue;
    }
    
    int getInt(const std::string& key, int defaultValue = 0) const {
        auto it = _ints.find(key);
        return it != _ints.end() ? it->second : defaultValue;
    }
    
    float getFloat(const std::string& key, float defaultValue = 0.0f) const {
        auto it = _floats.find(key);
        return it != _floats.end() ? it->second : defaultValue;
    }
    
    cocos2d::Vec2 getVec2(const std::string& key, const cocos2d::Vec2& defaultValue = cocos2d::Vec2::ZERO) const {
        auto it = _vec2s.find(key);
        return it != _vec2s.end() ? it->second : defaultValue;
    }
    
private:
    std::map<std::string, bool> _bools;
    std::map<std::string, int> _ints;
    std::map<std::string, float> _floats;
    std::map<std::string, cocos2d::Vec2> _vec2s;
};

// ============================================================
// 行为树节点基类
// ============================================================
class BTNode
{
public:
    virtual ~BTNode() {}
    virtual BTNodeStatus tick(float dt, Blackboard& blackboard) = 0;
    virtual void reset() {}
};

// ============================================================
// 复合节点基类
// ============================================================
class BTComposite : public BTNode
{
public:
    void addChild(std::shared_ptr<BTNode> child) {
        _children.push_back(child);
    }
    
    virtual void reset() override {
        for (auto& child : _children) {
            child->reset();
        }
    }
    
protected:
    std::vector<std::shared_ptr<BTNode>> _children;
};

// ============================================================
// 选择节点 (Selector) - 遇到成功就返回，全失败才失败
// ============================================================
class BTSelector : public BTComposite
{
public:
    virtual BTNodeStatus tick(float dt, Blackboard& blackboard) override {
        for (auto& child : _children) {
            BTNodeStatus status = child->tick(dt, blackboard);
            if (status != BTNodeStatus::FAILURE) {
                return status;  // SUCCESS 或 RUNNING
            }
        }
        return BTNodeStatus::FAILURE;
    }
};

// ============================================================
// 序列节点 (Sequence) - 遇到失败就返回，全成功才成功
// ============================================================
class BTSequence : public BTComposite
{
public:
    virtual BTNodeStatus tick(float dt, Blackboard& blackboard) override {
        for (auto& child : _children) {
            BTNodeStatus status = child->tick(dt, blackboard);
            if (status != BTNodeStatus::SUCCESS) {
                return status;  // FAILURE 或 RUNNING
            }
        }
        return BTNodeStatus::SUCCESS;
    }
};

// ============================================================
// 装饰器节点基类
// ============================================================
class BTDecorator : public BTNode
{
public:
    BTDecorator(std::shared_ptr<BTNode> child) : _child(child) {}
    
    virtual void reset() override {
        if (_child) _child->reset();
    }
    
protected:
    std::shared_ptr<BTNode> _child;
};

// ============================================================
// 反转节点 (Inverter) - 成功变失败，失败变成功
// ============================================================
class BTInverter : public BTDecorator
{
public:
    BTInverter(std::shared_ptr<BTNode> child) : BTDecorator(child) {}
    
    virtual BTNodeStatus tick(float dt, Blackboard& blackboard) override {
        BTNodeStatus status = _child->tick(dt, blackboard);
        if (status == BTNodeStatus::SUCCESS) return BTNodeStatus::FAILURE;
        if (status == BTNodeStatus::FAILURE) return BTNodeStatus::SUCCESS;
        return status;
    }
};

// ============================================================
// 重复节点 (Repeater) - 重复执行直到失败或达到次数
// ============================================================
class BTRepeater : public BTDecorator
{
public:
    BTRepeater(std::shared_ptr<BTNode> child, int count = -1) 
        : BTDecorator(child), _repeatCount(count), _currentCount(0) {}
    
    virtual BTNodeStatus tick(float dt, Blackboard& blackboard) override {
        if (_repeatCount > 0 && _currentCount >= _repeatCount) {
            _currentCount = 0;
            return BTNodeStatus::SUCCESS;
        }
        
        BTNodeStatus status = _child->tick(dt, blackboard);
        if (status == BTNodeStatus::SUCCESS) {
            _currentCount++;
            if (_repeatCount > 0 && _currentCount >= _repeatCount) {
                _currentCount = 0;
                return BTNodeStatus::SUCCESS;
            }
            return BTNodeStatus::RUNNING;
        }
        
        return status;
    }
    
    virtual void reset() override {
        BTDecorator::reset();
        _currentCount = 0;
    }
    
private:
    int _repeatCount;
    int _currentCount;
};

// ============================================================
// 条件节点 - 使用函数判断
// ============================================================
class BTCondition : public BTNode
{
public:
    using ConditionFunc = std::function<bool(Blackboard&)>;
    
    BTCondition(ConditionFunc func) : _condition(func) {}
    
    virtual BTNodeStatus tick(float dt, Blackboard& blackboard) override {
        return _condition(blackboard) ? BTNodeStatus::SUCCESS : BTNodeStatus::FAILURE;
    }
    
private:
    ConditionFunc _condition;
};

// ============================================================
// 动作节点 - 执行具体行为
// ============================================================
class BTAction : public BTNode
{
public:
    using ActionFunc = std::function<BTNodeStatus(float, Blackboard&)>;
    
    BTAction(ActionFunc func) : _action(func) {}
    
    virtual BTNodeStatus tick(float dt, Blackboard& blackboard) override {
        return _action(dt, blackboard);
    }
    
private:
    ActionFunc _action;
};

// ============================================================
// 等待节点 - 等待一段时间
// ============================================================
class BTWait : public BTNode
{
public:
    BTWait(float duration) : _duration(duration), _elapsed(0.0f) {}
    
    virtual BTNodeStatus tick(float dt, Blackboard& blackboard) override {
        _elapsed += dt;
        if (_elapsed >= _duration) {
            _elapsed = 0.0f;
            return BTNodeStatus::SUCCESS;
        }
        return BTNodeStatus::RUNNING;
    }
    
    virtual void reset() override {
        _elapsed = 0.0f;
    }
    
private:
    float _duration;
    float _elapsed;
};

// ============================================================
// 随机选择器 - 随机选择一个子节点执行
// ============================================================
class BTRandomSelector : public BTComposite
{
public:
    BTRandomSelector() : _selectedIndex(-1) {}
    
    virtual BTNodeStatus tick(float dt, Blackboard& blackboard) override {
        if (_children.empty()) return BTNodeStatus::FAILURE;
        
        if (_selectedIndex < 0) {
            _selectedIndex = rand() % _children.size();
        }
        
        BTNodeStatus status = _children[_selectedIndex]->tick(dt, blackboard);
        
        if (status != BTNodeStatus::RUNNING) {
            _selectedIndex = -1;  // 重置选择
        }
        
        return status;
    }
    
    virtual void reset() override {
        BTComposite::reset();
        _selectedIndex = -1;
    }
    
private:
    int _selectedIndex;
};

// ============================================================
// 权重随机选择器 - 根据权重随机选择
// ============================================================
class BTWeightedRandomSelector : public BTComposite
{
public:
    void addChildWithWeight(std::shared_ptr<BTNode> child, float weight) {
        _children.push_back(child);
        _weights.push_back(weight);
        _totalWeight += weight;
    }
    
    virtual BTNodeStatus tick(float dt, Blackboard& blackboard) override {
        if (_children.empty()) return BTNodeStatus::FAILURE;
        
        if (_selectedIndex < 0) {
            // 根据权重随机选择
            float randomValue = (float)(rand() % 10000) / 10000.0f * _totalWeight;
            float currentWeight = 0.0f;
            
            for (size_t i = 0; i < _weights.size(); i++) {
                currentWeight += _weights[i];
                if (randomValue <= currentWeight) {
                    _selectedIndex = (int)i;
                    break;
                }
            }
            
            if (_selectedIndex < 0) _selectedIndex = 0;
        }
        
        BTNodeStatus status = _children[_selectedIndex]->tick(dt, blackboard);
        
        if (status != BTNodeStatus::RUNNING) {
            _selectedIndex = -1;  // 重置选择
        }
        
        return status;
    }
    
    virtual void reset() override {
        BTComposite::reset();
        _selectedIndex = -1;
    }
    
private:
    std::vector<float> _weights;
    float _totalWeight = 0.0f;
    int _selectedIndex = -1;
};

#endif // __BEHAVIOR_TREE_H__
