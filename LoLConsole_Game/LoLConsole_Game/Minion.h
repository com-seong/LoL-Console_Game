// Minion.h
#pragma once
#include "Core.h"
#include "Draw.h"
#include <string>

class Minion : public IDrawable {
public:
    
    Minion(bool isRanged, Team t, int id, Vec2 p);

    int  Id() const { return id_; }

    void Kill() { alive_ = false; }
    bool Alive() const { return alive_; }

    void SetPos(const Vec2& p) { pos_ = p; }
    void SetPos(int x, int y) { pos_.x = x; pos_.y = y; }
    Vec2& Pos() { return pos_; }
    Team GetTeam() const { return team_; }

    // “원거리 유닛인가?”
    bool IsRanged() const { return isRanged_; }

    // 능력치
    MStats& MS() { return m_stats; }
    const MStats& MS() const { return m_stats; }

    // 초기 스탯 세팅(인자로 ranged 여부)
    void SetMs(bool ranged);

    // 그림 요청
    DrawResponse OnDrawRequest(const DrawRequest& req) const override;

    int width()  const { return 5; }
    int height() const { return 2; }

private:
    bool   isRanged_;         
    Team   team_;
    int    id_;
    bool   alive_{ true };
    Vec2   pos_;

    MStats m_stats{};
};
