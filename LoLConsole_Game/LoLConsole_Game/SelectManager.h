// SelectManager.h
#pragma once
#include <memory>
#include <vector>
#include "Champion.h"
#include "Canvas.h"

class SelectManager {
public:
    explicit SelectManager(Canvas& c);

    void SetCandidates(std::vector<std::shared_ptr<Champion>> cs);

    void Update();

    void Render(const char* title);

    bool Confirmed() const { return confirmed; }

    std::shared_ptr<Champion> Selected() const { 
        return cands.empty() ? nullptr : cands[cursor]; 
    }
private:
    Canvas& cv; std::vector<std::shared_ptr<Champion>> cands;
    int cursor = 0;
    bool confirmed = false; 
    int debounce = 0;
};
