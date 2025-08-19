#define NOMINMAX
#include "BattleManager.h"
#include <cmath>
#include <iostream>
#include <windows.h>

static WORD colBlue = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
static WORD colRed = FOREGROUND_RED | FOREGROUND_INTENSITY;
static WORD colHUD = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                            //캔버스           플레이어                적
BattleManager::BattleManager(Canvas& c, std::shared_ptr<Champion> p, std::shared_ptr<Champion> e)
                 //자원 이동 move
    : canvas(c), player_(std::move(p)), enemy_(std::move(e)) {
}
//미니언 좌표계산용
static inline Vec2 centerOf(const std::vector<std::string>& img, const Vec2& pos) {
    int h = (int)img.size();
    int w = 0; for (auto& s : img) w = std::max(w, (int)s.size());
    return { pos.x + w / 2, pos.y + h / 2 };
}
//주요 행동
void BattleManager::Update(int dt) {
    int itemIndx = 0;
    bool bought = false;
    shopBuyDebounce_ms = std::max(0, shopBuyDebounce_ms - dt);

    // 상점 토글
    if (GetAsyncKeyState('P') & 0x8000) { 
        if (!shopClose) { 
        shopOpen = !shopOpen; 
        shopClose = true; } 
    }
    else shopClose = false;
    if (shopOpen) {
        for (int i = 0;i < (int)shop.size() && i < 9; ++i) {
            if (itemIndx >= 0 && shopBuyDebounce_ms == 0) {
                if (GetAsyncKeyState('1' + i) & 0x8000) {
                    if (goldP >= shop[i].cost) {
                        goldP -= shop[i].cost;
                        player_->S().atk += shop[i].atk;
                        player_->S().max_hp += shop[i].hp;
                        player_->S().hp = std::min(player_->S().hp + shop[i].hp, player_->S().max_hp);
                        player_->S().move_ms = std::max(50, player_->S().move_ms + shop[i].move_delta);
                        bought = true;
                        itemIndx = i;
                        shopBuyDebounce_ms = 150; //150ms동안 추가 입력 무시. 키 중복 입력 방지.
                    }
                }
            }
        }
        if (bought) {
            storeBoughtItem(shop[itemIndx].name);
        }
        return; // 상점 열렸으면 게임 정지
    }
    //힐 업데이트
    updateHeals(dt);
    //쿨타임 버프 업데이트
    updateCooldownOT(dt);
    //이동 버프 누적 갱신
    updateBuffMove(dt);

    UpdateJump(dt);
    //적 AI로직 업데이트
    updateEnemyAI(dt);

    //플레이어키 입력
    handleInput(dt);

    //미니언 스폰
    spawnMinions(dt);
    //미니언 행동 업데이트
    updateMinions(dt);

    //이펙트 이동, 업데이트
    updateEffects(dt);
    //충돌 , 데미지 넣기
    auto hits = Collectcollisions();
    for (auto& h : hits) DamageTo(h.victim, h.amount);
    //지나간 이펙트 정리
    effs.erase(std::remove_if(effs.begin(), effs.end(), [](auto& e) { return !e->Alive(); }), effs.end());
}
//구매한 아이템 저장
void BattleManager::storeBoughtItem(std::string itemName)
{
    My_item.insert(My_item.begin() + BattleManager::ItemStoreCount, itemName);
    BattleManager::ItemStoreCount++;
}
//이미지 사이즈
std::pair<int, int> BattleManager::spriteSize(const std::vector<std::string>& img) {
    int h = (int)img.size();
    int w = 0;
    for (const auto& s : img) w = std::max(w, (int)s.size());
    return { w,h };
}
//이동 로직
// selfMinion: 자기 자신(스킵용). checkBothChamps: 두 챔피언 모두와 겹침 금지
bool BattleManager::tryMove(Vec2& whoPos, int dx, int dy,
    const std::vector<std::string>& whoImg,
    const Vec2& otherPos, const std::vector<std::string>& otherImg,
    const Minion* selfMinion /*=nullptr*/,
    bool checkBothChamps /*=false*/)
{
    auto [ww, wh] = spriteSize(whoImg);
    int nx = whoPos.x + dx;
    int ny = whoPos.y + dy;

    // 벽 클램프
    int minX = 1, minY = 6;
    int maxX = canvas.width() - ww - 5;
    int maxY = canvas.height() - wh - 5;
    nx = std::max(minX, std::min(maxX, nx));
    ny = std::max(minY, std::min(maxY, ny));

    // 상대(호출자가 넘긴 대상)과 겹침 금지
    if (AABB(nx, ny, ww, wh, otherPos.x, otherPos.y,
        spriteSize(otherImg).first, spriteSize(otherImg).second))
        return false;

    // 필요하면 챔피언 둘 다와 체크
    if (checkBothChamps) {
        auto pImg = player_->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
        auto eImg = enemy_->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
        auto [pw, ph] = spriteSize(pImg);
        auto [ew, eh] = spriteSize(eImg);
        if (AABB(nx, ny, ww, wh, pPos_.x, pPos_.y, pw, ph)) return false;
        if (AABB(nx, ny, ww, wh, ePos_.x, ePos_.y, ew, eh)) return false;
    }

    // 모든 미니언과 겹침 금지 (자기 자신 스킵)
    for (auto& m : mins) {
        if (!m->Alive()) continue;

        if (selfMinion && m.get() == selfMinion) continue; //자기 자신 스킵

        if (selfMinion && m->GetTeam() == selfMinion->GetTeam()) continue; //같은 팀 미니언 끼리는 통과

        auto img = m->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
        auto [mw, mh] = spriteSize(img);
        auto mp = m->Pos();
        if (AABB(nx, ny, ww, wh, mp.x, mp.y, mw, mh))
            return false;
    }

    // 통과했을 때만 커밋
    whoPos.x = nx; whoPos.y = ny;
    return true;
}

void BattleManager::handleInput(int dt) {
    // 이동 주기 관리
    move_accum += dt;

    // 쿨다운 틱
    for (int i = 0; i < 4; ++i) player_->cd[i].tick(dt);

    // ── 이동: 일정 주기(move_ms)마다 1칸씩
    if (move_accum >= EffectiveMoveMs(Team::Blue)) {
                                                                               
        move_accum -= EffectiveMoveMs(Team::Blue); //좀 더 부드럽게 하기 위해 누적시간을 0으로 초기화 하지 않고 남은 누적시간을 활용

        int dx = 0, dy = 0;
        if (GetAsyncKeyState(VK_LEFT) & 0x8000) dx = -1;
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000) dx = +1;
        if (GetAsyncKeyState(VK_UP) & 0x8000) dy = -1;
        if (GetAsyncKeyState(VK_DOWN) & 0x8000) dy = +1;

        if (dx || dy) {
            auto pImg = player_->OnDrawRequest({ DrawRequest::Kind::BattleSprite, 0 }).image;
            auto eImg = enemy_->OnDrawRequest({ DrawRequest::Kind::BattleSprite, 0 }).image;

            // 상대(적)과, 벽과 겹치지 않는다면만 이동    
            (void)tryMove(pPos_, dx, dy, pImg, ePos_, eImg,nullptr,false);
        }
    }

    // ── QWER 발사 (좌표는 현재 pPos_/ePos_ 기준)
    struct K { int vk; int idx; };
    static K map[4] = { {'Q',0},{'W',1},{'E',2},{'R',3} };  //q,w,e,r 매핑 배열

    //반복하면서 키 매핑
    for (auto [vk, idx] : map) {
        //매핑한 vk(키)를 입력 받으면
        if (GetAsyncKeyState(vk) & 0x8000) {
            player_->Cast(idx, *this);    //스킬별로 기본 공격력 + 몇을 한 데미지를 줘야함.
        }
    }       
}
// 근접/원거리 한 종만, 양 팀 동시에 1개씩
void BattleManager::spawnOneKind(bool ranged) {
    auto spawnFor = [&](Team t) {
        int sy = 20;
        int ly = 23;
        if (ranged == true) {
            Vec2 pos = (t == Team::Blue) ? Vec2{ 6, ly } : Vec2{ W - 8, ly };
            mins.emplace_back(std::make_shared<Minion>(/*isRanged=*/ranged, t, 100 + rand(), pos));
        }
        else {
            Vec2 pos = (t == Team::Blue) ? Vec2{ 6, sy } : Vec2{ W - 8, sy };
            mins.emplace_back(std::make_shared<Minion>(/*isRanged=*/ranged, t, 100 + rand(), pos));
        }
     };
    spawnFor(Team::Blue);
    spawnFor(Team::Red);
}
//미니언 스폰 시간
void BattleManager::spawnMinions(int dt) {
    waveTimerMs_ += dt;

    // 첫 웨이브: 즉시 근접 1번 스폰
    if (firstWave_) {
        spawnOneKind(/*ranged=*/false); // 근접
        firstWave_ = false;
        waveTimerMs_ = 0;
        wavePhase_ = WavePhase::WaitRanged;
        return;
    }
    // 이후: 상태에 따라 대기 시간만큼 경과하면 다음 것을 스폰
    if (wavePhase_ == WavePhase::WaitMelee) {
        // 원거리 이후 9초 더 기다려 총 10초 주기에서 근접
        if (waveTimerMs_ >= 10000) {
            spawnOneKind(/*ranged=*/false); // 근접
            waveTimerMs_ = 0;
            wavePhase_ = WavePhase::WaitRanged;
        }
    }
    else { // WavePhase::WaitRanged
        // 근접 1초 뒤 원거리
        if (waveTimerMs_ >= 1000) {
            spawnOneKind(/*ranged=*/true);  // 원거리
            waveTimerMs_ = 0;
            wavePhase_ = WavePhase::WaitMelee;
        }
    }
}

//미니언 업데이트
void BattleManager::updateMinions(int dt) {
    // 쿨다운 틱
    for (auto it = minionAtkCdMs_.begin(); it != minionAtkCdMs_.end(); ++it) {
        if (it->second > 0) { it->second -= dt; if (it->second < 0) it->second = 0; }
    }

    for (auto& m : mins) {
        if (!m->Alive()) continue;

        auto mImg = m->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
        auto [mw, mh] = spriteSize(mImg);
        Vec2 cur = m->Pos();

        // === 0) 교전 상대 탐색 =========================
        bool isRanged = m->IsRanged();
        int rx = isRanged ? 20 : 7;  //원거리 20, 근접 7
        int ry = 5;                 // y 허용폭

        Minion* target = nullptr;
        std::shared_ptr<Minion> targetPtr;
        for (auto& e : mins) {
            if (!e->Alive()) continue;
            if (e->GetTeam() == m->GetTeam()) continue;
            Vec2 ep = e->Pos();
            if (std::abs(cur.x - ep.x) <= rx && std::abs(cur.y - ep.y) <= ry) {
                target = e.get();
                targetPtr = e;
                break;
            }
        }

        if (target) {
            // 1) Y 정렬
            Vec2 tp = target->Pos();
            int avgY = (cur.y + tp.y) / 2;
            if (avgY != cur.y) m->SetPos({ cur.x, avgY });
            if (avgY != tp.y)  target->SetPos({ tp.x, avgY });

            // 2) 공격 쿨다운 체크
            int& cd = minionAtkCdMs_[m.get()];
            if (cd == 0) {
                // 3) 이펙트 생성 한 번만
                int fxIdx = isRanged ? 1 : 0; // 0=근접, 1=원거리 라는 전제
                auto fxArt = m->OnDrawRequest({ DrawRequest::Kind::SkillEffect, fxIdx }).image;

                // 중심 좌표
                auto tImg = targetPtr->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
                Vec2 aC = centerOf(mImg, m->Pos());
                Vec2 tC = centerOf(tImg, targetPtr->Pos());

                if (isRanged) {
                    int vx = (m->GetTeam() == Team::Blue) ? +1 : -1;
                    effs.emplace_back(std::make_shared<Effect>(
                        fxArt,
                        /*spawn*/ Vec2{ aC.x, aC.y },
                        /*vx*/ vx,
                        m->GetTeam(),
                        /*ttl_ms*/ 600,
                        /*damage*/ m->MS().damage,   //스탯 사용
                        /*pierce*/ false,
                        /*pulls*/ false, /*pullDist*/ 0,
                        /*singleHit*/ true,
                        /*follow*/ false, nullptr, Vec2{ 0,0 },
                        /*ownerKind*/ SourceKind::Minion,
                        /*ownerId*/   m->Id()));     //태깅
                }
                else {
                    // 근접: 타깃 중심에서 잠깐 번쩍
                    Vec2 spawn{ tC.x - (int)fxArt[0].size() / 2, tC.y - (int)fxArt.size() / 2 };
                    effs.emplace_back(std::make_shared<Effect>(
                        fxArt,
                        spawn,
                        /*vx*/ 0,
                        m->GetTeam(),
                        /*ttl_ms*/ 120,
                        /*damage*/ m->MS().damage,   //스탯 사용
                        /*pierce*/ true,
                        /*pulls*/ false, /*pullDist*/ 0,
                        /*singleHit*/ true,
                        /*follow*/ false, nullptr, Vec2{ 0,0 },
                        /*ownerKind*/ SourceKind::Minion,
                        /*ownerId*/   m->Id()));     //태깅
                }

                // 4) 쿨다운 재설정
                cd = isRanged ? 1200 : 900; // ms
            }

            // 교전 중에는 이동 스킵
            continue;
        }

        // === (기존) 이동/경계 처리 ===
        int dir = (m->GetTeam() == Team::Blue ? +1 : -1);
        int minX = 1;
        int maxX = canvas.width() - mw - 5;
        if ((dir > 0 && cur.x >= maxX) || (dir < 0 && cur.x <= minX)) {
            m->Kill();
            continue;
        }

        Vec2 test = cur;
        if (tryMove(test, dir, 0, mImg,
            (m->GetTeam() == Team::Blue ? ePos_ : pPos_),
            ((m->GetTeam() == Team::Blue ? enemy_ : player_)
                ->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image),
            m.get(), true))
        {
            m->SetPos(test);
        }
    }

    // 정리
    mins.erase(std::remove_if(mins.begin(), mins.end(),
        [](auto& m) { return !m->Alive(); }), mins.end());
}


//범위 벗어나면 이펙트 제거 아니면 생성
void BattleManager::updateEffects(int dt) {
    for (auto& e : effs) e->Update(dt);
}
//투사체 충돌
std::vector<HitEvent> BattleManager::Collectcollisions() {
    std::vector<HitEvent> hits;

    // 챔피언 스프라이트 크기
    auto pImg = player_->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
    auto eImg = enemy_->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
    auto [pw, ph] = spriteSize(pImg);
    auto [ew, eh] = spriteSize(eImg);

    // 챔피언 대상의 "1회 히트" 판정을 위한 식별자
    const int playerId = player_->id();
    const int enemyId = enemy_->id();

    for (auto& ef : effs) {
        if (!ef->Alive()) continue;

        // 이펙트 크기/좌표
        auto art = ef->OnDrawRequest({ DrawRequest::Kind::SkillEffect,0 }).image;
        int bw = art.empty() ? 0 : (int)art[0].size();
        int bh = (int)art.size();
        int bx = ef->Pos().x, by = ef->Pos().y;

        Team atkTeam = ef->Owner();                        // 발사 팀
        Team vicTeam = (atkTeam == Team::Blue) ? Team::Red : Team::Blue; // 피격 팀
        bool hitSomething = false;

        // ─────────────────────────────
        // 1) 챔피언 피격 판정(상대 챔피언만)
        // ─────────────────────────────
        auto tryPullChampion = [&](Team victimTeam) {
            if (!ef->PullsOnHit() || ef->PullDist() <= 0) return;

            // 가해자/피해자 현재 위치
            const Vec2& atkP = (atkTeam == Team::Blue) ? pPos_ : ePos_;
            const Vec2& vicP = (victimTeam == Team::Blue) ? pPos_ : ePos_;

            // 피해자를 가해자 쪽으로 1칸씩 당김
            int dir = (vicP.x < atkP.x) ? +1 : -1;   // victim -> attacker
            for (int s = 0; s < ef->PullDist(); ++s)
                Dash(victimTeam, dir, 0);           // 벽/겹침 체크 내장
            };

        if (vicTeam == Team::Red) {
            if (AABB(bx, by, bw, bh, ePos_.x, ePos_.y, ew, eh) && ef->TryHitOnce(enemyId)) {
                tryPullChampion(Team::Red);                        //끌기
                hits.push_back({ atkTeam, Team::Red, ef->Damage() });
                hitSomething = true;
            }
        }
        else { // vicTeam == Blue
            if (AABB(bx, by, bw, bh, pPos_.x, pPos_.y, pw, ph) && ef->TryHitOnce(playerId)) {
                tryPullChampion(Team::Blue);                       //끌기
                hits.push_back({ atkTeam, Team::Blue, ef->Damage() });
                hitSomething = true;
            }
        }
        // ─────────────────────────────
        // 2) 미니언 피격 판정(상대 미니언만)
        // ─────────────────────────────
        for (auto& m : mins) {
            if (!m->Alive() || m->GetTeam() != vicTeam) continue;

            auto mImg = m->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
            auto [mw, mh] = spriteSize(mImg);
            Vec2 mp = m->Pos();

            if (AABB(bx, by, bw, bh, mp.x, mp.y, mw, mh) && ef->TryHitOnce(m->Id())) {
                // 데미지 적용
                int dmg = std::max(0, ef->Damage());
                m->MS().hp -= dmg;

                // 사망 처리(+ 현상금)
                if (m->MS().hp <= 0) {
                    m->Kill();

                    //내 챔피언이 처치했을 때만 골드 지급
                    if (ef->OwnerKind() == SourceKind::Champion) {
                        if (atkTeam == Team::Blue) goldP += minionBounty_;
                        else                       goldE += minionBounty_;
                    }
                }

                hitSomething = true;

                // 비관통이면 한 번 맞춘 뒤 더 진행하지 않음(같은 이펙트가 여러 미니언을 동시에 때리는 것 방지)
                if (!ef->isPierce()) break;
            }
        }

        // ─────────────────────────────
        // 3) 비관통 처리
        // ─────────────────────────────
        if (hitSomething && !ef->isPierce()) {
            ef->Kill();
        }
    }

    return hits; // 챔피언 피해는 Update()에서 DamageTo로 일괄 적용
}
//HUD 출력
void BattleManager::hud() {
    // 상단 아이템/골드 — 아이템은 생략, 골드만
    canvas.BlitText(2, 0, "MY GOLD:" + std::to_string(goldP), colHUD);
    //나의 보유 아이템 출력
    canvas.BlitText(2, 2, "MY ITEM:");
    int mx = 10;
    for (const auto& s : My_item) {
        canvas.BlitText(mx, 2, s, colHUD);
        mx += (int)s.size() + 2;  // 문자열 길이 + 여백
    }

    canvas.BlitText(W - 23, 0, "ENEMY GOLD:" + std::to_string(goldE), colHUD);
    int ex = W - 30;
    canvas.BlitText(ex, 2, "ENEMY ITEM:", colHUD);
    int x = ex + 12;
    for (auto& s : enemyItems_) {
        canvas.BlitText(x, 2, s, colHUD);
        x += (int)s.size() + 2;
    }
    // 하단 상태
    canvas.BlitText(2, H - 4, "HP:" + std::to_string(player_->S().hp) + "/" + std::to_string(player_->S().max_hp) +
        " ATK:" + std::to_string(player_->S().atk) +
        " MOVE:" + std::to_string(player_->S().move_ms), colHUD);

    //적 능력치 상태
    canvas.BlitText(W - 30, H - 4, "HP:" + std::to_string(enemy_->S().hp) + "/" + std::to_string(enemy_->S().max_hp) +
        " ATK:" + std::to_string(enemy_->S().atk) +
        " MOVE:" + std::to_string(enemy_->S().move_ms), colHUD);
    //스킬 쿨 상태
    canvas.BlitText(41, H - 4, "CD Q:" + std::to_string(player_->cd[0].ms) +
        " W:" + std::to_string(player_->cd[1].ms) +
        " E:" + std::to_string(player_->cd[2].ms) +
        " R:" + std::to_string(player_->cd[3].ms), colHUD);
}

void BattleManager::Render() {
    canvas.Clear(); canvas.DrawBorder();

    auto pImg = player_->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
    auto eImg = enemy_->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
    //적 스프라이트 반전
    eImg = mirrorAscii(eImg);
    canvas.BlitImage(pPos_.x, pPos_.y, pImg, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    canvas.BlitImage(ePos_.x, ePos_.y, eImg, FOREGROUND_RED | FOREGROUND_INTENSITY);

    // 미니언
    for (auto& m : mins) {
        auto img = m->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
        WORD c = (m->GetTeam() == Team::Blue) ? colBlue : colRed;
        canvas.BlitImage(m->Pos().x, m->Pos().y, img, c);
    }
    // 이펙트
    for (auto& e : effs) {
        auto img = e->OnDrawRequest({ DrawRequest::Kind::SkillEffect,0 }).image;
        if (e->Owner() == Team::Red) img = mirrorAscii(img); //적이 쓴 이펙트도 반전
        WORD c = (e->Owner() == Team::Blue) ? colBlue : colRed;
        canvas.BlitImage(e->Pos().x, e->Pos().y, img, c);
    }

    // HUD + 상점
    hud();
    if (shopOpen) {
        int sx = 10, sy = 4;
        canvas.BlitText(sx, sy, "== SHOP (P로 닫기) ==", colHUD);
        for (int i = 0;i < (int)shop.size();++i) {
            std::string line = std::to_string(i + 1) + ") " + shop[i].name + " $" + std::to_string(shop[i].cost) +
                " (+ATK:" + std::to_string(shop[i].atk) + " +HP:" + std::to_string(shop[i].hp) +
                "MOVE SPEED:" + std::to_string(shop[i].move_delta) + ")";
            canvas.BlitText(sx, sy + 2 + i * 2, line, colHUD);
        }
    }

    canvas.Present();
}
void BattleManager::UpdateJump(int dt) {
    auto updateOne = [&](Team who, JumpState& j) {
        if (!j.active) return;
        j.accum += dt;
        while (j.accum >= j.stepMs) {
            j.accum -= j.stepMs;
            int dy = 0;
            if (j.upRemain > 0) { dy = -1; j.upRemain--; }
            else if (j.downRemain > 0) { dy = +1; j.downRemain--; }
            if (dy != 0) Dash(who, 0, dy);  // 벽/겹침 체크 포함
            if (j.upRemain == 0 && j.downRemain == 0) { j.active = false; break; }
        }
        };
    updateOne(Team::Blue, jumpP_);
    updateOne(Team::Red, jumpE_);
}
//힐 업데이트
void BattleManager::updateHeals(int dt)
{
    for (auto& h : hots_) {
        h.accMs += dt;

        while (h.ticksLeft > 0 && h.accMs >= h.tickMs) {
            h.accMs -= h.tickMs;

            // 이번 틱에 실제 회복량(앞쪽 몇 틱에 +1씩 얹어주기)
            int amount = h.perTick + (h.remainder > 0 ? 1 : 0);
            if (h.remainder > 0) --h.remainder;

            auto& guy = (h.target == Team::Blue) ? player_ : enemy_;
            int canHeal = guy->S().max_hp - guy->S().hp;
            if (canHeal > 0) {
                int add = std::min(canHeal, amount);
                guy->S().hp += add;                // 클램프
            }
            --h.ticksLeft;
        }
    }

    // 끝난 HoT 제거
    hots_.erase(
        std::remove_if(hots_.begin(), hots_.end(),
            [](const HotJob& h) { return h.ticksLeft <= 0; }),
        hots_.end());
}
void BattleManager::updateCooldownOT(int dt)
{
    for (auto& j : cdrs_) {
        j.accMs += dt;
        while (j.accMs >= j.tickMs && j.remainMs > 0) {
            j.accMs -= j.tickMs;
            j.remainMs -= j.tickMs;

            auto& who = (j.target == Team::Blue) ? player_ : enemy_;
            for (int i = 0;i < 4;++i) if (j.skillMask & (1 << i)) {
                // 진행 중인 남은 쿨을 직접 감소
                if (who->cd[i].ms > 0) {
                    who->cd[i].ms = std::max(0, who->cd[i].ms - j.reducePerTickMs);
                }
            }
        }
    }
    cdrs_.erase(std::remove_if(cdrs_.begin(), cdrs_.end(),
        [](auto& j) { return j.remainMs <= 0; }), cdrs_.end());
}
void BattleManager::updateBuffMove(int dt) {
    moveBuffSumBlue = moveBuffSumRed = 0;
    for (auto& b : moveBuffs_) {
        b.acc += dt;
        while (b.acc >= b.tickMs && b.remainMs > 0) {
            b.acc -= b.tickMs;
            b.remainMs -= b.tickMs;
            b.applied += b.perTickDelta;
        }
        if (b.t == Team::Blue) moveBuffSumBlue += b.applied;
        else                   moveBuffSumRed += b.applied;
    }
    moveBuffs_.erase(std::remove_if(moveBuffs_.begin(), moveBuffs_.end(),
        [](const MoveBuff& b) { return b.remainMs <= 0; }),
        moveBuffs_.end());
}
int BattleManager::EffectiveMoveMs(Team t) const {
    const auto& s = (t == Team::Blue ? player_ : enemy_)->S();
    int base = s.move_ms; 
    int add = (t == Team::Blue) ? moveBuffSumBlue : moveBuffSumRed;
    return std::max(50, base + add);
}
/// ////////////////////////////////////////스킬 종류 API////////////////////////////////////////////////////////////////
//투사체
void BattleManager::FireProjectile(Team team, const std::vector<std::string>& art, int dir, int ttl_ms, int yOffset,int amount) {
    Vec2 spawn = Pos(team);
    spawn.x += (dir > 0 ? +2 : -1);
    spawn.y += yOffset;                                                             //관통여부
    effs.emplace_back(std::make_shared<Effect>(art, spawn, dir, team, ttl_ms, amount,false, false, 0));
}
//데쉬
bool BattleManager::Dash(Team team, int dx, int dy) {
    auto& me = (team == Team::Blue) ? player_ : enemy_;
    auto& you = (team == Team::Blue) ? enemy_ : player_;
    auto& myP = Pos(team);
    auto& urP = Pos(team == Team::Blue ? Team::Red : Team::Blue);
    auto myImg = me->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
    auto urImg = you->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
    return tryMove(myP, dx, dy, myImg, urP, urImg,nullptr,false);
}
//데미지를 주는 함수.
void BattleManager::DamageTo(Team target, int amount) {
    auto& guy = (target == Team::Blue) ? player_ : enemy_;
    guy->S().hp -= amount;
    if (guy->S().hp < 0) guy->S().hp = 0;
}
//점프
void BattleManager::StartJump(Team who, int upSteps, int downSteps, int stepMs,bool active) {
    JumpState& j = (who == Team::Blue) ? jumpP_ : jumpE_;
    j.active = active;
    j.upRemain = upSteps;
    j.downRemain = downSteps;
    j.accum = 0;
    j.stepMs = stepMs;
}
//이펙트가 따라다니는 형식으로 공격
void BattleManager::AttachFollowEffect(Team from,
    const std::vector<std::string>& art,
    int durationMs, int damage, bool pierce,
    bool pulls, int pullDist, bool singleHitPerVictim)
{
    auto& who = (from == Team::Blue) ? player_ : enemy_;
    auto& whoPos = (from == Team::Blue) ? pPos_ : ePos_;
    auto  whoImg = who->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
    auto [w, h] = spriteSize(whoImg);

    // 몸통 중심
    Vec2 center{ whoPos.x + w / 2, whoPos.y + h / 2 };
    int aw = art.empty() ? 0 : (int)art[0].size();
    int ah = (int)art.size();

    // 아트 좌상단이 몸통 중심에 맞게 보이도록 오프셋
    Vec2 offset{ -aw / 2, -ah / 2 };

    // anchor는 챔피언 좌표의 "중심".
    // Effect는 매 프레임 anchor+offset으로 위치를 동기화.
    // anchor로 center를 주고 싶지만 center는 값이므로, 간단히 몸 위치(whoPos)를 anchor로 넘기고
    // offset에 (+w/2, +h/2)도 반영:
    Vec2 centerOffset{ w / 2 + offset.x, h / 2 + offset.y };

    effs.emplace_back(std::make_shared<Effect>(
        art,
        /*p*/Vec2{ center.x + offset.x, center.y + offset.y }, /*vx*/0, from,
        /*ttl*/durationMs, /*damage*/damage, /*pierce*/pierce,
        /*pulls*/pulls, /*pullDist*/pullDist,
        /*singleHitPerVictim*/singleHitPerVictim,
        /*follow*/true, /*anchor*/&whoPos, /*offset*/centerOffset
    ));
}
void BattleManager::Heal_OverTime(Team team, int totalAmount, int durationMs, int tickMs)
{
    // 몇 번 나눠 줄지(올림)
    int ticks = std::max(1, (durationMs + tickMs - 1) / tickMs);
    HotJob job;
    job.target = team;
    job.ticksLeft = ticks;
    job.perTick = totalAmount / ticks;   // 균등 분배
    job.remainder = totalAmount % ticks;   // 남는 1~(ticks-1) 만큼 앞쪽 틱에 +1
    job.tickMs = tickMs;
    hots_.push_back(job);
}
void BattleManager::CooldownOverTime(Team team, int totalReduceMs, int durationMs,
    int tickMs, int skillMask)
{
    // 균등 분배
    int ticks = std::max(1, (durationMs + tickMs - 1) / tickMs);
    CDRJob j;
    j.target = team;
    j.skillMask = (skillMask == -1 ? (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) : skillMask);
    j.tickMs = tickMs;
    j.reducePerTickMs = std::max(1, totalReduceMs / ticks);
    j.remainMs = durationMs;
    cdrs_.push_back(j);
}
//이동 버프
void BattleManager::BuffMove(Team t, int totalDeltaMs, int durationMs, int tickMs) {
    int ticks = std::max(1, (durationMs + tickMs - 1) / tickMs);
    moveBuffs_.push_back({ t, tickMs, totalDeltaMs / ticks, durationMs });
}
////////////////////////////////적 AI로직 필요 함수/////////////////////////////////////////////////////////
// 선택 유틸: 쿨이 빈 스킬 하나 고르기
// ── 간단 거리/사정 체크
static inline int dxAbs(const Vec2& a, const Vec2& b) { return std::abs(a.x - b.x); }
static inline int dyAbs(const Vec2& a, const Vec2& b) { return std::abs(a.y - b.y); }
static inline bool inRectRange(const Vec2& a, const Vec2& b, int rx, int ry) {
    return dxAbs(a, b) <= rx && dyAbs(a, b) <= ry;
}
int BattleManager::chooseUsableSkill(Champion& c, std::initializer_list<int> pool) {
    std::vector<int> cand;
    for (int i : pool) if (c.cd[i].ready()) cand.push_back(i);
    if (cand.empty()) return -1;
    return cand[rand() % cand.size()];
}

// 근처에 미니언 있나?(적 기준 rx,ry)
bool BattleManager::enemyHasMinionNearby(int rx, int ry) {
    for (auto& m : mins) {
        if (!m->Alive()) continue;
        if (std::abs(m->Pos().x - ePos_.x) <= rx && std::abs(m->Pos().y - ePos_.y) <= ry)
            return true;
    }
    return false;
}

// 위협 투사체(플레이어) 접근 검사: horizonMs 내에 닿을 듯?
bool BattleManager::incomingThreat(int horizonMs) {
    auto eImg = enemy_->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
    auto [ew, eh] = spriteSize(eImg);
    for (auto& ef : effs) {
        if (!ef->Alive()) continue;
        if (ef->Owner() != Team::Blue) continue;      // 플레이어 발사체만
        if (ef->IsFollow()) continue;                 // 따라다니는 이펙트 패스

        auto art = ef->OnDrawRequest({ DrawRequest::Kind::SkillEffect,0 }).image;
        int bw = art.empty() ? 0 : (int)art[0].size();
        int bh = (int)art.size();
        int bx = ef->Pos().x, by = ef->Pos().y;

        int vx = ef->Vx();
        if (vx <= 0) continue; // 적은 오른쪽, 블루 탄은 +x로 다가옴

        // x 거리/속도로 충돌 ETA 추정
        int dx = (ePos_.x - (bx + bw));
        if (dx < 0) dx = 0; // 이미 겹치거나 통과 중이면 즉시 위협
        int steps = (vx == 0 ? 999999 : dx / vx);
        int etaMs = steps * 13; // 30FPS

        // y 대충 겹치는 라인에 있으면 위협
        bool yClose = !((by + bh) < ePos_.y || by > (ePos_.y + eh));

        if (yClose && etaMs <= horizonMs) return true;
    }
    return false;
}
// 회피 시도(위아래 우선)
bool BattleManager::tryEnemyDodge() {
    // 위/아래 한 칸 시도 → 안 되면 좌우로 1~2칸
    if (Dash(Team::Red, 0, -1)) return true;
    if (Dash(Team::Red, 0, +1)) return true;
    if (Dash(Team::Red, -2, 0)) return true;
    if (Dash(Team::Red, +2, 0)) return true;
    return false;
}
// 적 자동 구매
void BattleManager::enemyAutoBuyIfRich(int dt) {
    if (ai_.shopDebounceMs > 0) { ai_.shopDebounceMs = std::max(0, ai_.shopDebounceMs - dt); return; }
    if (goldE < 300) return;

    int idx = rand() % (int)shop.size();
    if (goldE >= shop[idx].cost) {
        goldE -= shop[idx].cost;
        enemy_->S().atk += shop[idx].atk;
        enemy_->S().max_hp += shop[idx].hp;
        enemy_->S().hp = std::min(enemy_->S().hp + shop[idx].hp, enemy_->S().max_hp);
        enemy_->S().move_ms = std::max(50, enemy_->S().move_ms + shop[idx].move_delta);
        enemyItems_.push_back(shop[idx].name);  // HUD에서 출력
        ai_.shopDebounceMs = 500;
    }
}
void BattleManager::updateEnemyAI(int dt) {
    // 1) 쿨다운도 똑같이 틱
    for (int i = 0;i < 4;++i) enemy_->cd[i].tick(dt);

    // 2) 상점 자동 구매
    enemyAutoBuyIfRich(dt);

    // 3) 회피: 가끔, 가능하면
    if (ai_.dodgeLockMs > 0) ai_.dodgeLockMs = std::max(0, ai_.dodgeLockMs - dt);
    if (ai_.dodgeLockMs == 0 && incomingThreat(400)) {     // 0.4초 내 위협
        if (tryEnemyDodge()) ai_.dodgeLockMs = 350;     // 잠깐 다시 안 피함
    }

    // 4) 모드 결정(가벼운 유한상태기)
    if (player_->S().hp < 200) ai_.mode = EnemyMode::Chase;
    else if (enemyHasMinionNearby(20, 10)) ai_.mode = EnemyMode::FightMinions;
    else ai_.mode = EnemyMode::Harass;
    
   // 5) 이동 템포
    ai_.moveAccumMs += dt;
    if (ai_.moveAccumMs >= EffectiveMoveMs(Team::Red)) {
        ai_.moveAccumMs -= EffectiveMoveMs(Team::Red);

        // 타깃 결정: 미니언이 가까우면 미니언, 아니면 플레이어
        std::shared_ptr<Minion> nearMin = nullptr;
        {
            int best = 1e9;
            for (auto& m : mins) {
                if (!m->Alive() || m->GetTeam() != Team::Blue) continue;
                auto mImg = m->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
                Vec2 mC = centerOfSprite(mImg, m->Pos());

                auto eImg = enemy_->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
                Vec2 eC = centerOfSprite(eImg, ePos_);
                int d = dxAbs(eC, mC) + dyAbs(eC, mC);
                if (d < best) { best = d; nearMin = m; }
            }
        }

        // 타깃 스프라이트/중심
        std::vector<std::string> tgtImg;
        Vec2 tgtPos{};
        if (nearMin) {
            tgtImg = nearMin->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
            tgtPos = nearMin->Pos();
        }
        else {
            tgtImg = player_->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
            tgtPos = pPos_;
        }
        auto eImg = enemy_->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
        Vec2 eC = centerOfSprite(eImg, ePos_);
        Vec2 tC = centerOfSprite(tgtImg, tgtPos);

        // 투사체형이면  12~20칸 유지, 근접형이면 2~5칸까지 붙기
        bool proj = isProjectileEnemy();
        int wantMinX = proj ? 12 : 2;
        int wantMaxX = proj ? 20 : 5;
        int rxYAlign = 6; // Y 정렬 허용 폭

        int dx = 0, dy = 0;

        // Y는 살짝 맞추기
        if (dyAbs(eC, tC) > rxYAlign) dy = (tC.y < eC.y) ? -1 : +1;

        int distX = dxAbs(eC, tC);
        if (distX < wantMinX) {
            // 너무 붙었다: 투사체형만 약간 뒤로 빠져서 간격 유지, 근접형은 멈춤
            if (proj) dx = (tC.x < eC.x) ? +1 : -1;
            else dx = 0;
        }
        else if (distX > wantMaxX) {
            // 너무 멀다: 타깃 쪽으로 접근
            dx = (tC.x < eC.x) ? -1 : +1;
        }
        else {
            // 적당한 거리면 Y만 맞추거나 정지
            // (이미 dy 설정됨)
        }

        // ── 레드 리쉬: 너무 왼쪽(블루 진영 깊숙이)으로 과도하게 들어가지 않게
        const int RED_LEASH_MIN_X = W / 2 + 10; // 이보다 더 왼쪽으로는 되도록 안 내려감
        if (ePos_.x < RED_LEASH_MIN_X && !(ai_.mode == EnemyMode::Chase && player_->S().hp < 200)) {
            // 추격+플레이어 저체력 아닐 때는 오른쪽으로 복귀 유도
            if (dx < 0) dx = 0;
            if (dx == 0) dx = +1;
        }

        if (dx || dy) {
            auto pImg = player_->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
            (void)tryMove(ePos_, dx, dy, eImg, pPos_, pImg, nullptr, false);
        }
    }


    auto tryCastRangedOrMelee = [&](std::initializer_list<int> pool) {
        // 현재 타깃(위 이동과 동일 기준)
        std::shared_ptr<Minion> nearMin = nullptr;
        {
            int best = 1e9;
            for (auto& m : mins) {
                if (!m->Alive() || m->GetTeam() != Team::Blue) continue;
                auto mImg = m->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
                Vec2 mC = centerOfSprite(mImg, m->Pos());
                auto eImg = enemy_->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
                Vec2 eC = centerOfSprite(eImg, ePos_);
                int d = dxAbs(eC, mC) + dyAbs(eC, mC);
                if (d < best) { best = d; nearMin = m; }
            }
        }
        std::vector<std::string> tgtImg;
        Vec2 tgtPos{};
        if (nearMin) { tgtImg = nearMin->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image; tgtPos = nearMin->Pos(); }
        else { tgtImg = player_->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image; tgtPos = pPos_; }

        auto eImg = enemy_->OnDrawRequest({ DrawRequest::Kind::BattleSprite,0 }).image;
        Vec2 eC = centerOfSprite(eImg, ePos_);
        Vec2 tC = centerOfSprite(tgtImg, tgtPos);

        bool proj = isProjectileEnemy();
        int rx = proj ? 22 : 5;  // 투사체 대충 사거리, 근접은 아주 짧게
        int ry = 7;

        // 범위 안에 들어왔을 때만 스킬 선택
        if (!inRectRange(eC, tC, rx, ry)) return;

        int idx = chooseUsableSkill(*enemy_, pool);
        if (idx < 0) return;

        // ── 마스터 이 Q: 과돌진 제어(거리 윈도우, 리쉬 고려)
        bool isMasterYi = false; 
        if (isMasterYi && idx == 0 /*Q 가정*/) {
            int distX = dxAbs(eC, tC);
            if (distX < 3 || distX > 16) return;     // 너무 가까우면/너무 멀면 Q 금지
            const int RED_LEASH_MIN_X = W / 2 + 6;
            if (ePos_.x < RED_LEASH_MIN_X && !(ai_.mode == EnemyMode::Chase && player_->S().hp < 200))
                return; // 리쉬 넘어가면 Q 안씀(계속 파고드는 것 방지)
        }

        enemy_->Cast(idx, *this);
        };

    if (ai_.mode == EnemyMode::FightMinions) {
        tryCastRangedOrMelee({ 0,1,2 }); // R 제외
    }
    else if (ai_.mode == EnemyMode::Chase) {
        tryCastRangedOrMelee({ 0,1,2,3 });
    }
    else if (ai_.mode == EnemyMode::Harass) {
        if (rand() % 50 == 0) tryCastRangedOrMelee({ 0,1,2,3 });
    }
}


