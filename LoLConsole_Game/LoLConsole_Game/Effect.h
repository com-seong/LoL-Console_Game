#pragma once
#include "Core.h"
#include "Draw.h"
#include <unordered_set>

// ���� ��ü ����(��� ����/�챺 �ǰ� ����/��� � ���)
enum class SourceKind { Champion, Minion };

struct HitEvent { Team attacker; Team victim; int amount; };

class Effect : public IDrawable {
public:
    Effect(std::vector<std::string> lineArt, Vec2 p, int vx, Team owner,
        int ttl_ms, int damage, bool pierce,
        bool pullsOnHit = false, int pullDist = 0,
        bool singleHitPerVictim = true,
        bool follow = false,                 // ����ٴ� ����
        const Vec2* anchor = nullptr,        // ���� ��ǥ(��: è�Ǿ� ��ġ ������)
        Vec2 offset = { 0,0 },                 // anchor������ ������
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

        // ����ٴϱ�� ��ġ�� ��Ŀ+���������� ����, �ƴϸ� �Ϲ� �̵�
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

    // ���� 1ȸ ��Ʈ ����(���� ��ų �ߺ� Ÿ ����)
    bool TryHitOnce(int targetId) {
        if (!singleHitPerVictim_) return true;
        return hitVictims_.insert(targetId).second; // ó�� ���� ���� true
    }

    // ��ȸ/������
    bool isPierce() const { return pierce_; }
    bool Alive()    const { return alive_; }
    void Kill() { alive_ = false; }
    int  Damage()   const { return damage_; }
    Vec2 Pos()      const { return pos; }
    Team Owner()    const { return owner; }

    bool PullsOnHit() const { return pullsOnHit_; }
    int  PullDist()   const { return pullDist_; }
    bool IsFollow()   const { return follow_; }

    //�߻� ��ü ����
    SourceKind OwnerKind() const { return ownerKind_; }
    int        OwnerId()   const { return ownerId_; }

    DrawResponse OnDrawRequest(const DrawRequest& req) const override {
        if (req.kind == DrawRequest::Kind::SkillEffect) return { art };
        return { {} };
    }
    //�� AI������ �ʿ�
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

    // ���� �ɼ�
    bool        follow_{ false };
    const Vec2* anchor_{ nullptr };
    Vec2        offset_{ 0,0 };
    bool   singleHitPerVictim_{ true };
    std::unordered_set<int> hitVictims_;

    bool   pullsOnHit_{ false };
    int    pullDist_{ 0 };

    // ��ü �±�
    SourceKind ownerKind_{ SourceKind::Champion };
    int        ownerId_{ 0 };
};
