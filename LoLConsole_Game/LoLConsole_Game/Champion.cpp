// Champion.cpp
#include "Champion.h"
#include <algorithm>
#include "SkillBehavior.h"   

Champion::Champion(std::string name, Team team, int id, ChampionArt art)
    : name_(std::move(name)), team_(team), id_(id), art_(std::move(art)) {
    
}
DrawResponse Champion::OnDrawRequest(const DrawRequest& req) const {
    if (req.kind == DrawRequest::Kind::Portrait || req.kind == DrawRequest::Kind::BattleSprite) {
        return { art_.sprite };
    }
    if (req.kind == DrawRequest::Kind::SkillEffect) {
        int k = std::clamp(req.variant, 0, (int)art_.skills.size() - 1);
        return { art_.skills[k] };
    }
    return { {} };
}

void Champion::SetSkill(int idx, std::unique_ptr<SkillBehavior> behavior) {
    if (idx < 0 || idx >= 4) return;
    skills_[idx] = std::move(behavior);
}
void Champion::SetStats(std::string name) {
    // 이즈리얼 maxhp 500, hp 500, atk 70, move_ms 120 Q cool 3000, W cool 4000 E cool 6000 R cool 10000
    if (name == "Ezreal") {
        stats_.hp = 500, stats_.max_hp = 500, stats_.atk = 70, stats_.move_ms = 120;
        cd_ms[0] = 3000, cd_ms[1] = 4000, cd_ms[2] = 6000, cd_ms[3] = 10000;
    }
    //다리우스  maxhp 700 hp 700 , atk 65 , move_ms 110 Q cool 4000, W cool 3000 E cool 6000 R cool 15000
    if (name == "Darius") {
        stats_.hp = 700, stats_.max_hp = 700, stats_.atk = 65, stats_.move_ms = 110;
        cd_ms[0] = 4000, cd_ms[1] = 3000, cd_ms[2] = 6000, cd_ms[3] = 15000;
    }
    //마스터이  maxhp 600 hp 600 , atk 65 , move_ms 100 Q cool 4000, W cool 8000 E cool 4000 R cool 15000
    if (name == "MasterE") {
        stats_.hp = 600, stats_.max_hp = 600, stats_.atk = 65, stats_.move_ms = 100;
        cd_ms[0] = 4000, cd_ms[1] = 8000, cd_ms[2] = 7000, cd_ms[3] = 50000;
    }
}
void Champion::Cast(int idx, BattleManager& world) {
    if (idx < 0 || idx >= 4) return;
    if (!skills_[idx]) return;

    // 쿨타임 확인
    if (!cd[idx].ready()) return;

    // 스킬 실행
    skills_[idx]->Cast(world, *this);

    // 쿨타임 설정: 전략이 CooldownMs 제공 → 우선
    int ms = skills_[idx]->CooldownMs(*this);
    if (ms <= 0) ms = cd_ms[idx];
    cd[idx].set(ms);
}
const std::vector<std::string>& Champion::SkillArt(int idx) const {

    static const std::vector<std::string> empty;

    if (idx < 0 || idx >= (int)art_.skills.size()) return empty;

    return art_.skills[idx];
}
