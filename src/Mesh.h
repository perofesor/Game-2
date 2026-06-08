// Mesh.h - GPU mesh with positions, normals, colors. Built procedurally in code.
#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

class Mesh {
public:
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 0;

    Mesh() = default;
    void upload(const std::vector<Vertex>& verts, const std::vector<unsigned int>& indices);
    void draw() const;
    void destroy();
};
