#include "Game.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

static float frand(){ return (float)rand()/(float)RAND_MAX; }
static float frand2(){ return frand()*2.0f-1.0f; }
static const float ARENA_LIMIT = 74.0f;

void Game::update(float dt) {
    gameTime += dt;
    // FPS counter
    fpsTimer += dt; frameCount++;
    if (fpsTimer >= 0.5f){ fps = (int)(frameCount/fpsTimer); frameCount=0; fpsTimer=0; }

    if (state == GameState::Playing) {
        updateCooldowns(dt);
        updatePlayer(dt);
        updateEnemies(dt);
        updateProjectiles(dt);
        updateZones(dt);
        updatePickups(dt);
        updatePhases(dt);
    }
    updateFloatTexts(dt);
    particles.update(dt);
    updateCamera();

    // reset per-frame mouse delta
    mouseDX = mouseDY = 0;
    mouseLeftPressed = false;
    for (int i=0;i<1024;i++) keysPrev[i]=keys[i];
}

void Game::updateCooldowns(float dt) {
    for (auto& a : abilities)
        if (a.cooldownTimer > 0) a.cooldownTimer -= dt;
    // mana / hp regen
    player.mana = std::min(player.maxMana, player.mana + player.manaRegen*dt);
    player.hp   = std::min(player.maxHp,   player.hp   + player.hpRegen*dt);
}

void Game::updatePlayer(float dt) {
    // camera-relative movement
    glm::vec3 fwd = cam.flatForward();
    glm::vec3 right = cam.flatRight();
    glm::vec3 move(0);
    if (keys[GLFW_KEY_W]) move += fwd;
    if (keys[GLFW_KEY_S]) move -= fwd;
    if (keys[GLFW_KEY_D]) move += right;
    if (keys[GLFW_KEY_A]) move -= right;

    bool running = keys[GLFW_KEY_LEFT_SHIFT] || keys[GLFW_KEY_RIGHT_SHIFT];
    float speed = running ? player.runSpeed : player.moveSpeed;
    float moving = glm::length(move);
    if (moving > 0.01f) {
        move = glm::normalize(move);
        player.pos.x += move.x * speed * dt;
        player.pos.z += move.z * speed * dt;
        // face movement direction (unless locked)
        float targetFacing = std::atan2(move.x, move.z);
        if (lockTarget>=0 && lockTarget<(int)enemies.size() && enemies[lockTarget].alive) {
            glm::vec3 d = enemies[lockTarget].pos - player.pos;
            targetFacing = std::atan2(d.x, d.z);
        }
        // smooth turn
        float diff = targetFacing - player.facing;
        while (diff > 3.14159f) diff -= 6.28318f;
        while (diff < -3.14159f) diff += 6.28318f;
        player.facing += diff * std::min(1.0f, dt*12.0f);
    } else if (lockTarget>=0 && lockTarget<(int)enemies.size() && enemies[lockTarget].alive) {
        glm::vec3 d = enemies[lockTarget].pos - player.pos;
        float tf = std::atan2(d.x, d.z);
        float diff = tf - player.facing;
        while (diff > 3.14159f) diff -= 6.28318f;
        while (diff < -3.14159f) diff += 6.28318f;
        player.facing += diff * std::min(1.0f, dt*10.0f);
    }

    // arena bounds
    float r = std::sqrt(player.pos.x*player.pos.x + player.pos.z*player.pos.z);
    if (r > ARENA_LIMIT) { player.pos.x *= ARENA_LIMIT/r; player.pos.z *= ARENA_LIMIT/r; }

    // jump + gravity
    if (keys[GLFW_KEY_SPACE] && player.grounded) {
        player.vel.y = 7.5f; player.grounded = false;
        player.anim.loco = LocoState::Jump;
    }
    player.vel.y -= 20.0f*dt;
    player.pos.y += player.vel.y*dt;
    if (player.pos.y <= 0) { player.pos.y = 0; player.vel.y = 0; player.grounded = true; }

    // animation state
    if (!player.grounded) {
        player.anim.loco = (player.vel.y > 0) ? LocoState::Jump : LocoState::Fall;
    } else if (moving > 0.01f) {
        player.anim.loco = running ? LocoState::Run : LocoState::Walk;
    } else {
        player.anim.loco = LocoState::Idle;
    }
    float spd01 = moving>0.01f ? (running?1.0f:0.6f) : 0.0f;
    player.anim.update(dt, spd01);

    // ----- input: ability selection (1..8) -----
    for (int i=0;i<8;i++){
        int key = GLFW_KEY_1 + i;
        if (keys[key] && !keysPrev[key]) selectedAbility = i;
    }
    // lock toggle
    if (keys[GLFW_KEY_Q] && !keysPrev[GLFW_KEY_Q]) toggleLock();
    // inventory / skill tree toggles
    if (keys[GLFW_KEY_I] && !keysPrev[GLFW_KEY_I]) state = (state==GameState::Inventory)?GameState::Playing:GameState::Inventory;
    if (keys[GLFW_KEY_T] && !keysPrev[GLFW_KEY_T]) state = (state==GameState::SkillTree)?GameState::Playing:GameState::SkillTree;

    // aiming with right mouse
    aiming = mouseRightHeld;
    if (aiming) aimPoint = groundAimPoint();

    // cast with left click (or while held for some)
    if (mouseLeftPressed) castSelected();

    // player aura particles (subtle)
    if (frand() < 0.3f) {
        glm::vec3 base = player.pos + glm::vec3(0,1.6f,0);
        particles.spawn(base + glm::vec3(frand2()*0.2f,frand2()*0.2f,frand2()*0.2f),
                        glm::vec3(0,0.4f,0), glm::vec4(0.5f,0.4f,0.9f,0.5f), 0.12f, 0.6f, 0.0f);
    }

    // death check
    if (player.hp <= 0) { state = GameState::Dead; }
}

void Game::updateCamera() {
    cam.aspect = (float)width/(float)std::max(1,height);
    cam.update(player.pos);
}

void Game::updateEnemies(float dt) {
    int aliveCount = 0;
    for (size_t i=0;i<enemies.size();++i){
        Enemy& e = enemies[i];
        if (!e.alive) {
            // play death animation, then fade
            e.deathTimer += dt;
            e.anim.update(dt, 0);
            continue;
        }
        aliveCount++;
        if (e.hitFlash > 0) e.hitFlash -= dt*3.0f;

        // ---- status effects ----
        Status& s = e.status;
        if (s.burn > 0) { s.burn -= dt; e.hp -= s.burnDps*dt;
            if (frand()<0.4f) particles.spawn(e.pos+glm::vec3(frand2()*0.3f,1.0f+frand()*1.2f,frand2()*0.3f),
                glm::vec3(0,1.5f,0), glm::vec4(1.0f,0.4f,0.1f,0.8f),0.2f,0.5f,-1.0f); }
        if (s.freeze > 0) { s.freeze -= dt; }
        if (s.poisonStacks > 0) {
            s.poisonTimer -= dt;
            float pdps = 4.0f * s.poisonStacks; // stacks => faster hp loss
            e.hp -= pdps*dt;
            if (frand()<0.3f) particles.spawn(e.pos+glm::vec3(frand2()*0.3f,1.0f+frand()*0.8f,frand2()*0.3f),
                glm::vec3(0,0.6f,0), glm::vec4(0.3f,0.9f,0.2f,0.7f),0.18f,0.6f,-0.5f);
            if (s.poisonTimer<=0){ s.poisonStacks=0; }
        }
        if (s.bleedStacks > 0) {
            s.bleedTimer -= dt;
            float bdps = 6.0f * s.bleedStacks;
            e.hp -= bdps*dt;
            if (frand()<0.3f) particles.spawn(e.pos+glm::vec3(frand2()*0.3f,0.8f+frand()*0.8f,frand2()*0.3f),
                glm::vec3(frand2()*0.5f,-1.0f,frand2()*0.5f), glm::vec4(0.7f,0.05f,0.05f,0.8f),0.16f,0.5f,1.0f);
            if (s.bleedTimer<=0) s.bleedStacks=0;
        }
        if (s.grasp > 0) {
            s.grasp -= dt;
            e.graspSink = std::min(1.2f, e.graspSink + dt*0.8f);
            e.hp -= 10.0f*dt; // crushed
        } else if (e.graspSink > 0) {
            e.graspSink = std::max(0.0f, e.graspSink - dt*1.5f);
        }
        if (e.hp <= 0) { killEnemy((int)i); continue; }

        // ---- movement AI (skip if frozen or grasped) ----
        bool canMove = (s.freeze<=0 && s.grasp<=0);
        glm::vec3 toPlayer = player.pos - e.pos;
        float dist = glm::length(glm::vec3(toPlayer.x,0,toPlayer.z));
        e.facing = std::atan2(toPlayer.x, toPlayer.z);

        bool ranged = (e.ctype==CharType::Caster);
        if (canMove) {
            float stopDist = e.isBoss ? 4.0f : (ranged ? 11.0f : 1.6f);
            float meleeRange = e.isBoss ? 5.5f : 2.4f;

            // ---- resolve a winding-up attack (damage lands on the strike frame) ----
            if (e.windingUp) {
                e.anim.loco = LocoState::Idle;
                e.windupTimer -= dt;
                if (e.windupTimer <= 0) {
                    e.windingUp = false;
                    if (e.attackIsRanged) {
                        // launch a visible hostile bolt toward the player's chest
                        glm::vec3 from = e.pos+glm::vec3(0,1.4f,0);
                        glm::vec3 to   = player.pos+glm::vec3(0,1.2f,0);
                        glm::vec3 col  = e.isBoss?glm::vec3(0.9f,0.2f,0.4f):glm::vec3(0.5f,0.85f,1.0f);
                        spawnEnemyBolt(from, to, e.damage, col);
                        particles.burst(from, glm::vec4(col,1), 12, 0.3f, 2.5f, 0.3f, 0.4f);
                    } else {
                        // melee strike: only connects if player still in range
                        glm::vec3 tp = player.pos - e.pos;
                        float d2 = glm::length(glm::vec3(tp.x,0,tp.z));
                        if (d2 < meleeRange) {
                            player.hp -= e.damage;
                            particles.burst(player.pos+glm::vec3(0,1.2f,0), glm::vec4(1,0.25f,0.2f,0.95f), 16, 0.4f, 3.0f, 0.3f, 0.4f);
                            floatTexts.push_back({player.pos+glm::vec3(0.3f,2.2f,0),"-"+std::to_string((int)e.damage),glm::vec4(1,0.35f,0.3f,1),0.8f,0.8f});
                        } else {
                            // whiffed – feedback dust
                            particles.burst(e.pos+glm::vec3(std::sin(e.facing),0.4f,std::cos(e.facing)), glm::vec4(0.6f,0.55f,0.5f,0.7f), 8, 0.3f, 2.0f, 0.3f, 0.4f);
                        }
                    }
                    e.attackTimer = e.isBoss ? 2.0f : (ranged ? 2.6f : 2.2f);
                }
            }
            // ---- otherwise: approach or begin an attack windup ----
            else if (dist > stopDist) {
                // approach with a little strafing so groups don't perfectly stack
                e.wanderAngle += dt*1.5f*(e.radius>0.7f?0.4f:1.0f);
                glm::vec3 dir = glm::normalize(glm::vec3(toPlayer.x,0,toPlayer.z));
                glm::vec3 strafe = glm::vec3(-dir.z,0,dir.x)*std::sin(e.wanderAngle)*0.35f;
                e.pos += (dir + strafe) * e.speed * dt;
                e.anim.loco = (e.speed>4.0f)?LocoState::Run:LocoState::Walk;
            } else {
                e.anim.loco = LocoState::Idle;
                e.attackTimer -= dt;
                if (e.attackTimer <= 0 && !e.windingUp) {
                    // begin a telegraphed attack (longer windup = more time to react)
                    e.windingUp = true;
                    e.attackIsRanged = ranged || (e.isBoss && frand()<0.5f);
                    if (e.attackIsRanged) {
                        e.windupTimer = 0.7f;
                        e.anim.triggerAction(ActionState::Cast, 0.85f);
                    } else {
                        e.windupTimer = e.isBoss ? 0.8f : 0.65f;
                        e.anim.triggerAction(ActionState::Slam, e.isBoss?0.9f:0.75f);
                    }
                }
            }
        } else {
            e.anim.loco = LocoState::Idle;
            e.windingUp = false;
        }

        // boss phase abilities
        if (e.isBoss) {
            e.bossAbilityTimer -= dt;
            float hpFrac = e.hp/e.maxHp;
            int newPhase = (hpFrac>0.66f)?1:(hpFrac>0.33f?2:3);
            if (newPhase != e.bossPhase){
                e.bossPhase = newPhase;
                floatTexts.push_back({e.pos+glm::vec3(0,e.radius*4.0f,0),
                    "BOSS PHASE "+std::to_string(newPhase)+"!", glm::vec4(1,0.3f,0.3f,1), 2.5f, 2.5f});
                particles.burst(e.pos+glm::vec3(0,2,0), glm::vec4(1,0.2f,0.3f,1), 60, 1.0f, 6.0f, 0.4f, 1.0f);
                e.speed += 0.8f;
            }
            if (e.bossAbilityTimer <= 0 && canMove) {
                e.bossAbilityTimer = std::max(2.0f, 5.0f - e.bossPhase*1.0f);
                e.anim.triggerAction(ActionState::Slam, 0.8f);
                // ground slam shockwave -> spawn fire zone at player
                Zone z; z.type=ZoneType::Inferno; z.pos=player.pos; z.radius=4.0f;
                z.duration=z.maxDuration=3.0f; z.dps=10.0f*e.bossPhase; z.alive=true;
                zones.push_back(z);
                floatTexts.push_back({player.pos+glm::vec3(0,2,0),"INCOMING!",glm::vec4(1,0.5f,0,1),1.2f,1.2f});
                // phase 3: summon adds
                if (e.bossPhase==3 && frand()<0.5f){
                    for(int k=0;k<2;k++){
                        float a=frand()*6.28f;
                        spawnEnemy(CharType::Grunt, e.pos+glm::vec3(std::cos(a)*4,0,std::sin(a)*4),
                                   40,8,3.0f,10,4);
                    }
                }
            }
        }

        // bounds for enemies
        float er = std::sqrt(e.pos.x*e.pos.x+e.pos.z*e.pos.z);
        if (er > ARENA_LIMIT+6) { e.pos.x*= (ARENA_LIMIT+6)/er; e.pos.z*=(ARENA_LIMIT+6)/er; }

        float spd01 = (e.anim.loco==LocoState::Run)?1.0f:(e.anim.loco==LocoState::Walk?0.6f:0.0f);
        e.anim.update(dt, spd01);
        s.chained = false; // reset; chain link rebuilt each frame in combat/render
    }
    enemiesRemaining = aliveCount;

    // purge fully-dead faded enemies
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        [](const Enemy& e){ return !e.alive && e.deathTimer > 2.0f; }), enemies.end());
    // fix lock index after erase
    if (lockTarget >= (int)enemies.size()) lockTarget = -1;
}

void Game::updatePhases(float dt) {
    if (phaseDelay > 0) { phaseDelay -= dt; return; }
    if (enemiesRemaining == 0) {
        if (!bossSpawned && currentPhase < totalPhases) {
            currentPhase++;
            if (currentPhase < totalPhases) {
                spawnPhase(currentPhase);
                floatTexts.push_back({player.pos+glm::vec3(0,4,0),
                    "WAVE "+std::to_string(currentPhase)+" / "+std::to_string(totalPhases),
                    glm::vec4(1,0.9f,0.4f,1), 2.5f, 2.5f});
            } else {
                spawnBoss();
                bossSpawned = true;
                floatTexts.push_back({player.pos+glm::vec3(0,5,0),"THE ARCHLICH AWAKENS",glm::vec4(1,0.2f,0.3f,1),3.5f,3.5f});
            }
            phaseDelay = 1.0f;
        } else if (bossSpawned && currentPhase >= totalPhases) {
            // boss dead -> victory
            state = GameState::Victory;
        }
    }
}

void Game::updateFloatTexts(float dt){
    for (auto& f : floatTexts){ f.life -= dt; f.worldPos.y += dt*0.8f; }
    floatTexts.erase(std::remove_if(floatTexts.begin(),floatTexts.end(),
        [](const FloatText& f){ return f.life<=0; }), floatTexts.end());
}

void Game::updatePickups(float dt){
    for (auto& p : pickups){
        if (!p.alive) continue;
        p.bob += dt*3.0f;
        glm::vec3 to = player.pos - p.pos;
        float d = glm::length(to);
        if (d < 2.5f){ p.pos += glm::normalize(to)*8.0f*dt; }
        if (d < 0.8f){
            p.alive=false;
            if (p.isGold){ player.gold += p.amount;
                floatTexts.push_back({p.pos+glm::vec3(0,1,0),"+"+std::to_string(p.amount)+"G",glm::vec4(1,0.85f,0.2f,1),1.0f,1.0f});
            } else { gainXP(p.amount); }
        }
    }
    pickups.erase(std::remove_if(pickups.begin(),pickups.end(),
        [](const Pickup& p){ return !p.alive; }), pickups.end());
}
