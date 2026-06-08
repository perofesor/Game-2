// Animation.h - Procedural keyframe-free animation.
// Computes a Pose from a locomotion state and a normalized phase using
// sinusoidal gaits and additive action layers (cast / throw / hurt / die).
#pragma once
#include "CharacterModel.h"

enum class LocoState { Idle, Walk, Run, Jump, Fall };
enum class ActionState { None, Cast, Throw, Slam, Hurt, Die };

class Animator {
public:
    LocoState loco = LocoState::Idle;
    ActionState action = ActionState::None;
    float phase = 0.0f;        // locomotion cycle phase (seconds)
    float actionTimer = 0.0f;  // counts up while an action plays
    float actionDuration = 0.6f;

    void update(float dt, float speed01); // speed01: 0..1 movement intensity
    void triggerAction(ActionState a, float duration);
    bool actionPlaying() const { return action != ActionState::None && actionTimer < actionDuration; }
    float actionProgress() const { return actionDuration > 0 ? glm::clamp(actionTimer/actionDuration,0.f,1.f) : 1.f; }

    Pose computePose() const;

private:
    float lastSpeed = 0.0f;
};
