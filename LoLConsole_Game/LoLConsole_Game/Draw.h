#pragma once
#include <vector>
#include <string>

//그려달라는 요청
struct DrawRequest {
    enum class Kind { Portrait, BattleSprite, SkillEffect };
    Kind kind; int variant = 0; 
};
//이미지
struct DrawResponse { std::vector<std::string> image; };
//그릴 수 있는 모든 객체가 스스로 호출해서 캡슐화 한다.
struct IDrawable {
    virtual ~IDrawable() = default;
    virtual DrawResponse OnDrawRequest(const DrawRequest&) const = 0;
};

