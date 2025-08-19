#pragma once
#include <memory>
#include <vector>
#include "Canvas.h"
#include "Champion.h"
#include "Minion.h"
#include "Effect.h"
#include <unordered_map>
struct ShopItem { 
    std::string name; 
    int cost; 
    int atk = 0; 
    int hp = 0; 
    int move_delta = 0; 
};
struct JumpState {
    bool active = false;
    int  upRemain = 0;      // 위로 몇 칸
    int  downRemain = 0;    // 아래로 몇 칸
    int  accum = 0;         // 누적 dt
    int  stepMs = 80;       // 한 칸당 이동 ms
};
inline JumpState jumpP_, jumpE_;   // 플레이어/적

struct HotJob {
    Team target;
    int  ticksLeft;       // 남은 틱 수
    int  perTick;         // 틱당 회복량(균등 분배)
    int  remainder;       // 분배 후 남은 나머지(앞쪽 틱에 +1 씩)
    int  tickMs;          // 틱 간격(ms)
    int  accMs{ 0 };        // 누적 시간
};
struct CDRJob {
    Team target;
    int  skillMask;        // 1<<0=Q, 1<<1=W, 1<<2=E, 1<<3=R, -1=전부
    int  tickMs;           // 몇 ms마다
    int  reducePerTickMs;  // 틱마다 얼마나 줄일지
    int  remainMs;         // 버프 남은 시간
    int  accMs{ 0 };
};
enum class EnemyMode { Roam, FightMinions, Harass, Chase };

struct EnemyAI {
    int moveAccumMs = 0;     // 이동 템포
    int thinkAccumMs = 0;     // 의사결정 템포(예: 100ms)
    int dodgeLockMs = 0;     // 회피 후 잠깐 쿨다운
    int shopDebounceMs = 0;     // 상점 연타 방지
    EnemyMode mode = EnemyMode::Roam;
};
inline EnemyAI ai_;
class BattleManager {
public:
    BattleManager(Canvas& c, std::shared_ptr<Champion> player, std::shared_ptr<Champion> enemy);

    /////////////////////////////////적 AI함수//////////////////////////////////////////////
    int chooseUsableSkill(Champion& c, std::initializer_list<int> pool);

    bool isProjectileEnemy() const {
        // TODO: 네 프로젝트 기준으로 판정(이름, id 등)
        return enemy_->name() == "Ezreal";
        return false; // 기본은 비투사체(다리우스/마이 등)
    }
    // 대상(미니언/플레이어) 스프라이트 중심 좌표
    Vec2 centerOfSprite(const std::vector<std::string>& img, const Vec2& pos) const {
        auto [w, h] = spriteSize(img);
        return { pos.x + w / 2, pos.y + h / 2 };
    }

    bool enemyHasMinionNearby(int rx, int ry);

    bool incomingThreat(int horizonMs);
   
    bool tryEnemyDodge();

    void enemyAutoBuyIfRich(int dt);

    void updateEnemyAI(int dt);
    /// //////////////////////////////////////////////////////////////////////////////
   
    void SetShop(std::vector<ShopItem> s) { shop = std::move(s); }

    void Update(int dt);

    void Render();

    bool IsGameOver() const { return player_->S().hp <= 0 || enemy_->S().hp <= 0; }

    std::unordered_map<Minion*, int> minionAtkCdMs_; // 미니언별 공격 쿨다운(ms)

    // --- 스킬에서 사용할 월드 API ---
   // 발사체 생성
    void FireProjectile(Team team, const std::vector<std::string>& art, int dir, int ttl_ms, int yOffset,int amount);
    // 대시/점멸(벽/상대/미니언 겹침 금지)
    bool Dash(Team team, int dx, int dy);
    // 적/플레이어에게 직접 데미지
    void DamageTo(Team target, int amount);
    //이펙트 따라다니기
    void AttachFollowEffect(Team from,
        const std::vector<std::string>& art,
        int durationMs, int damage,
        bool pierce = true,
        bool pulls = false, int pullDist = 0,
        bool singleHitPerVictim = true);
    //점프시작
    void StartJump(Team who, int upSteps, int downSteps, int stepMs,bool active);

    // Buff API
    void BuffMove(Team t, int totalDeltaMs, int durationMs, int tickMs = 250);
    
    void CooldownOverTime(Team team, int totalReduceMs, int durationMs,
        int tickMs = 250, int skillMask = -1);

    void Heal_OverTime(Team team, int totalAmount, int durationMs, int tickMs = 250);

    // 캐릭터 현재 좌표 참조
    Vec2& Pos(Team t) { return (t == Team::Blue) ? pPos_ : ePos_; }

private:
    void handleInput(int dt);

    void spawnMinions(int dt);

    void updateMinions(int dt);

    void updateHeals(int dt);

    void updateCooldownOT(int dt);

    int  EffectiveMoveMs(Team t) const;

    void UpdateJump(int dt);

    void updateEffects(int dt);

    void spawnOneKind(bool ranged);

    void storeBoughtItem(std::string itemName);

   std::vector<HotJob> hots_;

   std::vector<CDRJob> cdrs_;

   std::vector<std::string> My_item;

   std::vector<std::string> enemyItems_; // 적이 산 아이템 이름

   std::vector<HitEvent> Collectcollisions();

    void hud();

    bool tryMove(Vec2& whoPos, int dx, int dy,
        const std::vector<std::string>& whoImg,
        const Vec2& otherPos, const std::vector<std::string>& otherImg,
        const Minion* selfMinion /*=nullptr*/,
        bool checkBothChamps /*=false*/);

    Canvas& canvas;
    std::shared_ptr<Champion> player_, enemy_;
    std::vector<std::shared_ptr<Minion>> mins;
    std::vector<std::shared_ptr<Effect>> effs;

    //객체 현재 위치
    Vec2 pPos_{ 10, 13 };
    Vec2 ePos_{ W - 15, 13 };

    // 돈/상점
    int goldP = 500; int goldE = 500;
    std::vector<ShopItem> shop; 
    bool shopOpen = false; 
    bool shopClose = false;
    int shopBuyDebounce_ms = 0;

    int minionBounty_{ 20 }; // 블루/레드 챔피언이 미니언을 처치했을 때 받는 골드

    //아이템 저장 개수
    int ItemStoreCount = 0;

    // 이동 타이머
    int move_accum = 0;

    // 미니언 스폰
    enum class WavePhase { WaitMelee, WaitRanged };

    int        waveTimerMs_ = 0;
    WavePhase  wavePhase_ = WavePhase::WaitMelee;
    bool       firstWave_ = true;   // 시작하자마자 근접을 한 번 바로 뿌릴지 여부


    //스프라이트 크기 계산 + 이동 시 검증
    static std::pair<int, int> spriteSize(const std::vector<std::string>& img);

    //이동 버프
    struct MoveBuff {
        Team t;
        int  tickMs;
        int  perTickDelta;
        int  remainMs;
        int  acc{ 0 };
        int  applied{ 0 };
    };
    std::vector<MoveBuff> moveBuffs_;
    int moveBuffSumBlue{ 0 }, moveBuffSumRed{ 0 };

    void updateBuffMove(int dt); // 내부에서만 씀
};
