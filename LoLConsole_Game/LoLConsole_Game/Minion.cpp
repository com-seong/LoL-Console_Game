// Minion.cpp
#include "Minion.h"

static const char* IMG_SHORT[2] = { "  0","<===>" };
static const char* SKILL_IMG_SHORT[1] = { "))" };

static const char* IMG_LONG[2] = { "  *","<--->" };
static const char* SKILL_IMG_LONG[1] = { "¡Ú" };

Minion::Minion(bool isRanged_, Team t, int id_, Vec2 p)
    : isRanged_(isRanged_), team_(t), id_(id_), pos_(p) {
    SetMs(isRanged_);
}
DrawResponse Minion::OnDrawRequest(const DrawRequest& req) const {
    if (req.kind == DrawRequest::Kind::BattleSprite || req.kind == DrawRequest::Kind::Portrait) {
        std::vector<std::string> v; auto arr = isRanged_ ? IMG_LONG : IMG_SHORT;
        v.emplace_back(arr[0]); v.emplace_back(arr[1]); return { v };
    }
    if (req.kind == DrawRequest::Kind::SkillEffect) {
        std::vector<std::string> v; auto arr = isRanged_ ? SKILL_IMG_LONG : SKILL_IMG_SHORT;
        v.emplace_back(arr[0]); return { v };
    }
    return { {} };
}
void Minion::SetMs(bool ranged) {
    if (ranged) {
        m_stats.hp = 40;
        m_stats.damage = 7;
    }
    else {
        m_stats.hp = 60;
        m_stats.damage = 5;
    }
}