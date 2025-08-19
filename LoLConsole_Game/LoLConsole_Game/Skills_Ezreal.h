#pragma once
#include "SkillBehavior.h"

struct Ezreal_Q : SkillBehavior {
    int CooldownMs(const Champion&) const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};
struct Ezreal_W : SkillBehavior {
    int CooldownMs(const Champion&) const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};
struct Ezreal_E : SkillBehavior { // ����/���
    int CooldownMs(const Champion&) const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};
struct Ezreal_R : SkillBehavior { // ��Ÿ�
    int CooldownMs(const Champion&) const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};

