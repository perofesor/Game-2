#include "Game.h"
#include "Shaders.h"
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>

static glm::mat4 T(glm::vec3 p){ return glm::translate(glm::mat4(1.0f), p); }
static glm::mat4 R(float a, glm::vec3 ax){ return glm::rotate(glm::mat4(1.0f), a, ax); }
static float frand(){ return (float)rand()/(float)RAND_MAX; }

bool Game::init(GLFWwindow* win) {
    window = win;
    srand((unsigned)time(nullptr));
    glfwGetFramebufferSize(win, &width, &height);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!litShader.buildFromSource(VS_LIT, FS_LIT)) return false;
    if (!particleShader.buildFromSource(VS_PARTICLE, FS_PARTICLE)) return false;
    if (!lineShader.buildFromSource(VS_LINE, FS_LINE)) return false;

    ui.init();
    particles.init();
    lines.init();

    buildModels();
    buildWorld();
    buildPropLayout();
    buildAbilities();
    resetGame();

    lastTime = glfwGetTime();
    return true;
}

void Game::buildModels() {
    mageModel.build(CharType::Mage);
    gruntModel.build(CharType::Grunt);
    casterModel.build(CharType::Caster);
    bruteModel.build(CharType::Brute);
    bossModel.build(CharType::Boss);
}

void Game::buildWorld() {
    GeometryBuilder g;

    // ----- Ground: a stone arena disc + grassy surrounding plains + a glowing
    //       rune ring. Uses per-tile colour for a textured, lived-in look. -----
    g.clear();
    int tiles = 64; float tileSize = 2.6f; float half = tiles*tileSize*0.5f;
    for (int x=0;x<tiles;x++)
        for (int z=0;z<tiles;z++){
            float cx = -half + (x+0.5f)*tileSize;
            float cz = -half + (z+0.5f)*tileSize;
            float d = std::sqrt(cx*cx+cz*cz);
            // pseudo-noise from coordinates (cheap, deterministic)
            float n  = 0.5f + 0.5f*std::sin(cx*0.7f)*std::cos(cz*0.6f);
            float n2 = 0.5f + 0.5f*std::sin(cx*0.21f+cz*0.17f);
            glm::vec3 col;
            if (d < 22.0f){
                // central paved arena: warm flagstones with seams
                bool seam = (((int)std::floor(cx/2.6f)+(int)std::floor(cz/2.6f))&1);
                glm::vec3 a(0.40f,0.38f,0.37f), b(0.30f,0.29f,0.30f);
                col = glm::mix(a,b, seam?0.85f:0.15f) * (0.9f+0.2f*n);
                // faintly warmer toward the centre
                col = glm::mix(col, glm::vec3(0.46f,0.40f,0.33f), glm::clamp(1.0f-d/22.0f,0.0f,1.0f)*0.35f);
            } else if (d < 26.0f){
                // glowing rune border ring
                col = glm::mix(glm::vec3(0.28f,0.45f,0.62f), glm::vec3(0.38f,0.62f,0.90f), n);
            } else {
                // grassy mossy plains with dirt patches
                glm::vec3 grass(0.20f,0.38f,0.18f), grass2(0.26f,0.46f,0.22f), dirt(0.36f,0.28f,0.18f);
                col = glm::mix(grass, grass2, n);
                if (n2>0.78f) col = glm::mix(col, dirt, (n2-0.78f)/0.22f);
            }
            if (d > half*0.92f) col *= 0.6f; // darken far edges
            g.addGroundQuad(glm::vec3(cx,0,cz), tileSize*1.02f, col);
        }
    groundMesh = g.build();

    // ----- Trees: trunk + layered foliage, taller & richer -----
    g.clear();
    g.addTaperedCylinder(T({0,0,0}), 0.28f, 0.16f, 2.6f, glm::vec3(0.30f,0.20f,0.11f), 9);
    g.addTaperedCylinder(T({0.25f,1.4f,0.1f})*R(0.5f,{0,0,1}), 0.10f, 0.05f, 0.9f, glm::vec3(0.28f,0.18f,0.1f), 6);
    g.addEllipsoid(T({0,3.0f,0}), {1.2f,1.5f,1.2f}, glm::vec3(0.11f,0.34f,0.16f), 14, 9);
    g.addEllipsoid(T({0.5f,3.9f,0.25f}), {0.8f,1.0f,0.8f}, glm::vec3(0.15f,0.40f,0.20f), 12, 8);
    g.addEllipsoid(T({-0.4f,3.4f,-0.35f}), {0.8f,0.95f,0.8f}, glm::vec3(0.09f,0.30f,0.14f), 12, 8);
    g.addEllipsoid(T({0.1f,4.6f,0.0f}), {0.6f,0.7f,0.6f}, glm::vec3(0.17f,0.44f,0.22f), 10, 7);
    treeMesh = g.build();

    // ----- Rocks: irregular faceted ellipsoids with moss -----
    g.clear();
    g.addEllipsoid(T({0,0.3f,0})*R(0.4f,{1,0,1}), {0.95f,0.65f,0.85f}, glm::vec3(0.30f,0.30f,0.33f), 9, 6);
    g.addEllipsoid(T({0.6f,0.2f,0.3f}), {0.55f,0.45f,0.55f}, glm::vec3(0.26f,0.27f,0.30f), 8, 5);
    g.addEllipsoid(T({-0.4f,0.18f,-0.2f}), {0.45f,0.32f,0.5f}, glm::vec3(0.18f,0.28f,0.18f), 8, 5); // mossy top
    rockMesh = g.build();

    // ----- Pillars: ancient ruined columns (fluted) -----
    g.clear();
    g.addEllipsoid(T({0,0.15f,0}), {0.85f,0.22f,0.85f}, glm::vec3(0.40f,0.38f,0.34f), 14, 6); // base
    g.addTaperedCylinder(T({0,0.2f,0}), 0.6f, 0.52f, 5.2f, glm::vec3(0.46f,0.44f,0.40f), 14);
    // flutes
    for(int i=0;i<8;i++){ float a=i/8.0f*6.28f; g.addTaperedCylinder(T({std::cos(a)*0.55f,0.2f,std::sin(a)*0.55f}),0.06f,0.05f,5.0f,glm::vec3(0.38f,0.36f,0.33f),5);}
    g.addEllipsoid(T({0,5.4f,0}), {0.8f,0.3f,0.8f}, glm::vec3(0.50f,0.47f,0.42f), 14, 6); // capital
    pillarMesh = g.build();

    // ----- Brazier: a glowing fire bowl on a stand (lights the arena) -----
    g.clear();
    g.addTaperedCylinder(T({0,0,0}), 0.18f, 0.30f, 1.5f, glm::vec3(0.22f,0.20f,0.20f), 10);
    g.addEllipsoid(T({0,1.55f,0}), {0.5f,0.22f,0.5f}, glm::vec3(0.25f,0.22f,0.2f), 12, 6);
    g.addEllipsoid(T({0,1.7f,0}), {0.36f,0.18f,0.36f}, glm::vec3(1.0f,0.5f,0.15f), 10, 6); // embers (emissive at draw)
    brazierMesh = g.build();

    // ----- Crystal cluster: glowing mana crystals jutting from the ground -----
    g.clear();
    g.addCrystal(T({0,0.4f,0}), 0.22f, 1.4f, glm::vec3(0.4f,0.7f,1.0f), 6);
    g.addCrystal(T({0.3f,0.25f,0.1f})*R(0.5f,{0,0,1}), 0.14f, 0.9f, glm::vec3(0.5f,0.85f,1.0f), 6);
    g.addCrystal(T({-0.25f,0.3f,-0.15f})*R(-0.4f,{1,0,0}), 0.12f, 0.8f, glm::vec3(0.3f,0.6f,0.95f), 6);
    crystalMesh = g.build();

    // ----- Sky dome (gradient via vertex color, dusk/aurora) -----
    g.clear();
    int seg=28, rings=18; float Rr=240.0f;
    {
        std::vector<Vertex> vv; std::vector<unsigned int> ii;
        for (int i=0;i<=rings;i++){
            float v=(float)i/rings*(float)M_PI;
            float y=std::cos(v), rr=std::sin(v);
            float h = (y*0.5f+0.5f);
            glm::vec3 top(0.04f,0.06f,0.16f), mid(0.10f,0.10f,0.24f), bot(0.30f,0.20f,0.26f);
            glm::vec3 col = h>0.5f? glm::mix(mid,top,(h-0.5f)*2.0f) : glm::mix(bot,mid,h*2.0f);
            for (int j=0;j<=seg;j++){
                float u=(float)j/seg*2.0f*(float)M_PI;
                glm::vec3 p(rr*std::cos(u)*Rr, y*Rr, rr*std::sin(u)*Rr);
                Vertex vert; vert.position=p; vert.normal=-glm::normalize(p); vert.color=col;
                vv.push_back(vert);
            }
        }
        int stride=seg+1;
        for(int i=0;i<rings;i++)for(int j=0;j<seg;j++){
            unsigned int a=i*stride+j, b=(i+1)*stride+j;
            ii.push_back(a);ii.push_back(a+1);ii.push_back(b);
            ii.push_back(a+1);ii.push_back(b+1);ii.push_back(b);
        }
        skyMesh.upload(vv,ii);
    }
}

void Game::buildPropLayout() {
    props.clear(); braziers.clear(); pillars.clear();
    srand(20240607);

    // Ring of ruined pillars just outside the arena rune-ring
    int npil = 14;
    for (int i=0;i<npil;i++){
        float a = (float)i/npil*6.2831853f;
        if (i%5==0) continue; // a few toppled/missing for ruin feel
        pillars.push_back(glm::vec3(std::cos(a)*30.0f, 0, std::sin(a)*30.0f));
    }
    // Four braziers framing the arena (cardinal points)
    braziers.push_back({ 18, 0,  18});
    braziers.push_back({-18, 0,  18});
    braziers.push_back({ 18, 0, -18});
    braziers.push_back({-18, 0, -18});

    // Scatter trees / rocks / crystal clusters across the plains (d 32..95),
    // avoiding the central arena so the play space stays clear.
    auto frnd=[&](){ return (float)rand()/(float)RAND_MAX; };
    int count = 120;
    for (int i=0;i<count;i++){
        float a = frnd()*6.2831853f;
        float d = 32.0f + frnd()*60.0f;
        glm::vec3 p(std::cos(a)*d, 0, std::sin(a)*d);
        float r = frnd();
        int kind = r<0.55f?0 : (r<0.85f?1:2); // tree / rock / crystal
        props.push_back({kind, p, frnd()*6.28f, 0.7f+frnd()*1.0f});
    }
    // a cluster of crystals near the arena entrance for ambience
    for (int i=0;i<5;i++){
        float a = -1.2f + i*0.12f;
        props.push_back({2, glm::vec3(std::cos(a)*27.0f,0,std::sin(a)*27.0f), frnd()*6.28f, 0.8f+frnd()*0.6f});
    }
}

void Game::buildAbilities() {
    auto& A = abilities;
    A[0] = { AbilityId::ChainLink,    "Chain Link",      "CHAIN", 18, 6.0f, 0, 22,  6.0f, {0.6f,0.9f,1.0f}, 1, true, true };
    A[1] = { AbilityId::Fireball,     "Fire Bolt",       "BOLT",   0, 2.0f, 0, 28,  0.0f, {1.0f,0.5f,0.1f}, 1, true, true };
    A[2] = { AbilityId::Frost,        "Frost Nova",      "FROST", 30, 8.0f, 0, 14,  5.0f, {0.4f,0.8f,1.0f}, 1, true, true };
    A[3] = { AbilityId::Inferno,      "Inferno",         "FIRE",  35, 9.0f, 0, 12,  5.5f, {1.0f,0.35f,0.05f}, 1, true, true };
    A[4] = { AbilityId::Poison,       "Poison Cloud",    "POISN", 22, 4.0f, 0, 10,  3.5f, {0.3f,0.9f,0.2f}, 1, true, true };
    A[5] = { AbilityId::GraspingHands,"Grasping Hands",  "GRASP", 45, 12.0f,0, 30,  6.0f, {0.4f,0.2f,0.5f}, 1, true, true };
    A[6] = { AbilityId::Meteor,       "Meteor",          "METER", 70, 14.0f,0, 120, 7.0f, {1.0f,0.3f,0.0f}, 1, true, true };
    A[7] = { AbilityId::Daggers,      "Bleeding Dagger", "DAGGR", 15, 3.0f, 0, 24,  0.0f, {0.85f,0.85f,0.9f}, 1, true, true };
}

void Game::resetGame() {
    player = Player();
    player.pos = {0,0,0};
    player.maxHp = 100; player.hp = 100;
    player.maxMana = 100; player.mana = 100;
    enemies.clear(); projectiles.clear(); zones.clear();
    floatTexts.clear(); pickups.clear();
    inventory.clear();
    inventory.push_back({"Bread", Item::Food, 3, 30});
    inventory.push_back({"Water Flask", Item::Water, 3, 30});
    inventory.push_back({"Health Potion", Item::HealthPotion, 2, 60});
    inventory.push_back({"Mana Potion", Item::ManaPotion, 2, 60});

    buildAbilities();
    currentPhase = 0;
    bossSpawned = false;
    phaseDelay = 2.0f;
    state = GameState::Playing;
    lockTarget = -1;
    selectedAbility = 1;
    gameTime = 0;
}
