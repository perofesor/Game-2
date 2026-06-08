// CharacterModel.h - Articulated procedural character.
// Each body part is its own Mesh placed by a bone transform every frame,
// giving us real skeletal-style animation (walk, run, jump, cast, hurt, die)
// generated entirely in code. Used for the Mage (player), grunts, casters,
// and the multi-phase Boss (with a larger, hornd silhouette).
#pragma once
#include "Geometry.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

enum class CharType { Mage, Grunt, Caster, Brute, Boss };

// Animation pose computed per-frame: rotations (radians) for limbs + body offsets.
struct Pose {
    float spineLean = 0.0f;
    float leftLegSwing = 0.0f, rightLegSwing = 0.0f;
    float leftKnee = 0.0f, rightKnee = 0.0f;
    float leftArmSwing = 0.0f, rightArmSwing = 0.0f;
    float leftElbow = 0.0f, rightElbow = 0.0f;
    float rightArmRaise = 0.0f;   // cast / throw
    float leftArmRaise = 0.0f;
    float bodyBob = 0.0f;         // vertical bob
    float bodyTilt = 0.0f;        // hurt / roll
    float headPitch = 0.0f;
    float armSpread = 0.0f;       // boss slam
    float scaleAll = 1.0f;
};

struct BodyPart {
    Mesh mesh;
    // function-style: how this part is placed given the pose & root transform
};

class CharacterModel {
public:
    CharType type = CharType::Mage;
    glm::vec3 skin{0.85f, 0.72f, 0.6f};
    glm::vec3 robe{0.25f, 0.18f, 0.45f};
    glm::vec3 accent{0.9f, 0.75f, 0.2f};

    // Pre-built part meshes (built once)
    Mesh torso, head, hood, upperArmL, lowerArmL, upperArmR, lowerArmR;
    Mesh upperLegL, lowerLegL, upperLegR, lowerLegR, staff, orb, footL, footR;
    bool built = false;
    float bodyScale = 1.0f;

    void build(CharType t);
    // Draw the full animated character at a world transform with given pose.
    // `glowColor`/`glowAmt` lets us flash on cast/hurt. `setColor` is a callback
    // for per-draw uniforms handled by caller.
    void draw(Shader& sh, const glm::mat4& root, const Pose& pose,
              glm::vec3 emissive, float emissiveAmt) const;

    // Where the staff orb / right hand is in local space (for spell spawn point).
    glm::vec3 handLocalOffset() const;

private:
    void drawPart(Shader& sh, const Mesh& m, const glm::mat4& xf,
                  glm::vec3 emissive, float emissiveAmt) const;
};
