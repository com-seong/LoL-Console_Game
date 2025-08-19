#pragma once
#include <vector>
#include <string>

//�׷��޶�� ��û
struct DrawRequest {
    enum class Kind { Portrait, BattleSprite, SkillEffect };
    Kind kind; int variant = 0; 
};
//�̹���
struct DrawResponse { std::vector<std::string> image; };
//�׸� �� �ִ� ��� ��ü�� ������ ȣ���ؼ� ĸ��ȭ �Ѵ�.
struct IDrawable {
    virtual ~IDrawable() = default;
    virtual DrawResponse OnDrawRequest(const DrawRequest&) const = 0;
};

