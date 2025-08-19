#pragma once
#include "SkillBehavior.h"

struct Darius_Q : SkillBehavior { // 근접 원형 휘두르기
    int CooldownMs(const Champion&) const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};
struct Darius_W : SkillBehavior {
    int CooldownMs(const Champion&)const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};
struct Darius_E : SkillBehavior { // 당기기(무기에 닿으면 적을 내 쪽으로 이동)
    int CooldownMs(const Champion&) const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};
struct Darius_R :SkillBehavior {
    int CooldownMs(const Champion&) const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};

