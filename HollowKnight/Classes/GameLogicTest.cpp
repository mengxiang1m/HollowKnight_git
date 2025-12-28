#include "gtest/gtest.h"
#include "Player.h"
#include "Enemy.h"
#include "Zombie.h"
#include "Buzzer.h"
#include "Boss.h"
#include "HelloWorldScene.h"

// 1. Player 关键逻辑测试
TEST(PlayerTest, HealthChange) {
    Player* player = Player::create("Knight/idle/idle_1.png");
    ASSERT_NE(player, nullptr);
    int oldHp = player->getHealth();
    player->takeDamage(1, cocos2d::Vec2(0,0), {});
    EXPECT_LT(player->getHealth(), oldHp);
}

TEST(PlayerTest, SoulGain) {
    Player* player = Player::create("Knight/idle/idle_1.png");
    ASSERT_NE(player, nullptr);
    int oldSoul = player->getStats()->getSoul();
    player->gainSoul(5);
    EXPECT_GT(player->getStats()->getSoul(), oldSoul);
}

// 2. Enemy 关键逻辑测试
TEST(EnemyTest, PatrolState) {
    Enemy* enemy = Enemy::create("enemies/enemy_walk_1.png");
    ASSERT_NE(enemy, nullptr);
    EXPECT_EQ(enemy->getCurrentState(), Enemy::State::PATROL);
}

TEST(EnemyTest, TakeDamageAndDeath) {
    Enemy* enemy = Enemy::create("enemies/enemy_walk_1.png");
    ASSERT_NE(enemy, nullptr);
    int oldHp = enemy->getHealth();
    enemy->takeDamage(1, cocos2d::Vec2(0,0));
    EXPECT_LT(enemy->getHealth(), oldHp);
    enemy->takeDamage(10, cocos2d::Vec2(0,0));
    EXPECT_EQ(enemy->getCurrentState(), Enemy::State::DEAD);
}

// 3. Zombie 逻辑测试
TEST(ZombieTest, PatrolRange) {
    Zombie* zombie = Zombie::create("zombie/walk/walk_1.png");
    ASSERT_NE(zombie, nullptr);
    zombie->setPatrolRange(100, 200);
    EXPECT_GE(zombie->getPatrolRightBound(), 200);
}

// 4. Buzzer 逻辑测试
TEST(BuzzerTest, Creation) {
    Buzzer* buzzer = Buzzer::create("buzzer/idle/idle_1.png");
    ASSERT_NE(buzzer, nullptr);
}

// 5. Boss 逻辑测试
TEST(BossTest, Creation) {
    Boss* boss = Boss::create(cocos2d::Vec2(0,0));
    ASSERT_NE(boss, nullptr);
}

// 6. 关卡切换与资源加载
TEST(SceneTest, MapLoadFail) {
    HelloWorld* scene = HelloWorld::create();
    ASSERT_NE(scene, nullptr);
    // 尝试加载不存在的地图
    scene->loadMap("maps/not_exist.tmx");
    // 期望不会崩溃，且有错误日志
}
