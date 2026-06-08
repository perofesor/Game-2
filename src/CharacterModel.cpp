#include "CharacterModel.h"
#include <cmath>

static glm::mat4 T(glm::vec3 p){ return glm::translate(glm::mat4(1.0f), p); }
static glm::mat4 R(float a, glm::vec3 ax){ return glm::rotate(glm::mat4(1.0f), a, ax); }
static glm::mat4 S(glm::vec3 s){ return glm::scale(glm::mat4(1.0f), s); }

void CharacterModel::build(CharType t) {
    type = t;
    // Color schemes per archetype (richer, more saturated palette)
    glm::vec3 trim(0.0f);
    switch (t) {
        case CharType::Mage:
            robe = {0.22f, 0.18f, 0.52f}; accent = {1.0f, 0.82f, 0.28f}; skin = {0.90f,0.74f,0.60f}; trim={0.45f,0.30f,0.85f}; bodyScale = 1.0f; break;
        case CharType::Grunt:
            robe = {0.42f, 0.14f, 0.13f}; accent = {0.85f,0.35f,0.12f}; skin = {0.48f,0.38f,0.32f}; trim={0.25f,0.08f,0.08f}; bodyScale = 0.95f; break;
        case CharType::Caster:
            robe = {0.10f, 0.30f, 0.40f}; accent = {0.35f,0.90f,1.0f}; skin = {0.55f,0.60f,0.62f}; trim={0.06f,0.18f,0.28f}; bodyScale = 1.0f; break;
        case CharType::Brute:
            robe = {0.20f, 0.20f, 0.24f}; accent = {0.90f,0.40f,0.12f}; skin = {0.44f,0.34f,0.30f}; trim={0.12f,0.12f,0.14f}; bodyScale = 1.45f; break;
        case CharType::Boss:
            robe = {0.12f, 0.05f, 0.18f}; accent = {1.0f,0.22f,0.40f}; skin = {0.38f,0.20f,0.24f}; trim={0.30f,0.05f,0.20f}; bodyScale = 2.4f; break;
    }

    GeometryBuilder g;

    // ---- TORSO: tapered, robe-like (broad shoulders, narrow waist) ----
    g.clear();
    g.addTaperedCylinder(T({0,0,0}), 0.16f, 0.26f, 0.55f, robe, 16);  // waist->chest
    g.addEllipsoid(T({0,0.55f,0}), {0.28f,0.17f,0.23f}, robe, 16, 10); // chest/shoulders
    // flowing robe skirt that flares at the hem
    g.addTaperedCylinder(T({0,-0.05f,0}), 0.30f, 0.16f, 0.40f, robe*0.85f, 16);
    // chest trim / belt
    g.addTaperedCylinder(T({0,0.06f,0}), 0.18f, 0.18f, 0.06f, trim, 16);
    g.addEllipsoid(T({0,0.08f,0.16f}), {0.06f,0.06f,0.04f}, accent, 8, 6); // belt gem
    // collar
    g.addTaperedCylinder(T({0,0.66f,0}), 0.16f, 0.20f, 0.10f, trim, 14);
    if (t == CharType::Mage || t == CharType::Caster){
        // shoulder pads
        for (int i=-1;i<=1;i+=2)
            g.addEllipsoid(T({0.26f*i,0.62f,0}), {0.12f,0.08f,0.12f}, trim, 10, 6);
    }
    if (t == CharType::Brute){
        // heavy shoulder armor plates + back hump
        for (int i=-1;i<=1;i+=2)
            g.addEllipsoid(T({0.30f*i,0.64f,0}), {0.16f,0.12f,0.16f}, glm::vec3(0.28f,0.28f,0.32f), 10, 7);
        g.addEllipsoid(T({0,0.55f,-0.18f}), {0.24f,0.20f,0.18f}, glm::vec3(0.22f,0.22f,0.26f), 12, 8);
    }
    if (t == CharType::Boss) {
        // chest plate accent + crystal spikes + shoulder horns
        g.addEllipsoid(T({0,0.55f,0.13f}), {0.23f,0.15f,0.13f}, accent*0.55f, 14, 8);
        for (int i=-1;i<=1;i+=2){
            g.addCrystal(T({0.18f*i,0.64f,0.1f})*R(0.4f*i,{0,0,1}), 0.05f, 0.26f, accent, 6);
            g.addHorn(T({0.30f*i,0.66f,-0.05f})*R(-0.6f*i,{0,0,1}), 0.06f, 0.32f, 0.14f, glm::vec3(0.15f,0.05f,0.1f), 7, 7);
        }
        g.addEllipsoid(T({0,0.30f,0.18f}), {0.08f,0.10f,0.05f}, accent, 8, 6); // glowing core gem
    }
    torso = g.build();

    // ---- HEAD ----
    g.clear();
    g.addEllipsoid(T({0,0,0}), {0.12f,0.14f,0.12f}, skin, 14, 10);
    head = g.build();

    // ---- HOOD / HELM ----
    g.clear();
    if (t == CharType::Mage || t == CharType::Caster) {
        g.addTaperedCylinder(T({0,-0.05f,0})*R((float)M_PI, {1,0,0}), 0.16f, 0.02f, 0.32f, robe, 14); // pointed hood
        g.addEllipsoid(T({0,-0.04f,0}), {0.16f,0.12f,0.16f}, robe, 14, 8);
    } else if (t == CharType::Boss) {
        g.addEllipsoid(T({0,0,0}), {0.16f,0.16f,0.16f}, robe*1.2f, 14, 8);
        // big curved horns
        g.addHorn(T({0.09f,0.08f,0})*R(-0.5f,{0,0,1}), 0.05f, 0.42f, 0.18f, accent, 8, 8);
        g.addHorn(T({-0.09f,0.08f,0})*R(0.5f,{0,0,1}), 0.05f, 0.42f, 0.18f, accent, 8, 8);
    } else {
        g.addEllipsoid(T({0,0,0}), {0.15f,0.13f,0.15f}, robe, 12, 8);
        g.addHorn(T({0.07f,0.05f,0})*R(-0.3f,{0,0,1}), 0.03f, 0.16f, 0.05f, accent, 6, 6);
        g.addHorn(T({-0.07f,0.05f,0})*R(0.3f,{0,0,1}), 0.03f, 0.16f, 0.05f, accent, 6, 6);
    }
    hood = g.build();

    // ---- ARMS (upper + lower as capsules/tapered) ----
    auto buildArm = [&](Mesh& up, Mesh& low){
        g.clear(); g.addCapsule(T({0,-0.13f,0}), 0.055f, 0.18f, robe, 10, 4); up = g.build();
        g.clear(); g.addCapsule(T({0,-0.12f,0}), 0.045f, 0.16f, skin, 10, 4); low = g.build();
    };
    buildArm(upperArmL, lowerArmL);
    buildArm(upperArmR, lowerArmR);

    // ---- LEGS ----
    auto buildLeg = [&](Mesh& up, Mesh& low, Mesh& ft){
        g.clear(); g.addCapsule(T({0,-0.15f,0}), 0.07f, 0.22f, robe*0.85f, 10, 4); up = g.build();
        g.clear(); g.addCapsule(T({0,-0.15f,0}), 0.058f, 0.22f, robe*0.7f, 10, 4); low = g.build();
        g.clear(); g.addEllipsoid(T({0,0,0.04f}), {0.07f,0.05f,0.13f}, glm::vec3(0.12f,0.1f,0.1f), 10, 6); ft = g.build();
    };
    buildLeg(upperLegL, lowerLegL, footL);
    buildLeg(upperLegR, lowerLegR, footR);

    // ---- STAFF + ORB (mage/caster) ----
    g.clear();
    g.addTaperedCylinder(T({0,-0.5f,0}), 0.025f, 0.03f, 1.15f, glm::vec3(0.3f,0.2f,0.12f), 8);
    staff = g.build();
    g.clear();
    g.addCrystal(T({0,0,0}), 0.07f, 0.18f, accent, 6);
    g.addCrystal(T({0,0,0})*R((float)M_PI,{1,0,0}), 0.07f, 0.10f, accent, 6);
    orb = g.build();

    built = true;
}

glm::vec3 CharacterModel::handLocalOffset() const {
    return glm::vec3(0.32f, 1.15f, 0.2f) * bodyScale;
}

void CharacterModel::drawPart(Shader& sh, const Mesh& m, const glm::mat4& xf,
                              glm::vec3 emissive, float emissiveAmt) const {
    sh.setMat4("uModel", xf);
    sh.setVec3("uEmissive", emissive);
    sh.setFloat("uEmissiveAmt", emissiveAmt);
    m.draw();
}

void CharacterModel::draw(Shader& sh, const glm::mat4& rootIn, const Pose& pose,
                          glm::vec3 emissive, float emissiveAmt) const {
    // Root with body scale + bob + tilt
    glm::mat4 root = rootIn
        * S(glm::vec3(bodyScale * pose.scaleAll))
        * T({0, pose.bodyBob, 0})
        * R(pose.bodyTilt, {1,0,0});

    glm::mat4 spine = root * R(pose.spineLean, {1,0,0});

    drawPart(sh, torso, spine, emissive, emissiveAmt);

    // Head + hood on top of chest
    glm::mat4 neck = spine * T({0,0.78f,0}) * R(pose.headPitch, {1,0,0});
    drawPart(sh, head, neck, emissive, emissiveAmt);
    drawPart(sh, hood, neck * T({0,0.02f,0}), emissive, emissiveAmt);

    // Shoulders
    glm::mat4 shoulderL = spine * T({-0.24f, 0.6f, 0});
    glm::mat4 shoulderR = spine * T({ 0.24f, 0.6f, 0});

    // Left arm
    glm::mat4 uaL = shoulderL * R(pose.leftArmSwing, {1,0,0}) * R(-pose.leftArmRaise, {1,0,0}) * R(pose.armSpread, {0,0,1});
    drawPart(sh, upperArmL, uaL, emissive, emissiveAmt);
    glm::mat4 laL = uaL * T({0,-0.26f,0}) * R(pose.leftElbow, {1,0,0});
    drawPart(sh, lowerArmL, laL, emissive, emissiveAmt);

    // Right arm (casting / throwing arm)
    glm::mat4 uaR = shoulderR * R(pose.rightArmSwing, {1,0,0}) * R(-pose.rightArmRaise, {1,0,0}) * R(-pose.armSpread, {0,0,1});
    drawPart(sh, upperArmR, uaR, emissive, emissiveAmt);
    glm::mat4 laR = uaR * T({0,-0.26f,0}) * R(pose.rightElbow, {1,0,0});
    drawPart(sh, lowerArmR, laR, emissive, emissiveAmt);

    // Staff in right hand (mage/caster/boss)
    if (type == CharType::Mage || type == CharType::Caster || type == CharType::Boss) {
        glm::mat4 hand = laR * T({0,-0.2f,0.05f});
        drawPart(sh, staff, hand, emissive, emissiveAmt);
        // orb at top of staff, glows
        glm::mat4 orbXf = hand * T({0,0.62f,0});
        drawPart(sh, orb, orbXf, accent, 0.7f + emissiveAmt);
    }

    // Hips & legs
    glm::mat4 hipL = root * T({-0.1f, 0.05f, 0});
    glm::mat4 hipR = root * T({ 0.1f, 0.05f, 0});

    glm::mat4 ulL = hipL * R(pose.leftLegSwing, {1,0,0});
    drawPart(sh, upperLegL, ulL, emissive, emissiveAmt);
    glm::mat4 llL = ulL * T({0,-0.3f,0}) * R(pose.leftKnee, {1,0,0});
    drawPart(sh, lowerLegL, llL, emissive, emissiveAmt);
    drawPart(sh, footL, llL * T({0,-0.32f,0}), emissive, emissiveAmt);

    glm::mat4 ulR = hipR * R(pose.rightLegSwing, {1,0,0});
    drawPart(sh, upperLegR, ulR, emissive, emissiveAmt);
    glm::mat4 llR = ulR * T({0,-0.3f,0}) * R(pose.rightKnee, {1,0,0});
    drawPart(sh, lowerLegR, llR, emissive, emissiveAmt);
    drawPart(sh, footR, llR * T({0,-0.32f,0}), emissive, emissiveAmt);
}
