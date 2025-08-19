#pragma once
#include "Core.h"
#include "Draw.h"
#include <unordered_set>

// 공격 주체 종류(골드 지급/우군 피격 방지/통계 등에 사용)
enum class SourceKind { Champion, Minion };

struct HitEvent { Team attacker; Team victim; int amount; };

class Effect : public IDrawable {
public:
    Effect(std::vector<std::string> lineArt, Vec2 p, int vx, Team owner,
        int ttl_ms, int damage, bool pierce,
        bool pullsOnHit = false, int pullDist = 0,
        bool singleHitPerVictim = true,
        bool follow = false,                 // 따라다님 여부
        const Vec2* anchor = nullptr,        // 기준 좌표(예: 챔피언 위치 포인터)
        Vec2 offset = { 0,0 },                 // anchor에서의 오프셋
        SourceKind ownerKind = SourceKind::Champion, 
        int ownerId = 0)                                
        : art(std::move(lineArt)), pos(p), vx(vx), owner(owner),
        damage_(damage), pierce_(pierce), alive_(true),
        pullsOnHit_(pullsOnHit), pullDist_(pullDist),
        singleHitPerVictim_(singleHitPerVictim),
        follow_(follow), anchor_(anchor), offset_(offset),
        ownerKind_(ownerKind), ownerId_(ownerId)
    {
        ttl.set(ttl_ms);
    }

    void Update(int dt) {
        if (!alive_) return;

        // 따라다니기면 위치를 앵커+오프셋으로 고정, 아니면 일반 이동
        if (follow_ && anchor_) {
            pos.x = anchor_->x + offset_.x;
            pos.y = anchor_->y + offset_.y;
        }
        else {
            pos.x += vx;
        }

        ttl.tick(dt);
        if (ttl.ready()) alive_ = false;
        if (pos.x < 1 || pos.x > W - 2) alive_ = false;
    }

    // 대상당 1회 히트 보장(관통 스킬 중복 타 막기)
    bool TryHitOnce(int targetId) {
        if (!singleHitPerVictim_) return true;
        return hitVictims_.insert(targetId).second; // 처음 맞을 때만 true
    }

    // 조회/제어자
    bool isPierce() const { return pierce_; }
    bool Alive()    const { return alive_; }
    void Kill() { alive_ = false; }
    int  Damage()   const { return damage_; }
    Vec2 Pos()      const { return pos; }
    Team Owner()    const { return owner; }

    bool PullsOnHit() const { return pullsOnHit_; }
    int  PullDist()   const { return pullDist_; }
    bool IsFollow()   const { return follow_; }

    //발사 주체 정보
    SourceKind OwnerKind() const { return ownerKind_; }
    int        OwnerId()   const { return ownerId_; }

    DrawResponse OnDrawRequest(const DrawRequest& req) const override {
        if (req.kind == DrawRequest::Kind::SkillEffect) return { art };
        return { {} };
    }
    //적 AI로직에 필요
    int Vx() const { return vx; }

private:
    std::vector<std::string> art;
    Vec2   pos{};
    int    vx{};
    Team   owner{};
    Timer  ttl{};

    int    damage_{ 10 };
    bool   pierce_{ false };
    bool   alive_{ true };

    // 동작 옵션
    bool        follow_{ false };
    const Vec2* anchor_{ nullptr };
    Vec2        offset_{ 0,0 };
    bool   singleHitPerVictim_{ true };
    std::unordered_set<int> hitVictims_;

    bool   pullsOnHit_{ false };
    int    pullDist_{ 0 };

    // 주체 태그
    SourceKind ownerKind_{ SourceKind::Champion };
    int        ownerId_{ 0 };
};
