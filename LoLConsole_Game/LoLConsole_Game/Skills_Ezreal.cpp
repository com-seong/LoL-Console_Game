// Skills_Ezreal.cpp
#include "BattleManager.h"
#include "Champion.h"
#include "Core.h"
#include "Skills_Ezreal.h"
void Ezreal_Q::Cast(BattleManager& world, Champion& self) {
    int dir = (self.team() == Team::Blue) ? +1 : -1;
    world.FireProjectile(self.team(), self.SkillArt(0), dir, 1500,0, self.S().atk + 10);
}
void Ezreal_W::Cast(BattleManager& world, Champion& self) {
    int dir = (self.team() == Team::Blue) ? +1 : -1;
    world.FireProjectile(self.team(), self.SkillArt(1), dir, 1000, /*yOffset=*/0,self.S().atk+5);
}
void Ezreal_E::Cast(BattleManager& world, Champion& self) {
    int dx = (self.team() == Team::Blue) ? -5 : +5;
    world.Dash(self.team(), dx, 0); // 겹침/벽 체크 포함
}
void Ezreal_R::Cast(BattleManager& world, Champion& self) {
    int dir = (self.team() == Team::Blue) ? +1 : -1;
    world.FireProjectile(self.team(), self.SkillArt(3), dir, 3000, 0, self.S().atk + 20);
}
