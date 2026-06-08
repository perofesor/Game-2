// GameTypes.h - Core gameplay data: abilities, entities, effects.
#pragma once
#include <glm/glm.hpp>
#include "CharacterModel.h"
#include "Animation.h"
#include <string>
#include <vector>

// ---------------- Abilities (the 8 spells + passives) ----------------
enum class AbilityId {
    ChainLink = 0,   // 1: invisible link between nearby enemies; shared damage
    Fireball,        // 2: no mana, 2s cooldown, basic fire bolt
    Frost,           // 3: AoE freeze, shows duration over enemies
    Inferno,         // 4: AoE ground fire, enemies burn
    Poison,          // 5: stacking poison DoT (green)
    GraspingHands,   // 6: hands rise from ground, grab & drag enemies down
    Meteor,          // 7: huge fireball, hits up to 10, big damage, big mana
    Daggers,         // 8: thrown knife, bleed, stacking
    COUNT
};

struct Ability {
    AbilityId id;
    std::string name;
    std::string shortName;
    float manaCost;
    float cooldown;       // seconds
    float cooldownTimer = 0.0f;
    float baseDamage;
    float radius;         // AoE radius (0 = single)
    glm::vec3 color;
    int level = 1;        // upgrade level
    bool unlocked = true;
    bool aimed = true;    // requires right-click aim
    bool ready() const { return cooldownTimer <= 0.0f; }
    float dmg() const { return baseDamage * (1.0f + 0.35f*(level-1)); }
    float manaActual() const { return manaCost * (1.0f - 0.04f*(level-1)); }
};

// ---------------- Status effects on enemies ----------------
struct Status {
    float burn = 0.0f;        // remaining burn time
    float burnDps = 0.0f;
    float freeze = 0.0f;      // remaining freeze (frozen in place)
    int   poisonStacks = 0;
    float poisonTimer = 0.0f;
    int   bleedStacks = 0;
    float bleedTimer = 0.0f;
    float grasp = 0.0f;       // remaining grasp (dragged down)
    bool  chained = false;    // linked by ChainLink this frame
    int   chainGroup = -1;
};

// ---------------- Enemy ----------------
struct Enemy {
    glm::vec3 pos{0,0,0};
    glm::vec3 vel{0,0,0};
    float facing = 0.0f;
    float hp = 50, maxHp = 50;
    float speed = 2.5f;
    float attackTimer = 0.0f;
    float damage = 8.0f;
    float radius = 0.55f;
    bool alive = true;
    bool isBoss = false;
    CharType ctype = CharType::Grunt;
    int xpReward = 12;
    int goldReward = 5;
    Status status;
    Animator anim;
    float deathTimer = 0.0f;
    float hitFlash = 0.0f;
    float graspSink = 0.0f;     // visual sink into ground
    // attack telegraph: windup then strike (so damage feels timed to the swing)
    bool  windingUp = false;    // true while charging an attack
    float windupTimer = 0.0f;   // counts down; damage lands at 0
    bool  attackIsRanged = false;
    float wanderAngle = 0.0f;   // slight strafing so they don't clump
    // boss specifics
    int bossPhase = 1;
    float bossAbilityTimer = 0.0f;
    float bossSlamTimer = 0.0f;
};

// ---------------- Spell projectiles / effects ----------------
enum class ProjType { Fireball, Meteor, Dagger, EnemyBolt };
struct Projectile {
    ProjType type;
    glm::vec3 pos, vel;
    float damage;
    float radius;       // explosion radius
    bool alive = true;
    float life = 5.0f;
    glm::vec3 color;
    int maxTargets = 1;
    int sourceAbility = 0;
    glm::vec3 target{0,0,0};
    bool hostile = false;   // true = damages the player
};

// Persistent ground AoE zone (frost / inferno / grasp)
enum class ZoneType { Frost, Inferno, Grasp };
struct Zone {
    ZoneType type;
    glm::vec3 pos;
    float radius;
    float duration;
    float maxDuration;
    float tickTimer = 0.0f;
    float dps;
    bool alive = true;
};

// Floating combat text
struct FloatText {
    glm::vec3 worldPos;
    std::string text;
    glm::vec4 color;
    float life;
    float maxLife;
};

// Inventory item
struct Item {
    std::string name;
    enum Kind { Food, Water, ManaPotion, HealthPotion } kind;
    int count;
    float restore;
};

// Gold/XP pickup orb
struct Pickup {
    glm::vec3 pos;
    bool isGold;
    int amount;
    float bob;
    bool alive = true;
};
