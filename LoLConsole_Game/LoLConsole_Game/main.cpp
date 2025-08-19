/*예전 방식(끊김 / 깜빡임의 원인)
매 프레임(혹은 변경된 셀마다) :

    SetConsoleCursorPosition(gotoXY)

    SetConsoleTextAttribute(색 바꾸기)

    putchar / printf로 문자 1개 출력

    즉, 셀마다(또는 변경된 셀마다) 시스템 콜이 발생.
    해상도 156×41이면 6, 396셀. 30 % 만 바뀌어도 1, 900회 내외 호출.전체화면으로 키우면 바뀌는 셀 수도 커지고, 커서 이동도 더 자주 일어나서 체감 프레임이 급락함.

    커서가 보이는 상태에서 계속 위치가 옮겨지니 커서 깜빡임도 같이 보임.

    게임 루프 여기저기에 Sleep(...)이 흩어져 있으면 입력도 그 순간 멈춤 → “키가 씹힌다”로 느껴짐.

    지금 방식(부드러운 이유)
    한 프레임을** 메모리 버퍼(CHAR_INFO[W×H])** 에 전부 합성한 뒤,

    WriteConsoleOutputA(...)를 프레임당 딱 1번 호출해서 화면 버퍼로 통째로 복사.

    커서 표시는 처음에 아예 꺼둔다(SetConsoleCursorInfo(..., bVisible = FALSE)),커서 이동 자체가 없어 깜빡임이 사라짐.

    Quick Edit(마우스 드래그 시 입력 중단)을 꺼서 입력 먹통 현상을 차단.

    프레임 타이밍은 루프 꼬리에서만 고정 간격(DT = 33ms)으로 sleep.중간에는 절대 sleep X.
    지금은 루프 꼬리에서만 DT 기준(33ms ≈ 30fps)으로 쉬고, 나머지는 전부 dt 기반 타이머(쿨타임/TTL/스폰/이동)로.
    어떤 해상도/창 크기에서도 동일한 게임 속도가 유지.

    매 프레임에 한 번, WriteConsoleOutputA로 내부 백버퍼(CHAR_INFO 배열)를 통째로 내보낸다. 
    셀마다 gotoXY/putchar를 부르지 않으니 시스템 콜 횟수가 급감하고, 깜빡임이 사라짐.
    메인 루프 꼬리에서만 고정 DT(예: 33ms)로 슬립.
    프레임 타이밍이 한 곳으로 모여 있으니 입력/업데이트가 중간에 끊기지 X.
    그리고 바로 그 루프 1회마다 Present()가 호출되어 프레임당 1회 출력이 보장됨.
    챔피언: cd[i].tick(dt)로 스킬 쿨타임.
    이펙트: ttl.tick(dt) + 속도에 따라 좌표 갱신.
    미니언: 스폰 간격·이동 주기를 dt 누적으로 제어.
    각 객체가 자기 타이머/상태를 들고 매 프레임 독립적으로 진화하므로, 
    동시에 여러 것이 움직여도 서로를 블로킹하지 않음
    요약하면 :
    [예전]  게임 로직 →(셀 반복) gotoXY + SetAttr + putchar →(커서 깜빡임, 수천 시스템콜) → 화면
    [지금]  게임 로직 → backbuffer(CHAR_INFO) 합성 → WriteConsoleOutputA(1회) → 화면
*/
#include <chrono>
#include <thread>
#include <memory>
#include <vector>
#include "Canvas.h"
#include "Champion.h"
#include "SelectManager.h"
#include "BattleManager.h"
#include "Skills_Darius.h"
#include "Skills_Ezreal.h"
#include "Skills_MasterE.h"
using namespace std;
using namespace std::chrono;

// 편의: 문자열 배열 → 벡터
static vector<string> V(const char* const* arr, int n) {
    vector<string> v; v.reserve(n); for (int i = 0;i < n;++i) v.emplace_back(arr[i]); return v; 
}
//런타임 객체 고유번호
struct IdGen { static int next() { static int n = 1; return n++; } };

int main() {
    Canvas canvas(W, H);
    
    //챔피언 아트
    //이즈리얼 이미지,스킬
    static const char* EZ_IMG[3] = { "  * ", " <$> ", " / \\" };
    static const char* EZ_Q[1] = { ">>>>>" };  //발사
    static const char* EZ_W[1] = { "]]]]" };  //발사
    static const char* EZ_E[1] = { ">>" };   //앞으로 순간이동
    static const char* EZ_R[1] = { "&&&&&&&" };    //발사

    //다리우스 이미지,스킬
    static const char* DR_IMG[4] = { "  @  <<&>>","| & |-/"," / \\","<   >" }; 
    static const char* DR_Q[1] = {"<<<<<<<<<         >>>>>>>>>"};        //화살표에 닿으면 데미지.
    static const char* DR_W[1] = { "       ======++++" };             //강화된 공격을 할 수 있음
    static const char* DR_E[1] = { "       ------>>>>>>>&&&&&&&" };          //닿으면 다리우스쪽으로 강제로 이동
    static const char* DR_R[1] = { "        #$$$$$$&&&" };          //사거리 내에 잇으면 다리우스가 적 대가리를 찍음

    //마스터이 이미지,스킬
    static const char* ME_IMG[4] = { "  &   /","| o |/"," / \\","#   #" };    // Master Yi
    static const char* ME_Q[1] = { ">>>>>>>>>>>>>>" };        //사거리 내에서 쓰면 적에게 이동후 데미지 입히기
    static const char* ME_W[1] = { "((   ))" };      //체력 3초에 걸쳐 회복. 이펙트는 주변에 둘러쌓이는것
    static const char* ME_E[1] = { "        ------*****" };      //관통으로 지지기
    static const char* ME_R[1] = { "=======+              " };         //잠시동안 이동속도,스킬 쿨타임이 빨라짐 이펙트는 뒤에 나옴

    ChampionArt ez{ V(EZ_IMG,3), { V(EZ_Q,1), V(EZ_W,1), V(EZ_E,1), V(EZ_R,1) } };
    ChampionArt dr{ V(DR_IMG,4), { V(DR_Q,1), V(DR_W,1), V(DR_E,1), V(DR_R,1) } };
    ChampionArt me{ V(ME_IMG,4), { V(ME_Q,1), V(ME_W,1), V(ME_E,1), V(ME_R,1) } };
    ////차라리 챔피언을 생성할때 state에 값을 더하고 빼서 그 상태로 할까.  
    //챔피언들 생성
    auto P1 = make_shared<Champion>("Ezreal", Team::Blue, IdGen::next(), ez);
    P1->SetSkill(0, std::make_unique<Ezreal_Q>());      //스킬 삽입
    P1->SetSkill(1, std::make_unique<Ezreal_W>());
    P1->SetSkill(2, std::make_unique<Ezreal_E>());
    P1->SetSkill(3, std::make_unique<Ezreal_R>());

    auto P2 = make_shared<Champion>("Darius", Team::Blue, IdGen::next(), dr);
    P2->SetSkill(0, std::make_unique<Darius_Q>());
    P2->SetSkill(1, std::make_unique<Darius_W>()); 
    P2->SetSkill(2, std::make_unique<Darius_E>());
    P2->SetSkill(3, std::make_unique<Darius_R>());

    auto P3 = make_shared<Champion>("MasterE", Team::Blue, IdGen::next(), me);
    P3->SetSkill(0, std::make_unique<MasterE_Q>());
    P3->SetSkill(1, std::make_unique<MasterE_W>());
    P3->SetSkill(2, std::make_unique<MasterE_E>());
    P3->SetSkill(3, std::make_unique<MasterE_R>());

    //this_thread -> 현재 진행되는 작업 , sleep_for -> 잠시 멈추기
    // ── 플레이어 선택
    SelectManager select(canvas);
    //선택할 목록
    select.SetCandidates({ P1,P2,P3 });
           //선택되지 않았다면 반복
    while (!select.Confirmed()) { 
        select.Update(); 
        select.Render("플레이어 챔피언을 선택하세요 (←/→, Enter)"); 
        //프레임 타이밍을 한 곳에서만 제어해서 입력 / 업데이트가 끊기지 않게.
        this_thread::sleep_for(milliseconds(50)); 
    }
    select.Render("              Selected");
    Sleep(1000);
    //선택한 것을 player에게 넘겨주기
    auto player = select.Selected();
    player->SetStats(player->name());

    // ── 적 선택(팀만 Red로 바꿔 재생성)  
    auto E1 = make_shared<Champion>("Ezreal", Team::Red, IdGen::next(), ez);
    E1->SetSkill(0, std::make_unique<Ezreal_Q>());      //스킬 삽입
    E1->SetSkill(1, std::make_unique<Ezreal_W>());
    E1->SetSkill(2, std::make_unique<Ezreal_E>());
    E1->SetSkill(3, std::make_unique<Ezreal_R>());
    //챔피언에다가 능력치 세팅시켜주는 함수를 만들고? 
    auto E2 = make_shared<Champion>("Darius", Team::Red, IdGen::next(), dr);
    E2->SetSkill(0, std::make_unique<Darius_Q>());
    E2->SetSkill(1, std::make_unique<Darius_W>()); // 미구현이면 nullptr 가능
    E2->SetSkill(2, std::make_unique<Darius_E>());
    E2->SetSkill(3, std::make_unique<Darius_R>());
    auto E3 = make_shared<Champion>("MasterE", Team::Red, IdGen::next(), me);
    E3->SetSkill(0, std::make_unique<MasterE_Q>());
    E3->SetSkill(0, std::make_unique<MasterE_Q>());
    E3->SetSkill(1, std::make_unique<MasterE_W>());
    E3->SetSkill(2, std::make_unique<MasterE_E>());
    E3->SetSkill(3, std::make_unique<MasterE_R>());
    //선택할 목록
    select.SetCandidates({ E1,E2,E3 });
        //선택되지 않았다면 반복
    while (!select.Confirmed()) {
        select.Update(); select.Render("적 챔피언을 선택하세요 (←/→, Enter)"); 
        this_thread::sleep_for(milliseconds(50)); 
    }
    //선택한 것을 enemy에게 넘겨주기
    auto enemy = select.Selected();
    enemy->SetStats(enemy->name());
    // ── 전투                 그리기 플레이어 적
    BattleManager battel_manager(canvas, player, enemy);
    //상점 세팅하기
    battel_manager.SetShop({ {"Long Sword",300, +10, 0, 0}, 
                             {"Boots",300, 0, 0, -30}, 
                             {"Ruby Crystal", 400, 0, +120, 0} });
    //마지막 시간 기록
    auto last = steady_clock::now();
    //틱
    const int DT = 33; // 30fps
    //게임이 끝날때까지
    while (!battel_manager.IsGameOver()) {
        //시간이 흐른 후 
        //현재시간 기록
        auto now = steady_clock::now();
        //현재 - 전에 시간 = 얼마나 흘렀는지 el
        auto el = duration_cast<milliseconds>(now - last).count();
        //el이 틱 보다 작다면 틱 - el만큼 멈추기
        if (el < DT) { this_thread::sleep_for(milliseconds(DT - el)); continue; }
        //마지막 시간 기록
        last = steady_clock::now();
        //변경사항 업데이트
        battel_manager.Update(DT);
        //그리기
        battel_manager.Render();
        //esc누르면 배틀 종료
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;
    }

    // 결과
    //캔버스 지우기
    canvas.Clear();
    //플레이어 피가 0이면 졌다 아니면 이겼다. 
    const char* msg = (player->S().hp > 0) ? "YOU WIN" : "YOU LOSE";
    canvas.BlitText((W - (int)strlen(msg)) / 2, H / 2, msg, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    canvas.Present(); this_thread::sleep_for(milliseconds(1200));
    return 0;
}
