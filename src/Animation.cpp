#include "Animation.h"
#include <cmath>

void Animator::triggerAction(ActionState a, float duration) {
    action = a;
    actionTimer = 0.0f;
    actionDuration = duration;
}

void Animator::update(float dt, float speed01) {
    lastSpeed = speed01;
    float gaitSpeed = (loco == LocoState::Run) ? 11.0f : 7.0f;
    if (loco == LocoState::Walk || loco == LocoState::Run)
        phase += dt * gaitSpeed * (0.5f + speed01);
    else
        phase += dt * 2.0f; // idle breathing

    if (action != ActionState::None) {
        actionTimer += dt;
        if (actionTimer >= actionDuration && action != ActionState::Die)
            action = ActionState::None;
    }
}

Pose Animator::computePose() const {
    Pose p;
    float s = std::sin(phase);
    float c = std::cos(phase);

    switch (loco) {
        case LocoState::Idle: {
            p.bodyBob = std::sin(phase) * 0.012f;
            p.leftArmSwing = 0.05f + std::sin(phase) * 0.02f;
            p.rightArmSwing = 0.05f - std::sin(phase) * 0.02f;
            p.leftElbow = -0.15f; p.rightElbow = -0.15f;
            p.spineLean = 0.02f;
        } break;
        case LocoState::Walk: {
            float amp = 0.5f;
            p.leftLegSwing  =  s * amp;
            p.rightLegSwing = -s * amp;
            p.leftKnee  = std::max(0.0f, -s) * 0.7f;
            p.rightKnee = std::max(0.0f,  s) * 0.7f;
            p.leftArmSwing  = -s * 0.4f;
            p.rightArmSwing =  s * 0.4f;
            p.leftElbow = -0.3f; p.rightElbow = -0.3f;
            p.bodyBob = std::fabs(c) * 0.03f;
            p.spineLean = 0.10f;
        } break;
        case LocoState::Run: {
            float amp = 0.85f;
            p.leftLegSwing  =  s * amp;
            p.rightLegSwing = -s * amp;
            p.leftKnee  = std::max(0.0f, -s) * 1.1f;
            p.rightKnee = std::max(0.0f,  s) * 1.1f;
            p.leftArmSwing  = -s * 0.7f;
            p.rightArmSwing =  s * 0.7f;
            p.leftElbow = -0.8f; p.rightElbow = -0.8f;
            p.bodyBob = std::fabs(c) * 0.05f;
            p.spineLean = 0.22f;
        } break;
        case LocoState::Jump: {
            p.leftLegSwing = -0.3f; p.rightLegSwing = -0.3f;
            p.leftKnee = 0.6f; p.rightKnee = 0.6f;
            p.leftArmSwing = -0.5f; p.rightArmSwing = -0.5f;
            p.leftElbow = -0.4f; p.rightElbow = -0.4f;
            p.spineLean = -0.05f;
        } break;
        case LocoState::Fall: {
            p.leftLegSwing = 0.2f; p.rightLegSwing = -0.2f;
            p.leftKnee = 0.4f; p.rightKnee = 0.4f;
            p.leftArmRaise = 0.6f; p.rightArmRaise = 0.6f;
        } break;
    }

    // Additive action layer
    if (action != ActionState::None) {
        float t = actionProgress();
        // bell curve 0->1->0
        float bell = std::sin(t * (float)M_PI);
        switch (action) {
            case ActionState::Cast: {
                // wind the staff back (0..0.4) then thrust it forward (0.4..1)
                if (t < 0.4f) {
                    float k = t/0.4f;
                    p.rightArmRaise += k * 1.3f;     // lift staff overhead
                    p.rightElbow    += -k * 0.7f;
                    p.spineLean     += -k * 0.12f;   // lean back to charge
                } else {
                    float k = (t-0.4f)/0.6f;
                    p.rightArmRaise += (1.0f-k) * 1.3f + k*0.6f; // thrust forward
                    p.rightArmSwing += -k * 0.9f;                 // point at target
                    p.rightElbow    += -(1.0f-k) * 0.7f;
                    p.spineLean     += k * 0.25f;                 // lunge into it
                    p.headPitch     += -k * 0.15f;
                }
            } break;
            case ActionState::Throw: {
                // wind back then snap forward (knife)
                if (t < 0.4f) {
                    float k=t/0.4f;
                    p.rightArmRaise += k*0.9f; p.rightArmSwing += k*0.8f; // cock back
                    p.rightElbow    += -k*1.1f; p.bodyTilt += k*0.08f;
                } else {
                    float k=(t-0.4f)/0.6f;
                    p.rightArmSwing += -k*1.6f;          // whip forward
                    p.rightArmRaise += (1.0f-k)*0.9f + k*0.3f;
                    p.rightElbow    += -(1.0f-k)*1.1f;
                    p.spineLean     += k*0.2f;
                }
            } break;
            case ActionState::Slam: {
                // overhead melee swing: arms up (0..0.4) then chop down (0.4..1)
                if (t < 0.4f) {
                    float k=t/0.4f;
                    p.rightArmRaise += k*2.2f; p.leftArmRaise += k*1.4f;
                    p.spineLean     += -k*0.15f;   // rear back
                } else {
                    float k=(t-0.4f)/0.6f;
                    p.rightArmRaise += (1.0f-k)*2.2f;          // chop down fast
                    p.rightArmSwing += k*1.2f;
                    p.leftArmRaise  += (1.0f-k)*1.4f;
                    p.spineLean     += k*0.5f;                  // lean into the hit
                    p.headPitch     += k*0.2f;
                }
            } break;
            case ActionState::Hurt: {
                p.bodyTilt += bell * 0.25f;
                p.spineLean += bell * 0.15f;
            } break;
            case ActionState::Die: {
                float k = glm::clamp(t,0.f,1.f);
                p.bodyTilt += k * 1.4f;       // fall backwards
                p.bodyBob  += -k * 0.4f;
                p.leftKnee += k*0.8f; p.rightKnee += k*0.8f;
                p.scaleAll = 1.0f - k*0.1f;
            } break;
            default: break;
        }
    }
    return p;
}
