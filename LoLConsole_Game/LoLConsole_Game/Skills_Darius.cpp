#include "BattleManager.h"
#include "Champion.h"
#include "Core.h"
#include "Skills_Darius.h"
void Darius_Q::Cast(BattleManager& world, Champion& self) {                //끌림 여부                                                  
    int qDamage = self.S().atk + 7;
    int qDuration = 300; // 시전 시간
    world.AttachFollowEffect(self.team(), self.SkillArt(0),
        qDuration, qDamage,
        /*pierce*/true,
        /*pulls*/false, /*pullDist*/0,
        /*singleHitPerVictim*/true);
}
void Darius_W::Cast(BattleManager& world, Champion& self) {
    int wDamage = self.S().atk + 6;
    int wDuration = 250;
    world.AttachFollowEffect(self.team(), self.SkillArt(1),
        wDuration, wDamage,
        /*pierce*/true,
        /*pulls*/false, /*pullDist*/0,
        /*singleHitPerVictim*/true);
}
void Darius_E::Cast(BattleManager& world, Champion& self) {
    int eDamage = self.S().atk - 20;
    int eDuration = 300;
    world.AttachFollowEffect(self.team(), self.SkillArt(2),
        eDuration, eDamage,
        /*pierce*/true,
        /*pulls*/true, /*pullDist*/10,
        /*singleHitPerVictim*/true);
}
// Skills_Darius.cpp
void Darius_R::Cast(BattleManager& world, Champion& self) {
    // 1) 점프 시작: 위로 3칸, 아래로 3칸, 한 칸 80ms (원하는 값으로)
    world.StartJump(self.team(), /*up*/10, /*down*/10, /*stepMs*/50, true);

    // 2) 점프 도중 몸을 따라다니는 R 이펙트 부착
    //    - 관통 + 대상당 1회만 히트(false로 바꿔 지속틱도 가능)
    //    - 데미지는 예시로 atk+20
    int rDamage = self.S().atk + 20;
    int rDuration = (10 + 10) * 50 + 150; // 점프 총 시간 + 약간의 여유
    world.AttachFollowEffect(self.team(), self.SkillArt(3),
        rDuration, rDamage,
        /*pierce*/true,
        /*pulls*/false, /*pullDist*/0,
        /*singleHitPerVictim*/true);
}
