#pragma once
#include "SkillBehavior.h"

struct MasterE_Q : SkillBehavior { // ���� ���� �ֵθ���
    int CooldownMs(const Champion&) const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};
struct MasterE_W : SkillBehavior {
    int CooldownMs(const Champion&)const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};
struct MasterE_E : SkillBehavior { // ����(���⿡ ������ ���� �� ������ �̵�)
    int CooldownMs(const Champion&) const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};
struct MasterE_R :SkillBehavior {
    int CooldownMs(const Champion&) const override { return 0; }
    void Cast(BattleManager& world, Champion& self) override;
};


