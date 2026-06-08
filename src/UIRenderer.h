// UIRenderer.h - Immediate-mode 2D renderer for HUD: filled rects, bordered
// panels, gradient bars, and a built-in vector font (stroke-based, no asset files).
#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class UIRenderer {
public:
    int screenW=1280, screenH=720;
    void init();
    void begin(int w, int h);
    void rect(float x, float y, float w, float h, glm::vec4 color);
    void rectBorder(float x, float y, float w, float h, glm::vec4 fill, glm::vec4 border, float bw);
    // Horizontal bar with background + fill (e.g. health). fill01 0..1
    void bar(float x, float y, float w, float h, float fill01, glm::vec4 bg, glm::vec4 fg, glm::vec4 border);
    void triangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec4 color);
    // Vector text. scale ~ pixel height / 7.
    void text(const std::string& s, float x, float y, float scale, glm::vec4 color);
    float textWidth(const std::string& s, float scale);
    void line2D(float x0,float y0,float x1,float y1,float thick,glm::vec4 color);
    void flush();
    void destroy();

private:
    struct V { glm::vec2 p; glm::vec4 c; };
    std::vector<V> verts;
    GLuint vao=0, vbo=0, prog=0;
    void glyph(char ch, float x, float y, float scale, glm::vec4 color);
};
