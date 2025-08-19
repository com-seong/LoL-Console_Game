#pragma once
class BattleManager;
class Champion;

struct SkillBehavior {
    virtual ~SkillBehavior() = default;
    virtual int  CooldownMs(const Champion& self) const = 0;  //챔피언에게 쿨다운 값을 제공 챔피언은 그 값을 받아
                                                              //자신의 타이머에 적용
    virtual void Cast(BattleManager& world, Champion& self) = 0;
};

