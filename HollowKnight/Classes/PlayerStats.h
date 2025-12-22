#ifndef __PLAYER_STATS_H__
#define __PLAYER_STATS_H__

#include "cocos2d.h"
#include <functional>
#include "Config.h" // 这里包含是为了识别结构体定义，这属于"数据契约"，是合理的耦合

class PlayerStats
{
public:
    PlayerStats();

    // 【核心变化】初始化时，传入一个配置结构体
    void initStats(const Config::PlayerStatConfig& config);

    // 核心操作
    void takeDamage(int dmg);
    bool recoverHealth();
    void gainSoulOnKill(); // 专门用于攻击回魂
    void gainSoul(int amount); // 通用加减
    void reset();

    // --- 查询 ---
    bool isDead() const { return _health <= 0; }
    bool canFocus() const; // 判断条件

    // Getters
    int getHealth() const { return _health; }
    int getSoul() const { return _soul; }

    // 方便外部查询
    int getMaxHealth() const { return _config.maxHealth; }
    int getMaxSoul() const { return _config.maxSoul; }

    // Callbacks
    std::function<void(int, int)> onHealthChanged;
    std::function<void(int)> onSoulChanged;

private:
    // 【关键】持有配置数据
    Config::PlayerStatConfig _config;

    // 动态状态 (会变的)
    int _health;
    int _soul;
};

#endif