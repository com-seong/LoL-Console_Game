// Canvas.cpp
#include "Canvas.h"

static void SetConsoleRect(HANDLE h, int w, int hgt) {

    SMALL_RECT tmp = { 0,0,1,1 }; SetConsoleWindowInfo(h, TRUE, &tmp);

    COORD sz = { (SHORT)w,(SHORT)hgt }; SetConsoleScreenBufferSize(h, sz);

    SMALL_RECT win = { 0,0,(SHORT)(w - 1),(SHORT)(hgt - 1) }; SetConsoleWindowInfo(h, TRUE, &win);
}
Canvas::Canvas(int w, int h) :W_(w), H_(h), buf_(w* h) {
    hOut_ = GetStdHandle(STD_OUTPUT_HANDLE);

    InitConsoleOnce();

    SetConsoleRect(hOut_, W_, H_);
}
void Canvas::InitConsoleOnce() {

    CONSOLE_CURSOR_INFO ci{ 1,FALSE }; SetConsoleCursorInfo(hOut_, &ci);  //FALSE로 커서 숨김

    DWORD mode = 0; GetConsoleMode(hOut_, &mode);

    mode &= ~ENABLE_QUICK_EDIT_MODE; SetConsoleMode(hOut_, mode);  //입력 멈춤 현상 방지
}
void Canvas::Clear() {
    for (auto& c : buf_) { 
        c.Char.AsciiChar = ' '; 
        c.Attributes = 0; 
    }
}
void Canvas::BlitText(int x, int y, const std::string& s, WORD color) {
    if (y < 0 || y >= H_) return;
    for (int i = 0;i < (int)s.size();++i) {
        int xx = x + i; if (xx < 0 || xx >= W_) continue;
        auto& cell = buf_[y * W_ + xx];
        cell.Char.AsciiChar = s[i];
        cell.Attributes = color;
    }
}
void Canvas::BlitImage(int x, int y, const std::vector<std::string>& img, WORD color) {
    for (int r = 0;r < (int)img.size();++r) {
        int yy = y + r; if (yy < 0 || yy >= H_) continue;
        const auto& line = img[r];
        for (int c = 0;c < (int)line.size();++c) {
            int xx = x + c; if (xx < 0 || xx >= W_) continue;
            auto& cell = buf_[yy * W_ + xx];
            cell.Char.AsciiChar = line[c];
            cell.Attributes = color;
        }
    }
}
void Canvas::DrawBorder(WORD color) {
    for (int x = 0;x < W_;++x) {
        buf_[5 * W_ + x].Char.AsciiChar = '-';  //위
        buf_[5 * W_ + x].Attributes = color;
        buf_[(H_ - 5) * W_ + x].Char.AsciiChar = '-'; //아래
        buf_[(H_ - 5) * W_ + x].Attributes = color;
    }
    for (int y = 0;y < H_;++y) {
        buf_[y * W_ + 0].Char.AsciiChar = '|'; 
        buf_[y * W_ + 0].Attributes = color;
        buf_[y * W_ + (W_ - 1)].Char.AsciiChar = '|'; 
        buf_[y * W_ + (W_ - 1)].Attributes = color;
    }
}
void Canvas::Present() {
    COORD bs{ (SHORT)W_,(SHORT)H_ };
    COORD bc{ 0,0 };
    SMALL_RECT rc{ 0,0,(SHORT)(W_ - 1),(SHORT)(H_ - 1) };
    WriteConsoleOutputA(hOut_, buf_.data(), bs, bc, &rc);  // 프레임 당 1회
}
