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
    int  upRemain = 0;      // ���� �� ĭ
    int  downRemain = 0;    // �Ʒ��� �� ĭ
    int  accum = 0;         // ���� dt
    int  stepMs = 80;       // �� ĭ�� �̵� ms
};
inline JumpState jumpP_, jumpE_;   // �÷��̾�/��

struct HotJob {
    Team target;
    int  ticksLeft;       // ���� ƽ ��
    int  perTick;         // ƽ�� ȸ����(�յ� �й�)
    int  remainder;       // �й� �� ���� ������(���� ƽ�� +1 ��)
    int  tickMs;          // ƽ ����(ms)
    int  accMs{ 0 };        // ���� �ð�
};
struct CDRJob {
    Team target;
    int  skillMask;        // 1<<0=Q, 1<<1=W, 1<<2=E, 1<<3=R, -1=����
    int  tickMs;           // �� ms����
    int  reducePerTickMs;  // ƽ���� �󸶳� ������
    int  remainMs;         // ���� ���� �ð�
    int  accMs{ 0 };
};
enum class EnemyMode { Roam, FightMinions, Harass, Chase };

struct EnemyAI {
    int moveAccumMs = 0;     // �̵� ����
    int thinkAccumMs = 0;     // �ǻ���� ����(��: 100ms)
    int dodgeLockMs = 0;     // ȸ�� �� ��� ��ٿ�
    int shopDebounceMs = 0;     // ���� ��Ÿ ����
    EnemyMode mode = EnemyMode::Roam;
};
inline EnemyAI ai_;
class BattleManager {
public:
    BattleManager(Canvas& c, std::shared_ptr<Champion> player, std::shared_ptr<Champion> enemy);

    /////////////////////////////////�� AI�Լ�//////////////////////////////////////////////
    int chooseUsableSkill(Champion& c, std::initializer_list<int> pool);

    bool isProjectileEnemy() const {
        // TODO: �� ������Ʈ �������� ����(�̸�, id ��)
        return enemy_->name() == "Ezreal";
        return false; // �⺻�� ������ü(�ٸ��콺/���� ��)
    }
    // ���(�̴Ͼ�/�÷��̾�) ��������Ʈ �߽� ��ǥ
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

    std::unordered_map<Minion*, int> minionAtkCdMs_; // �̴Ͼ� ���� ��ٿ�(ms)

    // --- ��ų���� ����� ���� API ---
   // �߻�ü ����
    void FireProjectile(Team team, const std::vector<std::string>& art, int dir, int ttl_ms, int yOffset,int amount);
    // ���/����(��/���/�̴Ͼ� ��ħ ����)
    bool Dash(Team team, int dx, int dy);
    // ��/�÷��̾�� ���� ������
    void DamageTo(Team target, int amount);
    //����Ʈ ����ٴϱ�
    void AttachFollowEffect(Team from,
        const std::vector<std::string>& art,
        int durationMs, int damage,
        bool pierce = true,
        bool pulls = false, int pullDist = 0,
        bool singleHitPerVictim = true);
    //��������
    void StartJump(Team who, int upSteps, int downSteps, int stepMs,bool active);

    // Buff API
    void BuffMove(Team t, int totalDeltaMs, int durationMs, int tickMs = 250);
    
    void CooldownOverTime(Team team, int totalReduceMs, int durationMs,
        int tickMs = 250, int skillMask = -1);

    void Heal_OverTime(Team team, int totalAmount, int durationMs, int tickMs = 250);

    // ĳ���� ���� ��ǥ ����
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

   std::vector<std::string> enemyItems_; // ���� �� ������ �̸�

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

    //��ü ���� ��ġ
    Vec2 pPos_{ 10, 13 };
    Vec2 ePos_{ W - 15, 13 };

    // ��/����
    int goldP = 500; int goldE = 500;
    std::vector<ShopItem> shop; 
    bool shopOpen = false; 
    bool shopClose = false;
    int shopBuyDebounce_ms = 0;

    int minionBounty_{ 20 }; // ���/���� è�Ǿ��� �̴Ͼ��� óġ���� �� �޴� ���

    //������ ���� ����
    int ItemStoreCount = 0;

    // �̵� Ÿ�̸�
    int move_accum = 0;

    // �̴Ͼ� ����
    enum class WavePhase { WaitMelee, WaitRanged };

    int        waveTimerMs_ = 0;
    WavePhase  wavePhase_ = WavePhase::WaitMelee;
    bool       firstWave_ = true;   // �������ڸ��� ������ �� �� �ٷ� �Ѹ��� ����


    //��������Ʈ ũ�� ��� + �̵� �� ����
    static std::pair<int, int> spriteSize(const std::vector<std::string>& img);

    //�̵� ����
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

    void updateBuffMove(int dt); // ���ο����� ��
};
