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

    // �����Ÿ� �����ΰ�?��
    bool IsRanged() const { return isRanged_; }

    // �ɷ�ġ
    MStats& MS() { return m_stats; }
    const MStats& MS() const { return m_stats; }

    // �ʱ� ���� ����(���ڷ� ranged ����)
    void SetMs(bool ranged);

    // �׸� ��û
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
