#include "PlayerStats.h"

USING_NS_CC;

PlayerStats::PlayerStats() : _health(0), _soul(0) {}

void PlayerStats::initStats(const Config::PlayerStatConfig& config)
{
    _config = config; // 保存配置

    //以此初始化状态
    _health = _config.maxHealth;
    _soul = 0;
}

void PlayerStats::reset()
{
    _health = _config.maxHealth;
    _soul = 0;
    if (onHealthChanged) onHealthChanged(_health, _config.maxHealth);
    if (onSoulChanged) onSoulChanged(_soul);
}

bool PlayerStats::canFocus() const
{
    return (_soul >= _config.healCost) && (_health < _config.maxHealth);
}

// 攻击回魂：使用配置里的数值
void PlayerStats::gainSoulOnKill()
{
    // 读取配置：杀怪回多少
    gainSoul(_config.soulGainPerKill);
}

void PlayerStats::gainSoul(int amount)
{
    _soul += amount;
    if (_soul > _config.maxSoul) _soul = _config.maxSoul; // 使用配置限制
    if (_soul < 0) _soul = 0;

    // 通知 UI 更新
    if (onSoulChanged) onSoulChanged(_soul);
}

bool PlayerStats::recoverHealth()
{
    if (!canFocus()) return false;

    gainSoul(-_config.healCost);

    _health++;
    if (_health > _config.maxHealth) _health = _config.maxHealth;

    if (onHealthChanged) onHealthChanged(_health, _config.maxHealth);
    return true;
}

void PlayerStats::takeDamage(int dmg)
{
    // 注意：受伤的数值通常来自"外部"（敌人的攻击力），而不是Player自己的Config
    if (_health <= 0) return;

    _health -= dmg;
    if (_health < 0) _health = 0;

    if (onHealthChanged) onHealthChanged(_health, _config.maxHealth);
}