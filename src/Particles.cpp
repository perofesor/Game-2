#include "Particles.h"
#include <cstdlib>

static float frand(){ return (float)rand()/(float)RAND_MAX; }
static float frand2(){ return frand()*2.0f-1.0f; }

void ParticleSystem::init() {
    float quad[] = { -0.5f,-0.5f,  0.5f,-0.5f,  0.5f,0.5f,  -0.5f,-0.5f,  0.5f,0.5f, -0.5f,0.5f };
    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&quadVbo);
    glGenBuffers(1,&instVbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, instVbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    // center(3) color(4) size(1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Inst),(void*)0);
    glVertexAttribDivisor(1,1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,4,GL_FLOAT,GL_FALSE,sizeof(Inst),(void*)offsetof(Inst,color));
    glVertexAttribDivisor(2,1);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3,1,GL_FLOAT,GL_FALSE,sizeof(Inst),(void*)offsetof(Inst,size));
    glVertexAttribDivisor(3,1);
    glBindVertexArray(0);
}

void ParticleSystem::spawn(glm::vec3 pos, glm::vec3 vel, glm::vec4 color, float size, float life, float gravity) {
    if (particles.size() > 6000) return; // cap for performance
    Particle p; p.pos=pos; p.vel=vel; p.color=color; p.size=size; p.life=life; p.maxLife=life; p.gravity=gravity;
    particles.push_back(p);
}

void ParticleSystem::burst(glm::vec3 c, glm::vec4 color, int count, float spread, float speed, float size, float life) {
    for (int i=0;i<count;i++){
        glm::vec3 dir = glm::normalize(glm::vec3(frand2(),frand2()*0.6f+0.3f,frand2()));
        glm::vec3 pos = c + glm::vec3(frand2(),frand2(),frand2())*spread;
        spawn(pos, dir*speed*(0.5f+frand()), color, size*(0.6f+frand()*0.8f), life*(0.6f+frand()*0.8f), 1.5f);
    }
}

void ParticleSystem::update(float dt) {
    for (size_t i=0;i<particles.size();){
        Particle& p = particles[i];
        p.life -= dt;
        if (p.life <= 0){ particles[i]=particles.back(); particles.pop_back(); continue; }
        p.vel.y -= p.gravity*dt;
        p.pos += p.vel*dt;
        float t = p.life/p.maxLife;
        p.color.a = t; // fade out
        ++i;
    }
}

void ParticleSystem::render() {
    if (particles.empty()) return;
    instData.clear();
    instData.reserve(particles.size());
    for (auto& p : particles)
        instData.push_back({p.pos, p.color, p.size});
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, instVbo);
    glBufferData(GL_ARRAY_BUFFER, instData.size()*sizeof(Inst), instData.data(), GL_DYNAMIC_DRAW);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)instData.size());
    glBindVertexArray(0);
}

void ParticleSystem::destroy() {
    if (instVbo) glDeleteBuffers(1,&instVbo);
    if (quadVbo) glDeleteBuffers(1,&quadVbo);
    if (vao) glDeleteVertexArrays(1,&vao);
}
