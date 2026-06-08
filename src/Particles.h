// Particles.h - Instanced billboard particle system for spell VFX.
#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>

struct Particle {
    glm::vec3 pos, vel;
    glm::vec4 color;
    float size, life, maxLife;
    float gravity = 0.0f;
};

class ParticleSystem {
public:
    std::vector<Particle> particles;
    GLuint vao=0, quadVbo=0, instVbo=0;

    void init();
    void spawn(glm::vec3 pos, glm::vec3 vel, glm::vec4 color, float size, float life, float gravity=0.0f);
    void burst(glm::vec3 center, glm::vec4 color, int count, float spread, float speed, float size, float life);
    void update(float dt);
    void render(); // uploads instance data; caller binds shader + sets view/proj
    size_t count() const { return particles.size(); }
    void destroy();

private:
    struct Inst { glm::vec3 center; glm::vec4 color; float size; };
    std::vector<Inst> instData;
};
