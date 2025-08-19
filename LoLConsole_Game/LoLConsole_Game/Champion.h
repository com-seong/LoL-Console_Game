// Champion.h
#pragma once
#include "Core.h"
#include "Draw.h"
#include <vector>
#include <array>
#include <memory>
#include <string>

// 전방선언(순환 의존 방지)
class BattleManager;
struct SkillBehavior;

struct ChampionArt {
    std::vector<std::string> sprite;                 // 챔피언 이미지
    std::vector<std::vector<std::string>> skills;    // Q,W,E,R 이미지(각각 여러 줄)
};

class Champion : public IDrawable {
public:
    Champion(std::string name, Team team, int id, ChampionArt art);

    // Portrait / BattleSprite 요청에는 sprite, SkillEffect 요청에는 skills[idx]
    DrawResponse OnDrawRequest(const DrawRequest& req) const override;

    // 상태
    const std::string& name() const { return name_; }
    Team team()  const { return team_; }
    int  id()    const { return id_; }

    Stats& S() { return stats_; }

    const Stats& S() const { return stats_; }
    //각 챔피언에 맞는 능력치 부여
    void SetStats(std::string name);

    // 스킬 전략 장착/사용
    void SetSkill(int idx, std::unique_ptr<SkillBehavior> behavior); // idx: 0=Q,1=W,2=E,3=R
    
    void Cast(int idx, BattleManager& world);                        // 쿨타임 체크 + 실행

    // 스킬 아트(전략 구현에서 사용)
    const std::vector<std::string>& SkillArt(int idx) const;

    // 쿨타임 (Q/W/E/R). 기본값, 전략이 CooldownMs를 제공하면 그 값을 씀
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
