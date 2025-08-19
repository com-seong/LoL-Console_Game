// Canvas.h
#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include "Core.h"

class Canvas {
public:
    Canvas(int w = W, int h = H);

    //화면 초기화
    void Clear();
    //글자 출력
    void BlitText(int x, int y, const std::string& s, WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    //이미지 출력
    void BlitImage(int x, int y, const std::vector<std::string>& img, WORD color);
    //테두리 출력
    void DrawBorder(WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    //프레임당 1회 WriteConsoleOutputA(hOut, buf_.data(), ...) 호출로 전체 프레임 출력.
    void Present();

    void InitConsoleOnce();

    int width() const { return W_; }
    int height() const { return H_; }
private:
    int W_, H_;
    std::vector<CHAR_INFO> buf_;
    HANDLE hOut_;
};
