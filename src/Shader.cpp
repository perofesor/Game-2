#include "Shader.h"
#include <iostream>

bool Shader::checkCompile(GLuint shader, const char* type) {
    GLint success;
    char infoLog[1024];
    if (std::string(type) != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "[Shader] Compile error (" << type << "):\n" << infoLog << std::endl;
            return false;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "[Shader] Link error:\n" << infoLog << std::endl;
            return false;
        }
    }
    return true;
}

bool Shader::buildFromSource(const char* vertexSrc, const char* fragmentSrc) {
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexSrc, nullptr);
    glCompileShader(vertex);
    if (!checkCompile(vertex, "VERTEX")) return false;

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentSrc, nullptr);
    glCompileShader(fragment);
    if (!checkCompile(fragment, "FRAGMENT")) return false;

    id = glCreateProgram();
    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    glLinkProgram(id);
    bool ok = checkCompile(id, "PROGRAM");

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return ok;
}

void Shader::destroy() {
    if (id) { glDeleteProgram(id); id = 0; }
}
