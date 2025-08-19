#include "BattleManager.h"
#include "Champion.h"
#include "Core.h"
#include "Skills_MasterE.h"
void MasterE_Q::Cast(BattleManager& world, Champion& self) {                
    int dx = (self.team() == Team::Blue) ? +15 : -15;
    world.Dash(self.team(), dx, 0); // 겹침/벽 체크 포함
    world.AttachFollowEffect(self.team(), self.SkillArt(0),
        1000, self.S().atk - 5 ,
        /*pierce*/true,
        /*pulls*/false, /*pullDist*/0,
        /*singleHitPerVictim*/true);
}
void MasterE_W::Cast(BattleManager& world, Champion& self) {
    //데미지 0으로 설정해서 이펙트만 나오도록
    world.AttachFollowEffect(self.team(), self.SkillArt(1),
        2000, 0,
        /*pierce*/true,
        /*pulls*/false, /*pullDist*/0,
        /*singleHitPerVictim*/false);
    world.Heal_OverTime(self.team(), /*total*/100, /*duration*/3000, /*tick*/250);
}
void MasterE_E::Cast(BattleManager& world, Champion& self) {
    world.AttachFollowEffect(self.team(), self.SkillArt(2),
        1000, self.S().atk - 64,
        /*pierce*/true,
        /*pulls*/false, /*pullDist*/0,
        /*singleHitPerVictim*/false);
}
void MasterE_R::Cast(BattleManager& world, Champion& self) {
    world.AttachFollowEffect(self.team(), self.SkillArt(3),
        10000, 0,
        /*pierce*/true,
        /*pulls*/false, /*pullDist*/0,
        /*singleHitPerVictim*/false);
    world.CooldownOverTime(self.team(), /*totalReduceMs*/30000, /*durationMs*/5000, /*tickMs*/250, /*mask*/-1);
    // 3초 동안 총 -60ms(더 빠르게), 250ms마다 -5ms씩 누적
    world.BuffMove(self.team(), /*totalDeltaMs=*/- 60, /*durationMs=*/5000);
}
