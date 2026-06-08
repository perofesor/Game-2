// Game.h - Main game: player, world, combat, progression, UI, rendering.
#pragma once
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "CharacterModel.h"
#include "Animation.h"
#include "Particles.h"
#include "UIRenderer.h"
#include "LineRenderer.h"
#include "GameTypes.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <array>

enum class GameState { Playing, LevelUp, SkillTree, Inventory, Dead, Victory, Paused };

struct Player {
    glm::vec3 pos{0,0,0};
    glm::vec3 vel{0,0,0};
    float facing = 0.0f;
    float hp=100, maxHp=100;
    float mana=100, maxMana=100;
    float manaRegen=8.0f, hpRegen=1.5f;
    bool grounded=true;
    int level=1;
    int xp=0, xpToNext=100;
    int gold=0;
    int skillPoints=0;
    float moveSpeed=4.5f, runSpeed=8.0f;
    Animator anim;
    float spellPower=1.0f;     // passive multiplier
    float maxHpBonus=0, maxManaBonus=0;
    float critChance=0.05f;
};

class Game {
public:
    bool init(GLFWwindow* win);
    void run();
    void shutdown();

    // input callbacks (set from main)
    void onMouse(double x, double y);
    void onScroll(double dy);
    void onKey(int key, int action);
    void onMouseButton(int button, int action);
    void onResize(int w, int h);

private:
    GLFWwindow* window=nullptr;
    int width=1280, height=720;

    // Rendering
    Shader litShader, particleShader, lineShader;
    UIRenderer ui;
    ParticleSystem particles;
    LineRenderer lines;
    Mesh groundMesh, treeMesh, rockMesh, pillarMesh, skyMesh;
    Mesh brazierMesh, crystalMesh;

    struct Prop { int kind; glm::vec3 pos; float rot; float scale; };
    std::vector<Prop> props;        // trees/rocks/crystals scattered in plains
    std::vector<glm::vec3> braziers; // fire bowl positions (light + particles)
    std::vector<glm::vec3> pillars;  // ring columns
    void buildPropLayout();
    CharacterModel mageModel, gruntModel, casterModel, bruteModel, bossModel;

    Camera cam;

    // Game data
    Player player;
    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;
    std::vector<Zone> zones;
    std::vector<FloatText> floatTexts;
    std::vector<Pickup> pickups;
    std::array<Ability, (int)AbilityId::COUNT> abilities;
    std::vector<Item> inventory;

    GameState state = GameState::Playing;
    GameState prevState = GameState::Playing; // to restore after pause
    int pauseHover = -1;     // 0 = resume, 1 = restart, 2 = exit
    int selectedAbility = 1; // index 0..7, default fireball-ish
    bool aiming = false;
    glm::vec3 aimPoint{0,0,0};
    int lockTarget = -1;     // enemy index, -1 = none

    int currentPhase = 0;
    int totalPhases = 5;
    bool bossSpawned = false;
    float phaseDelay = 0.0f;
    int enemiesRemaining = 0;

    // input state
    bool keys[1024] = {false};
    bool keysPrev[1024] = {false};
    double lastMouseX=0, lastMouseY=0;
    bool firstMouse=true;
    float mouseDX=0, mouseDY=0;
    bool mouseLeftPressed=false, mouseRightHeld=false, mouseLeftHeld=false;
    float mouseSens=0.0032f;

    double lastTime=0;
    float gameTime=0;
    int frameCount=0; float fpsTimer=0; int fps=0;

    // ---- setup ----
    void buildAbilities();
    void buildWorld();
    void buildModels();
    void resetGame();

    // ---- loop ----
    void update(float dt);
    void render();
    void updatePlayer(float dt);
    void updateCamera();
    void updateEnemies(float dt);
    void updateProjectiles(float dt);
    void updateZones(float dt);
    void updatePickups(float dt);
    void updateFloatTexts(float dt);
    void updatePhases(float dt);
    void updateCooldowns(float dt);

    // ---- combat ----
    void castSelected();
    void fireProjectile(AbilityId id, glm::vec3 target);
    void spawnEnemyBolt(glm::vec3 from, glm::vec3 to, float dmg, glm::vec3 color);
    void applyAreaEffect(AbilityId id, glm::vec3 center);
    void damageEnemy(int idx, float dmg, glm::vec3 hitColor, bool spreadChain=true);
    void spreadChainDamage(int srcIdx, float dmg);
    void killEnemy(int idx);
    void toggleLock();
    glm::vec3 groundAimPoint();
    void aimRay(glm::vec3& outOrigin, glm::vec3& outDir);
    int nearestEnemy(glm::vec3 from, float maxDist=9999.0f);

    // ---- progression ----
    void gainXP(int amount);
    void levelUp();
    void spawnPickup(glm::vec3 pos, bool gold, int amount);
    void useItem(int idx);

    // ---- spawning ----
    void spawnEnemy(CharType t, glm::vec3 pos, float hp, float dmg, float spd, int xp, int gold);
    void spawnPhase(int phase);
    void spawnBoss();

    // ---- rendering helpers ----
    void renderWorld();
    void renderCharacters();
    void renderEffects();
    void renderHUD();
    void renderSkillTree();
    void renderInventory();
    void renderLevelUp();
    void renderDeathScreen();
    void renderVictoryScreen();
    void renderPauseMenu();
    void togglePause();
    void setupLighting(Shader& sh);

    void drawEnemyModel(Enemy& e);
    glm::vec3 screenToGround(double mx, double my);
};
