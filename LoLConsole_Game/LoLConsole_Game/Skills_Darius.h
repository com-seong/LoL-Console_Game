#pragma once
#include "SkillBehavior.h"

struct Darius_Q : SkillBehavior { // ���� ���� �ֵθ���
    int CooldownMs(const Champion&) const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};
struct Darius_W : SkillBehavior {
    int CooldownMs(const Champion&)const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};
struct Darius_E : SkillBehavior { // ����(���⿡ ������ ���� �� ������ �̵�)
    int CooldownMs(const Champion&) const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};
struct Darius_R :SkillBehavior {
    int CooldownMs(const Champion&) const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};

