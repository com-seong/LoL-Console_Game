#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include <chrono>

//가로 156
constexpr int W = 156;
//세로 41
constexpr int H = 41;
//블루팀 레드팀 구분
enum class Team { Blue, Red };
//좌표 
struct Vec2 { int x = 0, y = 0; };

//이걸 챔피언 별로 다르게 기본값을 세팅하도록 해야됨. move_ms == // 낮을수록 빠름(틱 간격)
//능력치
struct Stats {
    int max_hp;
    int hp;
    int atk;
    int move_ms;   
};
struct MStats {
    int hp;
    int damage;
};
struct Timer {
    int ms = 0;
    void set(int v) {
        ms = v; 
    }
    void tick(int dt) { 
        if (ms > 0) { 
             ms -= dt; 
            if (ms < 0) ms = 0;
        } 
    }
    bool ready() const { return ms == 0; }
};
//스프라이트 반전 시키기
static char mirrorChar(char c) {
    switch (c) {
    case '(': return ')'; case ')': return '(';
    case '[': return ']'; case ']': return '[';
    case '{': return '}'; case '}': return '{';
    case '<': return '>'; case '>': return '<';
    case '/': return '\\'; case '\\': return '/';
    default: return c;
    }
}
static std::vector<std::string> mirrorAscii(const std::vector<std::string>& img) {
    // 줄 길이가 제각각이면 실루엣 보존을 위해 우측 패딩 후 뒤집기
    size_t maxw = 0;
    for (auto& s : img) if (s.size() > maxw) maxw = s.size();

    std::vector<std::string> out;
    out.reserve(img.size());

    for (auto row : img) {
        if (row.size() < maxw) row.resize(maxw, ' '); // 우측 패딩
        std::reverse(row.begin(), row.end());
        for (char& ch : row) ch = mirrorChar(ch);
        out.push_back(std::move(row));
    }
    return out;
}
inline bool AABB(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
    return (ax < bx + bw) && (ax + aw > bx) && (ay < by + bh) && (ay + ah > by);
}
