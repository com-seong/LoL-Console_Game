// Canvas.h
#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include "Core.h"

class Canvas {
public:
    Canvas(int w = W, int h = H);

    //ȭ�� �ʱ�ȭ
    void Clear();
    //���� ���
    void BlitText(int x, int y, const std::string& s, WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    //�̹��� ���
    void BlitImage(int x, int y, const std::vector<std::string>& img, WORD color);
    //�׵θ� ���
    void DrawBorder(WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    //�����Ӵ� 1ȸ WriteConsoleOutputA(hOut, buf_.data(), ...) ȣ��� ��ü ������ ���.
    void Present();

    void InitConsoleOnce();

    int width() const { return W_; }
    int height() const { return H_; }
private:
    int W_, H_;
    std::vector<CHAR_INFO> buf_;
    HANDLE hOut_;
};
