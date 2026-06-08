#include "Game.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <string>

static glm::mat4 T(glm::vec3 p){ return glm::translate(glm::mat4(1.0f), p); }
static glm::mat4 R(float a, glm::vec3 ax){ return glm::rotate(glm::mat4(1.0f), a, ax); }
static glm::mat4 S(glm::vec3 s){ return glm::scale(glm::mat4(1.0f), s); }
static float frand(){ return (float)rand()/(float)RAND_MAX; }

void Game::setupLighting(Shader& sh){
    sh.use();
    sh.setMat4("uView", cam.view());
    sh.setMat4("uProj", cam.proj());
    sh.setVec3("uViewPos", cam.position);
    // cool moonlit key light
    sh.setVec3("uLightDir", glm::normalize(glm::vec3(-0.35f,-0.80f,-0.45f)));
    sh.setVec3("uLightColor", glm::vec3(0.95f,1.00f,1.15f));
    sh.setVec3("uAmbient", glm::vec3(0.42f,0.45f,0.55f));
    sh.setVec3("uPointPos", player.pos+glm::vec3(0,1.5f,0));
    sh.setVec3("uPointColor", glm::vec3(0.55f,0.55f,1.25f));
    sh.setFloat("uPointStrength", 2.6f);
    sh.setVec3("uFogColor", glm::vec3(0.16f,0.18f,0.28f));
    sh.setFloat("uFogDensity", 0.0026f);
    sh.setVec3("uEmissive", glm::vec3(0));
    sh.setFloat("uEmissiveAmt", 0.0f);

    // brazier point lights (warm) - capped at 6
    int n = (int)std::min<size_t>(braziers.size(), 6);
    sh.setInt("uNumLights", n);
    float flick = 0.85f + 0.15f*std::sin(gameTime*9.0f);
    for (int i=0;i<n;i++){
        sh.setVec3("uLightPos["+std::to_string(i)+"]", braziers[i]+glm::vec3(0,1.7f,0));
        sh.setVec3("uLightCol["+std::to_string(i)+"]", glm::vec3(1.0f,0.55f,0.20f)*flick);
    }
}

void Game::render(){
    glViewport(0,0,width,height);
    glClearColor(0.07f,0.07f,0.11f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setupLighting(litShader);

    // Sky (drawn first, no fog impact since far)
    glDepthMask(GL_FALSE);
    litShader.setMat4("uModel", T(cam.position));
    litShader.setFloat("uFogDensity", 0.0f);
    skyMesh.draw();
    litShader.setFloat("uFogDensity", 0.0026f);
    glDepthMask(GL_TRUE);

    renderWorld();
    renderCharacters();
    renderEffects();
    renderHUD();
}

void Game::renderWorld(){
    litShader.use();
    litShader.setVec3("uEmissive", glm::vec3(0));
    litShader.setFloat("uEmissiveAmt", 0.0f);

    // ground
    litShader.setMat4("uModel", glm::mat4(1.0f));
    groundMesh.draw();

    // ring of ruined pillars
    for (auto& p : pillars){
        litShader.setVec3("uEmissive", glm::vec3(0));
        litShader.setFloat("uEmissiveAmt", 0.0f);
        litShader.setMat4("uModel", T(p));
        pillarMesh.draw();
    }

    // scattered plains props (trees / rocks / crystals)
    for (auto& pr : props){
        glm::mat4 m = T(pr.pos)*R(pr.rot,{0,1,0})*S(glm::vec3(pr.scale));
        litShader.setMat4("uModel", m);
        if (pr.kind==0){ litShader.setVec3("uEmissive",glm::vec3(0)); litShader.setFloat("uEmissiveAmt",0.0f); treeMesh.draw(); }
        else if (pr.kind==1){ litShader.setVec3("uEmissive",glm::vec3(0)); litShader.setFloat("uEmissiveAmt",0.0f); rockMesh.draw(); }
        else { // crystals glow
            litShader.setVec3("uEmissive", glm::vec3(0.35f,0.6f,1.0f));
            litShader.setFloat("uEmissiveAmt", 0.5f+0.15f*std::sin(gameTime*2.0f+pr.pos.x));
            crystalMesh.draw();
        }
    }

    // braziers with glowing embers
    for (auto& b : braziers){
        litShader.setVec3("uEmissive", glm::vec3(0));
        litShader.setFloat("uEmissiveAmt", 0.0f);
        litShader.setMat4("uModel", T(b));
        // draw stand + bowl normally, embers emissive (last submesh handled via emissive call)
        litShader.setVec3("uEmissive", glm::vec3(1.0f,0.55f,0.18f));
        litShader.setFloat("uEmissiveAmt", 0.7f+0.2f*std::sin(gameTime*6.0f+b.x));
        brazierMesh.draw();
        // fire particles
        if (frand()<0.7f)
            particles.spawn(b+glm::vec3(frand()*0.4f-0.2f,1.75f,frand()*0.4f-0.2f),
                            glm::vec3(0,2.2f,0), glm::vec4(1.0f,0.5f+frand()*0.3f,0.1f,0.9f), 0.35f, 0.6f, -1.5f);
    }
    litShader.setVec3("uEmissive", glm::vec3(0));
    litShader.setFloat("uEmissiveAmt", 0.0f);
}

void Game::drawEnemyModel(Enemy& e){
    CharacterModel* m = &gruntModel;
    switch (e.ctype){
        case CharType::Grunt:  m=&gruntModel; break;
        case CharType::Caster: m=&casterModel; break;
        case CharType::Brute:  m=&bruteModel; break;
        case CharType::Boss:   m=&bossModel; break;
        default: m=&gruntModel; break;
    }
    float sink = e.graspSink;
    glm::mat4 root = T(e.pos - glm::vec3(0,sink,0)) * R(e.facing, {0,1,0});

    glm::vec3 emissive(0); float emAmt=0;
    if (e.hitFlash>0){ emissive=glm::vec3(1,0.3f,0.3f); emAmt=e.hitFlash*0.6f; }
    if (e.status.freeze>0){ emissive=glm::vec3(0.5f,0.8f,1.0f); emAmt=0.4f; }
    else if (e.status.burn>0){ emissive=glm::vec3(1.0f,0.4f,0.1f); emAmt=0.3f; }
    else if (e.status.poisonStacks>0){ emissive=glm::vec3(0.3f,0.9f,0.2f); emAmt=0.25f+0.05f*e.status.poisonStacks; }

    Pose pose = e.anim.computePose();
    m->draw(litShader, root, pose, emissive, emAmt);
}

void Game::renderCharacters(){
    litShader.use();
    // enemies
    for (auto& e : enemies) drawEnemyModel(e);

    // player (mage)
    glm::mat4 root = T(player.pos)*R(player.facing,{0,1,0});
    glm::vec3 em(0); float emAmt=0;
    if (player.anim.action==ActionState::Cast||player.anim.action==ActionState::Throw){
        em=glm::vec3(0.6f,0.5f,1.0f); emAmt=0.25f*std::sin(player.anim.actionProgress()*3.1415f);
    }
    Pose pp = player.anim.computePose();
    mageModel.draw(litShader, root, pp, em, emAmt);
}

void Game::renderEffects(){
    // ---- 3D lines: chain links + lock ring + aim indicator + grasp/zone rings ----
    lines.clear();

    // chain link: connect chained enemies that are near each other
    for (size_t i=0;i<enemies.size();++i){
        if (!enemies[i].alive || !enemies[i].status.chained) continue;
        for (size_t j=i+1;j<enemies.size();++j){
            if (!enemies[j].alive || !enemies[j].status.chained) continue;
            float d = glm::length(enemies[i].pos-enemies[j].pos);
            if (d < 7.0f){
                glm::vec3 a=enemies[i].pos+glm::vec3(0,1.2f,0);
                glm::vec3 b=enemies[j].pos+glm::vec3(0,1.2f,0);
                float pulse = 0.6f+0.4f*std::sin(gameTime*6.0f);
                lines.add(a,b,glm::vec4(0.5f,0.85f,1.0f,pulse));
            }
        }
    }
    // lock-on ring
    if (lockTarget>=0 && lockTarget<(int)enemies.size() && enemies[lockTarget].alive){
        glm::vec3 c = enemies[lockTarget].pos;
        float rr = enemies[lockTarget].radius*2.0f + 0.3f*std::sin(gameTime*5.0f);
        lines.ring(c+glm::vec3(0,0.1f,0), rr, glm::vec4(1,0.3f,0.3f,1), 28);
        lines.ring(c+glm::vec3(0,0.1f,0), rr*0.7f, glm::vec4(1,0.5f,0.2f,0.8f), 24);
    }
    // aim indicator
    if (aiming){
        Ability& a = abilities[selectedAbility];
        float rr = a.radius>0 ? a.radius : 0.8f;
        lines.ring(aimPoint+glm::vec3(0,0.05f,0), rr, glm::vec4(a.color,0.9f), 36);
        if (a.radius>0) lines.ring(aimPoint+glm::vec3(0,0.05f,0), rr*0.5f, glm::vec4(a.color,0.6f), 28);
    }
    // enemy attack telegraph (pulsing ring under an enemy that is about to strike)
    for (auto& e : enemies){
        if (!e.alive || !e.windingUp) continue;
        float pulse = 0.5f + 0.5f*std::sin(gameTime*18.0f);
        glm::vec4 col = e.attackIsRanged ? glm::vec4(0.5f,0.85f,1.0f,0.5f+0.4f*pulse)
                                         : glm::vec4(1.0f,0.35f,0.2f,0.5f+0.4f*pulse);
        lines.ring(e.pos+glm::vec3(0,0.08f,0), e.radius*1.8f, col, 26);
    }
    // zone rings
    for (auto& z : zones){
        glm::vec4 col = z.type==ZoneType::Frost?glm::vec4(0.5f,0.85f,1,0.7f)
                       :z.type==ZoneType::Inferno?glm::vec4(1,0.4f,0.1f,0.7f)
                       :glm::vec4(0.5f,0.2f,0.6f,0.7f);
        lines.ring(z.pos+glm::vec3(0,0.06f,0), z.radius, col, 40);
    }

    lineShader.use();
    lineShader.setMat4("uView", cam.view());
    lineShader.setMat4("uProj", cam.proj());
    glLineWidth(2.0f);
    lines.render();

    // ---- grasping hands geometry (rise from ground in grasp zones) ----
    litShader.use();
    setupLighting(litShader);
    for (auto& z : zones){
        if (z.type!=ZoneType::Grasp) continue;
        float t = 1.0f - z.duration/z.maxDuration;
        int hands = 6;
        for (int h=0;h<hands;h++){
            float a = (float)h/hands*6.28f + z.pos.x;
            float rr = z.radius*0.6f;
            glm::vec3 hp = z.pos+glm::vec3(std::cos(a)*rr,0,std::sin(a)*rr);
            float rise = std::sin(std::min(1.0f,t*1.5f)*3.1415f)*1.5f;
            litShader.setVec3("uEmissive", glm::vec3(0.3f,0.15f,0.4f));
            litShader.setFloat("uEmissiveAmt", 0.3f);
            // hand = forearm cylinder + 3 finger horns
            glm::mat4 base = T(hp+glm::vec3(0,rise-1.2f,0));
            litShader.setMat4("uModel", base);
            // reuse rock as palm-ish, fingers via horns from pillar? Build inline:
            // simple: use tapered cylinder mesh approximated by tree trunk scaled
            litShader.setMat4("uModel", base*S(glm::vec3(0.25f,1.2f,0.25f)));
            treeMesh.draw();
        }
    }
    litShader.setVec3("uEmissive", glm::vec3(0));
    litShader.setFloat("uEmissiveAmt", 0);

    // ---- particles ----
    particleShader.use();
    particleShader.setMat4("uView", cam.view());
    particleShader.setMat4("uProj", cam.proj());
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive glow
    particles.render();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);

    // ---- pickups (gold = gem, xp = crystal) drawn as small glowing crystals ----
    litShader.use();
    setupLighting(litShader);
    for (auto& p : pickups){
        glm::vec3 c = p.isGold?glm::vec3(1,0.8f,0.2f):glm::vec3(0.4f,1,0.5f);
        litShader.setMat4("uModel", T(p.pos+glm::vec3(0,std::sin(p.bob)*0.15f,0))*R(p.bob,{0,1,0})*S(glm::vec3(0.4f)));
        litShader.setVec3("uEmissive", c);
        litShader.setFloat("uEmissiveAmt", 0.6f);
        rockMesh.draw();
    }
    litShader.setVec3("uEmissive", glm::vec3(0));
    litShader.setFloat("uEmissiveAmt", 0);
}
