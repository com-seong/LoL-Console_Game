// Champion.h
#pragma once
#include "Core.h"
#include "Draw.h"
#include <vector>
#include <array>
#include <memory>
#include <string>

// ���漱��(��ȯ ���� ����)
class BattleManager;
struct SkillBehavior;

struct ChampionArt {
    std::vector<std::string> sprite;                 // è�Ǿ� �̹���
    std::vector<std::vector<std::string>> skills;    // Q,W,E,R �̹���(���� ���� ��)
};

class Champion : public IDrawable {
public:
    Champion(std::string name, Team team, int id, ChampionArt art);

    // Portrait / BattleSprite ��û���� sprite, SkillEffect ��û���� skills[idx]
    DrawResponse OnDrawRequest(const DrawRequest& req) const override;

    // ����
    const std::string& name() const { return name_; }
    Team team()  const { return team_; }
    int  id()    const { return id_; }

    Stats& S() { return stats_; }

    const Stats& S() const { return stats_; }
    //�� è�Ǿ� �´� �ɷ�ġ �ο�
    void SetStats(std::string name);

    // ��ų ���� ����/���
    void SetSkill(int idx, std::unique_ptr<SkillBehavior> behavior); // idx: 0=Q,1=W,2=E,3=R
    
    void Cast(int idx, BattleManager& world);                        // ��Ÿ�� üũ + ����

    // ��ų ��Ʈ(���� �������� ���)
    const std::vector<std::string>& SkillArt(int idx) const;

    // ��Ÿ�� (Q/W/E/R). �⺻��, ������ CooldownMs�� �����ϸ� �� ���� ��
    Timer cd[4];
    int cd_ms[4]{ 0,0,0,0 };

private:
    std::string  name_;
    Team         team_;
    int          id_;
    Stats        stats_{};
    ChampionArt  art_;

    // Q, W, E, R
    std::array<std::unique_ptr<SkillBehavior>, 4> skills_;
};
