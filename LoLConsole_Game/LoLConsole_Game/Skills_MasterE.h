#pragma once
#include "SkillBehavior.h"

struct MasterE_Q : SkillBehavior { // 근접 원형 휘두르기
    int CooldownMs(const Champion&) const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};
struct MasterE_W : SkillBehavior {
    int CooldownMs(const Champion&)const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};
struct MasterE_E : SkillBehavior { // 당기기(무기에 닿으면 적을 내 쪽으로 이동)
    int CooldownMs(const Champion&) const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};
struct MasterE_R :SkillBehavior {
    int CooldownMs(const Champion&) const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};


