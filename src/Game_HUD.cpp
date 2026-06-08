#include "Game.h"
#include <cmath>
#include <algorithm>

// project a world point to screen pixels; returns false if behind camera
static bool worldToScreen(const glm::vec3& w, const glm::mat4& vp, int sw, int sh, glm::vec2& out){
    glm::vec4 clip = vp * glm::vec4(w,1.0f);
    if (clip.w <= 0.01f) return false;
    glm::vec3 ndc = glm::vec3(clip)/clip.w;
    out.x = (ndc.x*0.5f+0.5f)*sw;
    out.y = (1.0f-(ndc.y*0.5f+0.5f))*sh;
    return ndc.z < 1.0f;
}

// Draw a small unique icon per ability inside a slot box (vector art, no textures)
static void drawAbilityIcon(UIRenderer& ui, AbilityId id, float x, float y, float s, glm::vec4 col){
    float cx=x+s*0.5f, cy=y+s*0.5f;
    glm::vec4 c = col;
    switch(id){
        case AbilityId::ChainLink: // linked dots
            ui.rect(x+s*0.18f,cy-2,s*0.18f,4,c);
            ui.rect(x+s*0.64f,cy-2,s*0.18f,4,c);
            ui.line2D(x+s*0.27f,cy,x+s*0.73f,cy,3,c);
            ui.rect(x+s*0.2f,cy-6,12,12,c); ui.rect(x+s*0.62f,cy-6,12,12,c);
            break;
        case AbilityId::Fireball: // flame teardrop
            ui.triangle({cx,y+s*0.18f},{x+s*0.3f,y+s*0.7f},{x+s*0.7f,y+s*0.7f},c);
            ui.rect(cx-s*0.18f,y+s*0.55f,s*0.36f,s*0.25f,c);
            break;
        case AbilityId::Frost: // snowflake
            for(int i=0;i<3;i++){ float a=i*1.0472f; ui.line2D(cx-std::cos(a)*s*0.3f,cy-std::sin(a)*s*0.3f,cx+std::cos(a)*s*0.3f,cy+std::sin(a)*s*0.3f,3,c);}
            break;
        case AbilityId::Inferno: // multi flames
            for(int i=0;i<3;i++){ float ox=(i-1)*s*0.22f; ui.triangle({cx+ox,y+s*0.25f},{cx+ox-s*0.12f,y+s*0.7f},{cx+ox+s*0.12f,y+s*0.7f},c);}
            break;
        case AbilityId::Poison: // droplet
            ui.triangle({cx,y+s*0.2f},{cx-s*0.22f,cy},{cx+s*0.22f,cy},c);
            ui.rect(cx-s*0.22f,cy,s*0.44f,s*0.28f,c);
            break;
        case AbilityId::GraspingHands: // upward fingers
            for(int i=0;i<4;i++){ float ox=x+s*0.25f+i*s*0.14f; ui.rect(ox,y+s*0.3f,5,s*0.45f,c);}
            ui.rect(x+s*0.25f,y+s*0.7f,s*0.55f,s*0.12f,c);
            break;
        case AbilityId::Meteor: // big circle with trail
            ui.rect(cx-s*0.2f,cy-s*0.2f,s*0.4f,s*0.4f,c);
            ui.line2D(x+s*0.15f,y+s*0.15f,cx,cy,4,c);
            break;
        case AbilityId::Daggers: // blade
            ui.triangle({cx,y+s*0.18f},{cx-s*0.12f,cy+s*0.1f},{cx+s*0.12f,cy+s*0.1f},c);
            ui.rect(cx-s*0.05f,cy+s*0.1f,s*0.1f,s*0.3f,c);
            ui.rect(cx-s*0.18f,cy+s*0.08f,s*0.36f,5,c);
            break;
        default: break;
    }
}

void Game::renderHUD(){
    ui.begin(width,height);
    glm::mat4 vp = cam.proj()*cam.view();
    float W=(float)width, H=(float)height;

    // ---------- enemy health bars + status timers (world-space) ----------
    for (auto& e : enemies){
        if (!e.alive) continue;
        glm::vec2 sp;
        if (worldToScreen(e.pos+glm::vec3(0,e.radius*4.0f+0.6f,0), vp, width,height, sp)){
            float bw = e.isBoss?260.0f:60.0f;
            float bh = e.isBoss?14.0f:6.0f;
            ui.bar(sp.x-bw/2, sp.y, bw, bh, e.hp/e.maxHp,
                glm::vec4(0.1f,0,0,0.7f), glm::vec4(0.9f,0.2f,0.2f,1), glm::vec4(0,0,0,0.8f));
            if (e.isBoss){
                ui.text("ARCHLICH  -  PHASE "+std::to_string(e.bossPhase), sp.x-bw/2, sp.y-22, 2.0f, glm::vec4(1,0.4f,0.4f,1));
            }
            // status duration labels above the bar
            float ty = sp.y-14; std::string st;
            if (e.status.freeze>0) st="FROZEN "+std::to_string((int)std::ceil(e.status.freeze))+"s";
            else if (e.status.burn>0) st="BURN "+std::to_string((int)std::ceil(e.status.burn))+"s";
            else if (e.status.grasp>0) st="GRASP "+std::to_string((int)std::ceil(e.status.grasp))+"s";
            if (!st.empty() && !e.isBoss) ui.text(st, sp.x-ui.textWidth(st,1.4f)/2, ty, 1.4f, glm::vec4(0.7f,0.9f,1,1));
            // poison/bleed stack count
            if (e.status.poisonStacks>0) ui.text("PSN x"+std::to_string(e.status.poisonStacks), sp.x+bw/2+4, sp.y-2, 1.3f, glm::vec4(0.4f,1,0.3f,1));
            if (e.status.bleedStacks>0) ui.text("BLD x"+std::to_string(e.status.bleedStacks), sp.x+bw/2+4, sp.y+8, 1.3f, glm::vec4(1,0.3f,0.3f,1));
        }
        // lock marker (chevrons) over locked target
        if (lockTarget>=0 && lockTarget<(int)enemies.size() && &e==&enemies[lockTarget]){
            glm::vec2 mk;
            if (worldToScreen(e.pos+glm::vec3(0,e.radius*4.0f+1.4f,0), vp, width,height, mk)){
                float t=2.0f+2.0f*std::sin(gameTime*6.0f);
                ui.triangle({mk.x,mk.y+12+t},{mk.x-12,mk.y-6+t},{mk.x+12,mk.y-6+t},glm::vec4(1,0.2f,0.2f,1));
                ui.triangle({mk.x,mk.y+6+t},{mk.x-7,mk.y-4+t},{mk.x+7,mk.y-4+t},glm::vec4(0.1f,0.1f,0.1f,1));
                ui.text("LOCKED", mk.x-ui.textWidth("LOCKED",1.6f)/2, mk.y-30, 1.6f, glm::vec4(1,0.3f,0.3f,1));
            }
        }
    }

    // ---------- floating combat text ----------
    for (auto& f : floatTexts){
        glm::vec2 sp;
        if (worldToScreen(f.worldPos, vp, width,height, sp)){
            float alpha = std::min(1.0f, f.life/f.maxLife*1.5f);
            glm::vec4 c = f.color; c.a*=alpha;
            float sc = 1.8f + (1.0f-f.life/f.maxLife)*0.8f;
            ui.text(f.text, sp.x-ui.textWidth(f.text,sc)/2, sp.y, sc, c);
        }
    }

    // ---------- top-left: vitals ----------
    glm::vec4 panelBg(0.05f,0.05f,0.08f,0.7f), border(0,0,0,0.9f);
    ui.rect(14,14,360,96,panelBg);
    ui.text("MAGE  LVL "+std::to_string(player.level), 26, 22, 2.2f, glm::vec4(0.9f,0.85f,1,1));
    ui.bar(26, 46, 320, 16, player.hp/player.maxHp, glm::vec4(0.2f,0,0,0.8f), glm::vec4(0.85f,0.15f,0.2f,1), border);
    ui.text("HP "+std::to_string((int)player.hp)+"/"+std::to_string((int)player.maxHp), 30, 47, 1.4f, glm::vec4(1,1,1,1));
    ui.bar(26, 68, 320, 16, player.mana/player.maxMana, glm::vec4(0,0.05f,0.2f,0.8f), glm::vec4(0.2f,0.4f,0.95f,1), border);
    ui.text("MP "+std::to_string((int)player.mana)+"/"+std::to_string((int)player.maxMana), 30, 69, 1.4f, glm::vec4(1,1,1,1));
    ui.bar(26, 90, 320, 10, (float)player.xp/player.xpToNext, glm::vec4(0,0.1f,0,0.8f), glm::vec4(0.4f,0.9f,0.4f,1), border);

    // ---------- top-right: gold, wave, fps ----------
    ui.rect(W-220,14,206,76,panelBg);
    ui.text("GOLD "+std::to_string(player.gold), W-208, 22, 2.0f, glm::vec4(1,0.85f,0.25f,1));
    std::string wave = bossSpawned?"BOSS FIGHT":("WAVE "+std::to_string(currentPhase)+"/"+std::to_string(totalPhases));
    ui.text(wave, W-208, 46, 1.8f, glm::vec4(1,0.6f,0.4f,1));
    ui.text("ENEMIES "+std::to_string(enemiesRemaining), W-208, 66, 1.6f, glm::vec4(0.8f,0.8f,0.9f,1));
    ui.text("FPS "+std::to_string(fps), W-90, 96, 1.4f, glm::vec4(0.5f,0.7f,0.5f,1));
    if (player.skillPoints>0)
        ui.text("[T] SKILL PTS: "+std::to_string(player.skillPoints), W-260, 96, 1.6f, glm::vec4(1,0.9f,0.3f,1));

    // ---------- crosshair when aiming ----------
    if (aiming){
        float cx=W*0.5f, cy=H*0.5f;
        glm::vec4 cc(abilities[selectedAbility].color,0.9f);
        ui.line2D(cx-14,cy,cx-5,cy,2,cc); ui.line2D(cx+5,cy,cx+14,cy,2,cc);
        ui.line2D(cx,cy-14,cx,cy-5,2,cc); ui.line2D(cx,cy+5,cx,cy+14,2,cc);
    }

    // ---------- bottom ability bar (the "levels/powers" shapes) ----------
    int n = (int)AbilityId::COUNT;
    float slot = 64.0f, gap = 8.0f;
    float totalW = n*slot + (n-1)*gap;
    float bx = (W-totalW)*0.5f;
    float by = H - slot - 24;
    ui.rect(bx-12, by-12, totalW+24, slot+40, glm::vec4(0.04f,0.04f,0.07f,0.8f));
    for (int i=0;i<n;i++){
        Ability& a = abilities[i];
        float x = bx + i*(slot+gap);
        bool sel = (i==selectedAbility);
        glm::vec4 fill = a.unlocked? glm::vec4(0.12f,0.12f,0.18f,0.95f) : glm::vec4(0.05f,0.05f,0.05f,0.95f);
        glm::vec4 bord = sel? glm::vec4(1,0.9f,0.3f,1) : glm::vec4(0.3f,0.3f,0.4f,1);
        ui.rectBorder(x,by,slot,slot,fill,bord, sel?3:2);
        // icon
        glm::vec4 iconCol = a.unlocked? glm::vec4(a.color,1) : glm::vec4(0.3f,0.3f,0.3f,1);
        drawAbilityIcon(ui, a.id, x+8, by+6, slot-16, iconCol);
        // number key
        ui.text(std::to_string(i+1), x+4, by+2, 1.6f, glm::vec4(1,1,1,0.9f));
        // short name
        ui.text(a.shortName, x+(slot-ui.textWidth(a.shortName,1.2f))/2, by+slot-12, 1.2f, glm::vec4(0.8f,0.8f,0.9f,1));
        // cooldown overlay
        if (a.cooldownTimer>0){
            float frac = a.cooldownTimer/a.cooldown;
            ui.rect(x, by, slot, slot*frac, glm::vec4(0,0,0,0.65f));
            ui.text(std::to_string((int)std::ceil(a.cooldownTimer)), x+slot/2-6, by+slot/2-8, 2.0f, glm::vec4(1,1,1,1));
        }
        // mana cost
        if (a.manaCost>0)
            ui.text(std::to_string((int)a.manaActual()), x+slot-18, by+2, 1.2f, glm::vec4(0.4f,0.6f,1,1));
        else
            ui.text("FREE", x+slot-26, by+2, 1.0f, glm::vec4(0.5f,1,0.5f,1));
        // level pips
        for (int L=0;L<a.level && L<5;L++) ui.rect(x+4+L*6, by+slot-4, 4,3, glm::vec4(1,0.9f,0.3f,1));
    }
    // selected ability name + hint
    Ability& sa = abilities[selectedAbility];
    ui.text(sa.name, (W-ui.textWidth(sa.name,2.0f))/2, by-30, 2.0f, glm::vec4(1,1,1,1));

    // ---------- controls hint (bottom-left) ----------
    ui.text("WASD MOVE  SPACE JUMP  SHIFT RUN  Q LOCK  RMB AIM  LMB CAST  T TREE  I BAG", 16, H-18, 1.2f, glm::vec4(0.6f,0.6f,0.7f,0.8f));

    // ---------- overlays ----------
    if (state==GameState::SkillTree) renderSkillTree();
    else if (state==GameState::Inventory) renderInventory();
    else if (state==GameState::Dead) renderDeathScreen();
    else if (state==GameState::Victory) renderVictoryScreen();
    else if (state==GameState::Paused) renderPauseMenu();

    ui.flush();
}

void Game::renderSkillTree(){
    float W=(float)width,H=(float)height;
    ui.rect(0,0,W,H,glm::vec4(0,0,0,0.78f));
    float pw=820, ph=560, px=(W-pw)/2, py=(H-ph)/2;
    ui.rectBorder(px,py,pw,ph,glm::vec4(0.06f,0.06f,0.1f,0.98f),glm::vec4(0.5f,0.4f,0.8f,1),3);
    ui.text("ABILITY TREE", px+24, py+18, 3.0f, glm::vec4(0.9f,0.8f,1,1));
    ui.text("SKILL POINTS: "+std::to_string(player.skillPoints), px+pw-260, py+24, 2.0f, glm::vec4(1,0.9f,0.3f,1));
    ui.text("PRESS 1-8 TO UPGRADE THAT SPELL   (P PASSIVE, ENTER/T CLOSE)", px+24, py+ph-30, 1.3f, glm::vec4(0.6f,0.6f,0.7f,1));

    int n=(int)AbilityId::COUNT;
    for (int i=0;i<n;i++){
        Ability& a=abilities[i];
        int col=i%2, row=i/2;
        float x=px+40+col*390, y=py+70+row*110;
        ui.rectBorder(x,y,360,96, glm::vec4(0.1f,0.1f,0.16f,1), glm::vec4(a.color,1),2);
        drawAbilityIcon(ui,a.id,x+8,y+8,80,glm::vec4(a.color,1));
        ui.text(a.name, x+96, y+10, 2.0f, glm::vec4(1,1,1,1));
        ui.text("LEVEL "+std::to_string(a.level), x+96, y+34, 1.6f, glm::vec4(1,0.9f,0.4f,1));
        ui.text("DMG "+std::to_string((int)a.dmg())+"  MANA "+std::to_string((int)a.manaActual()), x+96, y+54, 1.4f, glm::vec4(0.7f,0.8f,1,1));
        ui.text("UPGRADE: KEY "+std::to_string(i+1), x+96, y+74, 1.3f, glm::vec4(0.6f,1,0.6f,1));
        // level pips
        for (int L=0;L<a.level && L<8;L++) ui.rect(x+96+L*10, y+92, 7,4, glm::vec4(1,0.9f,0.3f,1));
    }
    // passive summary
    ui.text("PASSIVES: SPELL POWER x"+std::to_string((int)(player.spellPower*100))+"%   CRIT "+std::to_string((int)(player.critChance*100))+"%   PRESS P TO INVEST",
             px+24, py+ph-54, 1.4f, glm::vec4(0.8f,0.9f,1,1));
}

void Game::renderInventory(){
    float W=(float)width,H=(float)height;
    ui.rect(0,0,W,H,glm::vec4(0,0,0,0.78f));
    float pw=560, ph=440, px=(W-pw)/2, py=(H-ph)/2;
    ui.rectBorder(px,py,pw,ph,glm::vec4(0.06f,0.08f,0.06f,0.98f),glm::vec4(0.4f,0.7f,0.4f,1),3);
    ui.text("INVENTORY", px+24, py+18, 3.0f, glm::vec4(0.8f,1,0.8f,1));
    ui.text("PRESS 1-4 TO USE ITEM   (I CLOSE)", px+24, py+ph-30, 1.3f, glm::vec4(0.6f,0.7f,0.6f,1));
    for (size_t i=0;i<inventory.size();++i){
        Item& it=inventory[i];
        float y=py+70+i*78;
        ui.rectBorder(px+30,y,pw-60,66, glm::vec4(0.1f,0.14f,0.1f,1), glm::vec4(0.3f,0.5f,0.3f,1),2);
        glm::vec4 c = (it.kind==Item::Food||it.kind==Item::HealthPotion)?glm::vec4(0.9f,0.4f,0.4f,1):glm::vec4(0.4f,0.6f,1,1);
        ui.rect(px+42,y+14,40,40,c);
        ui.text(it.name, px+96, y+10, 2.0f, glm::vec4(1,1,1,1));
        std::string eff = (it.kind==Item::Food||it.kind==Item::HealthPotion)?"+"+std::to_string((int)it.restore)+" HP":"+"+std::to_string((int)it.restore)+" MP";
        ui.text(eff, px+96, y+34, 1.5f, c);
        ui.text("x"+std::to_string(it.count), px+pw-110, y+22, 2.2f, glm::vec4(1,0.9f,0.5f,1));
        ui.text("KEY "+std::to_string(i+1), px+pw-110, y+46, 1.3f, glm::vec4(0.6f,1,0.6f,1));
    }
}

void Game::renderPauseMenu(){
    float W=(float)width,H=(float)height;
    ui.rect(0,0,W,H,glm::vec4(0.02f,0.02f,0.05f,0.72f));
    // title
    ui.text("PAUSED", (W-ui.textWidth("PAUSED",5.0f))/2, H*0.26f, 5.0f, glm::vec4(0.9f,0.85f,1,1));
    ui.text("ARCANE ASCENDANT", (W-ui.textWidth("ARCANE ASCENDANT",2.0f))/2, H*0.26f+62, 2.0f, glm::vec4(0.6f,0.6f,0.8f,1));

    const char* labels[3] = {"RESUME","RESTART","EXIT"};
    float bw=320,bh=56,bx=(W-bw)/2;
    float y0=H*0.46f;
    for (int i=0;i<3;i++){
        float by=y0+i*72;
        bool hov = (pauseHover==i);
        glm::vec4 fill = hov? glm::vec4(0.22f,0.2f,0.4f,0.98f) : glm::vec4(0.1f,0.1f,0.16f,0.95f);
        glm::vec4 bord = hov? glm::vec4(1,0.9f,0.4f,1) : glm::vec4(0.4f,0.4f,0.6f,1);
        ui.rectBorder(bx,by,bw,bh,fill,bord, hov?3:2);
        glm::vec4 tc = hov? glm::vec4(1,1,1,1) : glm::vec4(0.8f,0.8f,0.9f,1);
        ui.text(labels[i], bx+(bw-ui.textWidth(labels[i],2.6f))/2, by+16, 2.6f, tc);
    }
    ui.text("ESC / ENTER RESUME    R RESTART    Q QUIT", (W-ui.textWidth("ESC / ENTER RESUME    R RESTART    Q QUIT",1.4f))/2, y0+3*72+20, 1.4f, glm::vec4(0.6f,0.6f,0.7f,0.9f));
}

void Game::renderLevelUp(){}

void Game::renderDeathScreen(){
    float W=(float)width,H=(float)height;
    ui.rect(0,0,W,H,glm::vec4(0.1f,0,0,0.7f));
    ui.text("YOU DIED", (W-ui.textWidth("YOU DIED",6.0f))/2, H*0.35f, 6.0f, glm::vec4(0.9f,0.15f,0.15f,1));
    ui.text("REACHED WAVE "+std::to_string(currentPhase)+"   LEVEL "+std::to_string(player.level)+"   GOLD "+std::to_string(player.gold),
            (W-ui.textWidth("REACHED WAVE 0   LEVEL 0   GOLD 0",2.0f))/2, H*0.5f, 2.0f, glm::vec4(1,1,1,1));
    ui.text("PRESS  R  TO RESTART", (W-ui.textWidth("PRESS  R  TO RESTART",2.4f))/2, H*0.6f, 2.4f, glm::vec4(1,0.9f,0.4f,1));
}

void Game::renderVictoryScreen(){
    float W=(float)width,H=(float)height;
    ui.rect(0,0,W,H,glm::vec4(0.05f,0.05f,0.15f,0.7f));
    ui.text("VICTORY", (W-ui.textWidth("VICTORY",6.0f))/2, H*0.32f, 6.0f, glm::vec4(1,0.85f,0.3f,1));
    ui.text("THE ARCHLICH IS DEFEATED", (W-ui.textWidth("THE ARCHLICH IS DEFEATED",2.6f))/2, H*0.46f, 2.6f, glm::vec4(0.9f,0.9f,1,1));
    ui.text("FINAL LEVEL "+std::to_string(player.level)+"   GOLD "+std::to_string(player.gold),
            (W-ui.textWidth("FINAL LEVEL 00   GOLD 0000",2.0f))/2, H*0.56f, 2.0f, glm::vec4(1,1,1,1));
    ui.text("PRESS  R  TO PLAY AGAIN", (W-ui.textWidth("PRESS  R  TO PLAY AGAIN",2.4f))/2, H*0.66f, 2.4f, glm::vec4(1,0.9f,0.4f,1));
}
