#include "LineRenderer.h"
#include <cmath>

void LineRenderer::init(){
    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(V),(void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(V),(void*)offsetof(V,c));
    glBindVertexArray(0);
}

void LineRenderer::ring(glm::vec3 c, float r, glm::vec4 col, int seg){
    for (int i=0;i<seg;i++){
        float a0 = (float)i/seg*6.2831853f;
        float a1 = (float)(i+1)/seg*6.2831853f;
        glm::vec3 p0 = c + glm::vec3(std::cos(a0)*r, 0, std::sin(a0)*r);
        glm::vec3 p1 = c + glm::vec3(std::cos(a1)*r, 0, std::sin(a1)*r);
        add(p0,p1,col);
    }
}

void LineRenderer::render(){
    if (verts.empty()) return;
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(V), verts.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINES, 0, (GLsizei)verts.size());
    glBindVertexArray(0);
}

void LineRenderer::destroy(){
    if (vbo) glDeleteBuffers(1,&vbo);
    if (vao) glDeleteVertexArrays(1,&vao);
}
