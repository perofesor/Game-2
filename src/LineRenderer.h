// LineRenderer.h - Batched 3D line drawing (chain links, target rings, decals).
#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>

class LineRenderer {
public:
    void init();
    void clear(){ verts.clear(); }
    void add(glm::vec3 a, glm::vec3 b, glm::vec4 color){
        verts.push_back({a,color}); verts.push_back({b,color});
    }
    void ring(glm::vec3 center, float r, glm::vec4 color, int seg=32);
    void render(); // caller binds shader + sets view/proj
    void destroy();
private:
    struct V{ glm::vec3 p; glm::vec4 c; };
    std::vector<V> verts;
    GLuint vao=0, vbo=0;
};
