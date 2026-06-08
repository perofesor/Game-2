#include "Game.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

static float frand(){ return (float)rand()/(float)RAND_MAX; }
static float frand2(){ return frand()*2.0f-1.0f; }

int Game::nearestEnemy(glm::vec3 from, float maxDist){
    int best=-1; float bd=maxDist;
    for (size_t i=0;i<enemies.size();++i){
        if (!enemies[i].alive) continue;
        float d = glm::length(enemies[i].pos - from);
        if (d < bd){ bd=d; best=(int)i; }
    }
    return best;
}

void Game::toggleLock(){
    if (lockTarget >= 0) { lockTarget = -1; return; }
    lockTarget = nearestEnemy(player.pos, 40.0f);
}

// Build a world-space ray from the camera through the on-screen crosshair
// (which is always at the screen centre, because the cursor is disabled).
void Game::aimRay(glm::vec3& outOrigin, glm::vec3& outDir){
    // crosshair is centred -> use NDC (0,0)
    glm::vec4 rayClip(0.0f, 0.0f, -1.0f, 1.0f);
    glm::mat4 invProj = glm::inverse(cam.proj());
    glm::vec4 rayEye = invProj*rayClip; rayEye.z=-1.0f; rayEye.w=0.0f;
    glm::mat4 invView = glm::inverse(cam.view());
    outDir = glm::normalize(glm::vec3(invView*rayEye));
    outOrigin = cam.position;
}

// Intersect the aim ray with the nearest enemy (sphere) or, failing that,
// the ground plane. This is THE point the crosshair is pointing at, so the
// projectile flies exactly there.
glm::vec3 Game::groundAimPoint(){
    // if locked, always aim at the locked target's chest
    if (lockTarget>=0 && lockTarget<(int)enemies.size() && enemies[lockTarget].alive)
        return enemies[lockTarget].pos + glm::vec3(0,1.0f,0);

    glm::vec3 o, dir; aimRay(o, dir);

    // 1) try enemy hit (ray-sphere) – pick the closest enemy along the ray
    float bestT = 1e9f; bool hit=false; glm::vec3 hitPoint(0);
    for (auto& e : enemies){
        if (!e.alive) continue;
        glm::vec3 c = e.pos + glm::vec3(0,1.0f,0);
        float rr = e.radius*1.6f + 0.4f;
        glm::vec3 oc = o - c;
        float b = glm::dot(oc, dir);
        float cc = glm::dot(oc,oc) - rr*rr;
        float disc = b*b - cc;
        if (disc < 0) continue;
        float t = -b - std::sqrt(disc);
        if (t > 0.1f && t < bestT){ bestT=t; hit=true; hitPoint = o + dir*t; }
    }
    if (hit) return hitPoint;

    // 2) fall back to the ground plane (y=0)
    if (std::fabs(dir.y) > 1e-4f){
        float t = -o.y/dir.y;
        if (t > 0){
            glm::vec3 g = o + dir*t;
            return glm::vec3(g.x, 0.0f, g.z);
        }
    }
    // 3) far point straight ahead
    return o + dir*60.0f;
}

glm::vec3 Game::screenToGround(double, double){
    return groundAimPoint();
}

void Game::castSelected(){
    if (selectedAbility < 0 || selectedAbility >= (int)abilities.size()) return;
    Ability& a = abilities[selectedAbility];
    if (!a.unlocked) return;
    if (!a.ready()) {
        floatTexts.push_back({player.pos+glm::vec3(0,2.4f,0),"COOLDOWN",glm::vec4(0.8f,0.8f,0.8f,1),0.7f,0.7f});
        return;
    }
    if (player.mana < a.manaActual()) {
        floatTexts.push_back({player.pos+glm::vec3(0,2.4f,0),"NO MANA",glm::vec4(0.4f,0.6f,1,1),0.7f,0.7f});
        return;
    }
    glm::vec3 target = aiming ? aimPoint : groundAimPoint();

    player.mana -= a.manaActual();
    a.cooldownTimer = a.cooldown;

    switch (a.id) {
        case AbilityId::ChainLink:
        case AbilityId::Fireball:
        case AbilityId::Meteor:
        case AbilityId::Daggers:
            player.anim.triggerAction(a.id==AbilityId::Daggers?ActionState::Throw:ActionState::Cast, 0.5f);
            fireProjectile(a.id, target);
            break;
        case AbilityId::Frost:
        case AbilityId::Inferno:
        case AbilityId::Poison:
        case AbilityId::GraspingHands:
            player.anim.triggerAction(ActionState::Cast, 0.6f);
            applyAreaEffect(a.id, target);
            break;
        default: break;
    }
}

void Game::fireProjectile(AbilityId id, glm::vec3 target){
    Ability& a = abilities[(int)id];
    glm::vec3 spawn = player.pos + glm::vec3(0,1.55f,0)
                    + glm::vec3(std::sin(player.facing),0,std::cos(player.facing))*0.6f;
    // Direction: fire exactly toward the crosshair target so the impact lands
    // where the player is pointing.
    glm::vec3 dir = target - spawn;
    if (glm::length(dir) < 0.01f) dir = glm::vec3(std::sin(player.facing),0,std::cos(player.facing));
    dir = glm::normalize(dir);
    Projectile p;
    p.pos = spawn; p.color = a.color; p.damage = a.dmg()*player.spellPower;
    p.sourceAbility = (int)id;
    p.target = target;
    switch (id) {
        case AbilityId::ChainLink: p.type=ProjType::Fireball; p.vel=dir*32.0f; p.radius=0.0f; p.maxTargets=1; break;
        case AbilityId::Fireball:  p.type=ProjType::Fireball; p.vel=dir*38.0f; p.radius=1.8f; p.maxTargets=1; break;
        case AbilityId::Meteor: {
            // Meteor arcs down onto the aimed ground point from the sky.
            glm::vec3 gp = glm::vec3(target.x,0,target.z);
            p.type=ProjType::Meteor; p.pos = gp + glm::vec3(0,26.0f,0);
            glm::vec3 down = glm::normalize(glm::vec3(0,-1,0));
            p.vel = down*30.0f; p.radius=a.radius; p.maxTargets=10; p.target=gp;
        } break;
        case AbilityId::Daggers:   p.type=ProjType::Dagger;   p.vel=dir*46.0f; p.radius=0.0f; p.maxTargets=1; break;
        default: break;
    }
    projectiles.push_back(p);
}

void Game::applyAreaEffect(AbilityId id, glm::vec3 center){
    Ability& a = abilities[(int)id];
    switch (id) {
        case AbilityId::Frost: {
            Zone z; z.type=ZoneType::Frost; z.pos=center; z.radius=a.radius+0.5f*(a.level-1);
            z.duration=z.maxDuration=3.0f+0.5f*a.level; z.dps=a.dmg(); zones.push_back(z);
            for (size_t i=0;i<enemies.size();++i) if (enemies[i].alive &&
                glm::length(enemies[i].pos-center) < z.radius){
                enemies[i].status.freeze = z.duration;
                damageEnemy((int)i, a.dmg()*0.5f, a.color, false);
            }
            particles.burst(center+glm::vec3(0,0.5f,0), glm::vec4(0.5f,0.85f,1,0.9f), 50, z.radius*0.6f, 4.0f, 0.35f, 1.0f);
        } break;
        case AbilityId::Inferno: {
            Zone z; z.type=ZoneType::Inferno; z.pos=center; z.radius=a.radius+0.5f*(a.level-1);
            z.duration=z.maxDuration=4.0f; z.dps=a.dmg(); zones.push_back(z);
            for (size_t i=0;i<enemies.size();++i) if (enemies[i].alive &&
                glm::length(enemies[i].pos-center)<z.radius){
                enemies[i].status.burn = 4.0f; enemies[i].status.burnDps = a.dmg();
            }
            particles.burst(center+glm::vec3(0,0.3f,0), glm::vec4(1,0.4f,0.05f,0.9f), 60, z.radius*0.6f, 3.0f, 0.4f, 1.2f);
        } break;
        case AbilityId::Poison: {
            // poison applies stacks to enemies in radius
            for (size_t i=0;i<enemies.size();++i) if (enemies[i].alive &&
                glm::length(enemies[i].pos-center)<a.radius){
                enemies[i].status.poisonStacks = std::min(8, enemies[i].status.poisonStacks+1);
                enemies[i].status.poisonTimer = 5.0f;
                damageEnemy((int)i, a.dmg(), a.color, false);
            }
            particles.burst(center+glm::vec3(0,0.5f,0), glm::vec4(0.3f,0.9f,0.2f,0.8f), 40, a.radius*0.6f, 2.5f, 0.35f, 1.0f);
        } break;
        case AbilityId::GraspingHands: {
            Zone z; z.type=ZoneType::Grasp; z.pos=center; z.radius=a.radius;
            z.duration=z.maxDuration=4.0f; z.dps=a.dmg(); zones.push_back(z);
            for (size_t i=0;i<enemies.size();++i) if (enemies[i].alive &&
                glm::length(enemies[i].pos-center)<a.radius && !enemies[i].isBoss){
                enemies[i].status.grasp = 4.0f;
            }
            particles.burst(center+glm::vec3(0,0.2f,0), glm::vec4(0.4f,0.2f,0.5f,0.9f), 50, a.radius*0.7f, 3.0f, 0.4f, -0.5f);
        } break;
        default: break;
    }
}

void Game::updateProjectiles(float dt){
    for (auto& p : projectiles){
        if (!p.alive) continue;
        p.life -= dt;
        if (p.life<=0){ p.alive=false; continue; }
        if (p.type==ProjType::Meteor){ p.vel.y -= 6.0f*dt; }
        p.pos += p.vel*dt;

        // trail
        glm::vec4 trailCol(p.color, 0.9f);
        float tsize = (p.type==ProjType::Meteor)?0.9f:(p.type==ProjType::Dagger?0.12f:0.35f);
        particles.spawn(p.pos, glm::vec3(frand2()*0.5f,frand2()*0.5f,frand2()*0.5f),
                        trailCol, tsize, 0.4f, 0.0f);

        // hostile enemy bolt: travels toward the player, damages on contact
        if (p.hostile){
            if (glm::length(p.pos - (player.pos+glm::vec3(0,1.2f,0))) < 0.9f){
                player.hp -= p.damage;
                particles.burst(player.pos+glm::vec3(0,1.2f,0), glm::vec4(0.7f,0.2f,0.9f,1), 18, 0.4f, 3.0f, 0.3f, 0.5f);
                floatTexts.push_back({player.pos+glm::vec3(0,2.2f,0),"-"+std::to_string((int)p.damage),glm::vec4(1,0.3f,0.3f,1),0.9f,0.9f});
                p.alive=false;
            }
            if (p.pos.y < 0.1f) p.alive=false;
            continue;
        }

        // ground hit (meteor)
        if (p.pos.y < 0.2f && p.type==ProjType::Meteor){
            applyAreaEffect(AbilityId::Inferno, glm::vec3(p.pos.x,0,p.pos.z));
            // direct damage to up to maxTargets
            int hit=0;
            for (size_t i=0;i<enemies.size() && hit<p.maxTargets;++i)
                if (enemies[i].alive && glm::length(enemies[i].pos-glm::vec3(p.pos.x,0,p.pos.z))<p.radius){
                    damageEnemy((int)i, p.damage, p.color, true); hit++;
                }
            particles.burst(p.pos, glm::vec4(1,0.4f,0.05f,1), 120, 1.0f, 9.0f, 0.6f, 1.0f);
            particles.burst(p.pos, glm::vec4(0.3f,0.3f,0.3f,0.8f), 50, 1.0f, 4.0f, 0.8f, 0.5f);
            p.alive=false; continue;
        }

        // enemy collision
        for (size_t i=0;i<enemies.size();++i){
            if (!enemies[i].alive) continue;
            float hitR = enemies[i].radius + (p.radius>0?p.radius:0.5f);
            if (glm::length(enemies[i].pos+glm::vec3(0,1.0f,0) - p.pos) < hitR){
                if (p.sourceAbility==(int)AbilityId::ChainLink){
                    // mark chained: link nearby enemies into a group
                    enemies[i].status.chained = true;
                    damageEnemy((int)i, p.damage, p.color, true);
                } else if (p.type==ProjType::Dagger){
                    enemies[i].status.bleedStacks = std::min(10, enemies[i].status.bleedStacks+1);
                    enemies[i].status.bleedTimer = 5.0f;
                    damageEnemy((int)i, p.damage, p.color, true);
                    particles.burst(enemies[i].pos+glm::vec3(0,1,0), glm::vec4(0.7f,0.05f,0.05f,1), 14, 0.3f, 3.0f, 0.2f, 0.5f);
                } else if (p.radius>0){
                    // AoE blast
                    int hit=0;
                    for (size_t j=0;j<enemies.size() && hit<p.maxTargets;++j)
                        if (enemies[j].alive && glm::length(enemies[j].pos-enemies[i].pos)<p.radius){
                            damageEnemy((int)j, p.damage, p.color, true); hit++;
                        }
                    particles.burst(p.pos, glm::vec4(p.color,1), 40, 0.5f, 5.0f, 0.4f, 0.8f);
                } else {
                    damageEnemy((int)i, p.damage, p.color, true);
                }
                p.alive=false;
                break;
            }
        }
    }
    projectiles.erase(std::remove_if(projectiles.begin(),projectiles.end(),
        [](const Projectile& p){ return !p.alive; }), projectiles.end());
}

void Game::updateZones(float dt){
    for (auto& z : zones){
        if (!z.alive) continue;
        z.duration -= dt;
        z.tickTimer -= dt;
        if (z.tickTimer<=0){
            z.tickTimer = 0.5f;
            for (size_t i=0;i<enemies.size();++i){
                if (!enemies[i].alive) continue;
                if (glm::length(enemies[i].pos-z.pos) < z.radius){
                    if (z.type==ZoneType::Frost){ enemies[i].status.freeze = std::max(enemies[i].status.freeze, 0.6f);
                        damageEnemy((int)i, z.dps*0.5f, glm::vec3(0.5f,0.8f,1), false); }
                    else if (z.type==ZoneType::Inferno){ enemies[i].status.burn=std::max(enemies[i].status.burn,1.0f);
                        enemies[i].status.burnDps=z.dps; damageEnemy((int)i, z.dps*0.5f, glm::vec3(1,0.4f,0.1f), false); }
                    else if (z.type==ZoneType::Grasp){ damageEnemy((int)i, z.dps*0.5f, glm::vec3(0.5f,0.2f,0.6f), false); }
                }
            }
            // inferno zone also damages player if standing in it (boss slam)
            if (z.type==ZoneType::Inferno && glm::length(player.pos-z.pos)<z.radius)
                player.hp -= z.dps*0.4f;
        }
        // visuals
        if (frand()<0.6f){
            float a=frand()*6.28f, r=frand()*z.radius;
            glm::vec3 pp = z.pos+glm::vec3(std::cos(a)*r,0.2f,std::sin(a)*r);
            glm::vec4 col;
            if (z.type==ZoneType::Frost) col=glm::vec4(0.5f,0.85f,1,0.6f);
            else if (z.type==ZoneType::Inferno) col=glm::vec4(1,0.4f,0.1f,0.7f);
            else col=glm::vec4(0.4f,0.2f,0.5f,0.7f);
            particles.spawn(pp, glm::vec3(0, z.type==ZoneType::Grasp?-0.5f:1.0f, 0), col, 0.3f, 0.6f, z.type==ZoneType::Grasp?-1.0f:-0.5f);
        }
        if (z.duration<=0) z.alive=false;
    }
    zones.erase(std::remove_if(zones.begin(),zones.end(),
        [](const Zone& z){ return !z.alive; }), zones.end());
}

void Game::damageEnemy(int idx, float dmg, glm::vec3 hitColor, bool spreadChain){
    if (idx<0 || idx>=(int)enemies.size()) return;
    Enemy& e = enemies[idx];
    if (!e.alive) return;
    bool crit = frand() < player.critChance;
    float final = crit ? dmg*2.0f : dmg;
    e.hp -= final;
    e.hitFlash = 1.0f;
    e.anim.triggerAction(ActionState::Hurt, 0.3f);
    floatTexts.push_back({e.pos+glm::vec3(frand2()*0.3f,e.radius*3.0f,0),
        std::to_string((int)final)+(crit?"!":""),
        glm::vec4(hitColor, 1), 0.9f, 0.9f});
    particles.burst(e.pos+glm::vec3(0,1.0f,0), glm::vec4(hitColor,1), 8, 0.3f, 2.5f, 0.2f, 0.5f);

    // chain link: spread normal damage to chained neighbors
    if (spreadChain && e.status.chained)
        spreadChainDamage(idx, final*0.6f);

    if (e.hp<=0) killEnemy(idx);
}

void Game::spreadChainDamage(int srcIdx, float dmg){
    glm::vec3 src = enemies[srcIdx].pos;
    for (size_t i=0;i<enemies.size();++i){
        if ((int)i==srcIdx || !enemies[i].alive) continue;
        if (!enemies[i].status.chained) continue;
        if (glm::length(enemies[i].pos-src) < 7.0f){ // only nearby links
            enemies[i].hp -= dmg;
            enemies[i].hitFlash = 1.0f;
            floatTexts.push_back({enemies[i].pos+glm::vec3(0,enemies[i].radius*3.0f,0),
                std::to_string((int)dmg), glm::vec4(0.6f,0.9f,1,1), 0.8f, 0.8f});
            if (enemies[i].hp<=0) killEnemy((int)i);
        }
    }
}

void Game::killEnemy(int idx){
    Enemy& e = enemies[idx];
    if (!e.alive) return;
    e.alive=false; e.deathTimer=0;
    e.anim.triggerAction(ActionState::Die, 1.2f);
    gainXP(e.xpReward);
    spawnPickup(e.pos, true, e.goldReward + (int)(frand()*e.goldReward));
    if (frand()<0.4f) spawnPickup(e.pos+glm::vec3(0.4f,0,0.4f), false, e.xpReward/2);
    particles.burst(e.pos+glm::vec3(0,1,0), glm::vec4(0.8f,0.7f,0.3f,1), 25, 0.4f, 4.0f, 0.3f, 0.8f);
    if (lockTarget==idx) lockTarget=-1;
    if (e.isBoss){ state = GameState::Victory; }
}

void Game::gainXP(int amount){
    player.xp += amount;
    floatTexts.push_back({player.pos+glm::vec3(frand2(),2.2f,0),"+"+std::to_string(amount)+"XP",glm::vec4(0.5f,1,0.6f,1),1.0f,1.0f});
    while (player.xp >= player.xpToNext) { player.xp -= player.xpToNext; levelUp(); }
}

void Game::levelUp(){
    player.level++;
    player.skillPoints++;
    player.xpToNext = (int)(player.xpToNext*1.35f);
    player.maxHp += 12; player.hp = player.maxHp;
    player.maxMana += 10; player.mana = player.maxMana;
    player.manaRegen += 0.6f;
    floatTexts.push_back({player.pos+glm::vec3(0,3,0),"LEVEL UP!  LVL "+std::to_string(player.level),glm::vec4(1,0.9f,0.3f,1),2.0f,2.0f});
    particles.burst(player.pos+glm::vec3(0,1,0), glm::vec4(1,0.9f,0.4f,1), 60, 0.5f, 5.0f, 0.4f, -1.0f);
}

void Game::spawnPickup(glm::vec3 pos, bool gold, int amount){
    Pickup p; p.pos=pos+glm::vec3(0,0.5f,0); p.isGold=gold; p.amount=std::max(1,amount); p.bob=frand()*6.28f; p.alive=true;
    pickups.push_back(p);
}

void Game::useItem(int idx){
    if (idx<0||idx>=(int)inventory.size()) return;
    Item& it = inventory[idx];
    if (it.count<=0) return;
    switch (it.kind){
        case Item::Food: case Item::HealthPotion:
            player.hp = std::min(player.maxHp, player.hp+it.restore);
            floatTexts.push_back({player.pos+glm::vec3(0,2.4f,0),"+"+std::to_string((int)it.restore)+"HP",glm::vec4(0.4f,1,0.4f,1),1.0f,1.0f});
            break;
        case Item::Water: case Item::ManaPotion:
            player.mana = std::min(player.maxMana, player.mana+it.restore);
            floatTexts.push_back({player.pos+glm::vec3(0,2.4f,0),"+"+std::to_string((int)it.restore)+"MP",glm::vec4(0.4f,0.6f,1,1),1.0f,1.0f});
            break;
    }
    it.count--;
}

void Game::spawnEnemyBolt(glm::vec3 from, glm::vec3 to, float dmg, glm::vec3 color){
    Projectile p;
    p.type = ProjType::EnemyBolt;
    p.pos = from;
    glm::vec3 d = to - from;
    if (glm::length(d) < 0.01f) d = glm::vec3(0,0,1);
    p.vel = glm::normalize(d) * 18.0f;
    p.damage = dmg; p.color = color; p.hostile = true;
    p.radius = 0.0f; p.life = 4.0f;
    projectiles.push_back(p);
}

// ---------------- spawning ----------------
void Game::spawnEnemy(CharType t, glm::vec3 pos, float hp, float dmg, float spd, int xp, int gold){
    Enemy e;
    e.pos = pos; e.ctype=t; e.hp=e.maxHp=hp; e.damage=dmg; e.speed=spd;
    e.xpReward=xp; e.goldReward=gold;
    e.radius = (t==CharType::Brute)?0.8f:0.55f;
    e.attackTimer = 1.0f + frand();
    enemies.push_back(e);
}

void Game::spawnPhase(int phase){
    // gentler ramp: wave 1 = 3 enemies, grows by 2 each wave
    int count = 2 + phase;
    for (int i=0;i<count;i++){
        float a = frand()*6.2831853f;
        float dist = 24.0f + frand()*12.0f;
        glm::vec3 pos(std::cos(a)*dist, 0, std::sin(a)*dist);
        float roll = frand();
        if (phase>=3 && roll<0.22f)
            spawnEnemy(CharType::Brute, pos, 100+phase*18, 12+phase*2, 1.8f, 28+phase*4, 14);
        else if (phase>=2 && roll<0.38f)
            spawnEnemy(CharType::Caster, pos, 40+phase*8, 7+phase, 2.0f, 18+phase*2, 9);
        else
            spawnEnemy(CharType::Grunt, pos, 48+phase*10, 6+phase, 2.6f+frand()*0.6f, 14+phase*2, 6);
    }
}

void Game::spawnBoss(){
    Enemy e;
    e.pos = glm::vec3(0,0,-26); e.ctype=CharType::Boss; e.isBoss=true;
    e.hp=e.maxHp=1400; e.damage=26; e.speed=2.6f;
    e.xpReward=400; e.goldReward=300; e.radius=1.8f;
    e.attackTimer=2.0f; e.bossAbilityTimer=3.0f; e.bossPhase=1;
    enemies.push_back(e);
}
