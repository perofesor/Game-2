// Shader.h - GLSL shader program wrapper (OpenGL 3.3 core, weak-GPU friendly)
#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

class Shader {
public:
    GLuint id = 0;

    Shader() = default;

    // Build from inline source strings (we embed shaders so the game is single-file portable)
    bool buildFromSource(const char* vertexSrc, const char* fragmentSrc);

    void use() const { glUseProgram(id); }
    void destroy();

    void setBool (const std::string& n, bool v)  const { glUniform1i(glGetUniformLocation(id, n.c_str()), (int)v); }
    void setInt  (const std::string& n, int v)   const { glUniform1i(glGetUniformLocation(id, n.c_str()), v); }
    void setFloat(const std::string& n, float v) const { glUniform1f(glGetUniformLocation(id, n.c_str()), v); }
    void setVec2 (const std::string& n, const glm::vec2& v) const { glUniform2fv(glGetUniformLocation(id, n.c_str()), 1, glm::value_ptr(v)); }
    void setVec3 (const std::string& n, const glm::vec3& v) const { glUniform3fv(glGetUniformLocation(id, n.c_str()), 1, glm::value_ptr(v)); }
    void setVec4 (const std::string& n, const glm::vec4& v) const { glUniform4fv(glGetUniformLocation(id, n.c_str()), 1, glm::value_ptr(v)); }
    void setMat4 (const std::string& n, const glm::mat4& m) const { glUniformMatrix4fv(glGetUniformLocation(id, n.c_str()), 1, GL_FALSE, glm::value_ptr(m)); }

private:
    bool checkCompile(GLuint shader, const char* type);
};
