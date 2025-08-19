#pragma once
class BattleManager;
class Champion;

struct SkillBehavior {
    virtual ~SkillBehavior() = default;
    virtual int  CooldownMs(const Champion& self) const = 0;  //è�Ǿ𿡰� ��ٿ� ���� ���� è�Ǿ��� �� ���� �޾�
                                                              //�ڽ��� Ÿ�̸ӿ� ����
    virtual void Cast(BattleManager& world, Champion& self) = 0;
};

