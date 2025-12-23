#include "HitEffect.h"

USING_NS_CC;

void HitEffect::play(Node* parent, const Vec2& center, float size, float duration) {
    // 三帧动画，水平顺序：hit_crack0, hit_crack1, hit_crack2
    Vector<SpriteFrame*> frames;
    for (int i = 0; i < 3; ++i) {
        std::string tex = "hit_crack/hit_crack" + std::to_string(i) + ".png";
        auto sprite = Sprite::create(tex);
        if (sprite) {
            auto frame = sprite->getSpriteFrame();
            frames.pushBack(frame);
        }
    }
    if (frames.empty()) return;
    float base = 200.0f;
    float scale = size / base;
    auto effect = Sprite::createWithSpriteFrame(frames.at(0));
    effect->setPosition(center);
    effect->setScale(scale);
    effect->setOpacity(210);
    parent->addChild(effect, 99);
    // 帧动画
    float frameDur = duration / 3.0f;
    auto animation = Animation::createWithSpriteFrames(frames, frameDur);
    auto animate = Animate::create(animation);
    // 淡出
    auto fade = FadeOut::create(frameDur);
    auto seq = Sequence::create(animate, fade, RemoveSelf::create(), nullptr);
    effect->runAction(seq);
}
