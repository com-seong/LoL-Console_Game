// SelectManager.cpp
#include "SelectManager.h"
#include <windows.h>

SelectManager::SelectManager(Canvas& c) :cv(c) {}
void SelectManager::SetCandidates(std::vector<std::shared_ptr<Champion>> cs) {
    cands = std::move(cs); cursor = 0; confirmed = false; debounce = 0;
}

void SelectManager::Update() {
    if (debounce > 0) --debounce;
    if (GetAsyncKeyState(VK_LEFT) & 0x8000) { 
        if (!debounce && cursor > 0) { --cursor; debounce = 6; } 
    }
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { 
        if (!debounce && cursor + 1 < (int)cands.size()) { ++cursor; debounce = 6; } 
    }
    if (GetAsyncKeyState(VK_RETURN) & 0x8000) confirmed = true;
}
void SelectManager::Render(const char* title) {
    cv.Clear(); cv.DrawBorder();
    cv.BlitText(58, 2, title, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    int x = 28, y = 14;
    for (int i = 0;i < (int)cands.size();++i) {
        auto img = cands[i]->OnDrawRequest({ DrawRequest::Kind::Portrait,0 }).image;
        WORD col = (cands[i]->team() == Team::Blue) ? (FOREGROUND_BLUE | FOREGROUND_INTENSITY)
            : (FOREGROUND_RED | FOREGROUND_INTENSITY);
        cv.BlitImage(x, y, img, col);
        cv.BlitText(x, y + (int)img.size() + 1, cands[i]->name(),
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        if (i == cursor) cv.BlitText(x + 2, y + (int)img.size() + 3, "бу", FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        x += 44;
    }
    cv.Present();
}
