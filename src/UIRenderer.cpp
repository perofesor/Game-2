#include "UIRenderer.h"
#include "Shaders.h"
#include "Shader.h"
#include <cmath>
#include <cctype>

// Stroke font: each glyph is a list of line segments on a 0..5 (x) by 0..7 (y) grid.
// Compact but readable. Lowercase mapped to uppercase.
struct Seg { float x0,y0,x1,y1; };
static std::vector<Seg> glyphSegs(char c) {
    c = (char)std::toupper((unsigned char)c);
    switch(c){
        case 'A': return {{0,0,0,5},{0,5,3,7},{3,7,5,5},{5,5,5,0},{0,3,5,3}};
        case 'B': return {{0,0,0,7},{0,7,4,7},{4,7,5,6},{5,6,5,4},{5,4,4,3},{0,3,4,3},{4,3,5,2},{5,2,5,1},{5,1,4,0},{0,0,4,0}};
        case 'C': return {{5,6,3,7},{3,7,1,7},{1,7,0,6},{0,6,0,1},{0,1,1,0},{1,0,3,0},{3,0,5,1}};
        case 'D': return {{0,0,0,7},{0,7,3,7},{3,7,5,5},{5,5,5,2},{5,2,3,0},{3,0,0,0}};
        case 'E': return {{5,7,0,7},{0,7,0,0},{0,0,5,0},{0,3.5f,4,3.5f}};
        case 'F': return {{5,7,0,7},{0,7,0,0},{0,3.5f,4,3.5f}};
        case 'G': return {{5,6,3,7},{3,7,1,7},{1,7,0,6},{0,6,0,1},{0,1,1,0},{1,0,4,0},{4,0,5,1},{5,1,5,3},{5,3,3,3}};
        case 'H': return {{0,0,0,7},{5,0,5,7},{0,3.5f,5,3.5f}};
        case 'I': return {{1,7,4,7},{1,0,4,0},{2.5f,0,2.5f,7}};
        case 'J': return {{4,7,4,1},{4,1,3,0},{3,0,1,0},{1,0,0,1}};
        case 'K': return {{0,0,0,7},{0,3.5f,5,7},{0,3.5f,5,0}};
        case 'L': return {{0,7,0,0},{0,0,5,0}};
        case 'M': return {{0,0,0,7},{0,7,2.5f,4},{2.5f,4,5,7},{5,7,5,0}};
        case 'N': return {{0,0,0,7},{0,7,5,0},{5,0,5,7}};
        case 'O': return {{1,0,4,0},{4,0,5,1},{5,1,5,6},{5,6,4,7},{4,7,1,7},{1,7,0,6},{0,6,0,1},{0,1,1,0}};
        case 'P': return {{0,0,0,7},{0,7,4,7},{4,7,5,6},{5,6,5,4},{5,4,4,3},{4,3,0,3}};
        case 'Q': return {{1,0,4,0},{4,0,5,1},{5,1,5,6},{5,6,4,7},{4,7,1,7},{1,7,0,6},{0,6,0,1},{0,1,1,0},{3,2,5,0}};
        case 'R': return {{0,0,0,7},{0,7,4,7},{4,7,5,6},{5,6,5,4},{5,4,4,3},{0,3,4,3},{2.5f,3,5,0}};
        case 'S': return {{5,6,3,7},{3,7,1,7},{1,7,0,6},{0,6,1,4},{1,4,4,3},{4,3,5,2},{5,2,4,0},{4,0,1,0},{1,0,0,1}};
        case 'T': return {{0,7,5,7},{2.5f,7,2.5f,0}};
        case 'U': return {{0,7,0,1},{0,1,1,0},{1,0,4,0},{4,0,5,1},{5,1,5,7}};
        case 'V': return {{0,7,2.5f,0},{2.5f,0,5,7}};
        case 'W': return {{0,7,1,0},{1,0,2.5f,4},{2.5f,4,4,0},{4,0,5,7}};
        case 'X': return {{0,0,5,7},{0,7,5,0}};
        case 'Y': return {{0,7,2.5f,4},{5,7,2.5f,4},{2.5f,4,2.5f,0}};
        case 'Z': return {{0,7,5,7},{5,7,0,0},{0,0,5,0}};
        case '0': return {{1,0,4,0},{4,0,5,1},{5,1,5,6},{5,6,4,7},{4,7,1,7},{1,7,0,6},{0,6,0,1},{0,1,1,0},{0,1,5,6}};
        case '1': return {{1,5,2.5f,7},{2.5f,7,2.5f,0},{1,0,4,0}};
        case '2': return {{0,6,1,7},{1,7,4,7},{4,7,5,6},{5,6,5,5},{5,5,0,0},{0,0,5,0}};
        case '3': return {{0,7,5,7},{5,7,2,4},{2,4,5,2},{5,2,4,0},{4,0,1,0},{1,0,0,1}};
        case '4': return {{4,0,4,7},{4,7,0,2},{0,2,5,2}};
        case '5': return {{5,7,0,7},{0,7,0,4},{0,4,4,4},{4,4,5,3},{5,3,5,1},{5,1,4,0},{4,0,1,0},{1,0,0,1}};
        case '6': return {{4,7,2,7},{2,7,0,5},{0,5,0,1},{0,1,1,0},{1,0,4,0},{4,0,5,1},{5,1,5,3},{5,3,4,4},{4,4,0,4}};
        case '7': return {{0,7,5,7},{5,7,1,0}};
        case '8': return {{1,4,1,7},{1,7,4,7},{4,7,4,4},{4,4,1,4},{1,4,1,0},{1,0,4,0},{4,0,4,4},{4,4,5,4},{1,4,0,4}};
        case '9': return {{5,3,5,6},{5,6,4,7},{4,7,1,7},{1,7,0,6},{0,6,0,4},{0,4,5,4}};
        case '.': return {{2,0,3,0},{2,0,2,1},{3,0,3,1},{2,1,3,1}};
        case ',': return {{2,1,3,1},{3,1,2,-1}};
        case ':': return {{2.5f,2,2.5f,3},{2.5f,5,2.5f,6}};
        case '-': return {{1,3.5f,4,3.5f}};
        case '+': return {{2.5f,2,2.5f,5},{1,3.5f,4,3.5f}};
        case '/': return {{0,0,5,7}};
        case '%': return {{0,0,5,7},{0.5f,6,1.5f,6},{0.5f,6,0.5f,5},{1.5f,6,1.5f,5},{0.5f,5,1.5f,5},{3.5f,2,4.5f,2},{3.5f,2,3.5f,1},{4.5f,2,4.5f,1},{3.5f,1,4.5f,1}};
        case '!': return {{2.5f,7,2.5f,2},{2.5f,0,2.5f,1}};
        case '?': return {{0,6,1,7},{1,7,4,7},{4,7,5,6},{5,6,5,5},{5,5,2.5f,3},{2.5f,3,2.5f,2},{2.5f,0,2.5f,1}};
        case '(': return {{3,7,1,5},{1,5,1,2},{1,2,3,0}};
        case ')': return {{2,7,4,5},{4,5,4,2},{4,2,2,0}};
        case '>': return {{1,7,4,3.5f},{4,3.5f,1,0}};
        case '<': return {{4,7,1,3.5f},{1,3.5f,4,0}};
        case '*': return {{2.5f,1,2.5f,6},{0.5f,2,4.5f,5},{4.5f,2,0.5f,5}};
        case ' ': return {};
        default:  return {{0,0,5,0},{5,0,5,7},{5,7,0,7},{0,7,0,0}}; // box for unknown
    }
}

void UIRenderer::init() {
    Shader s; s.buildFromSource(VS_UI, FS_UI); prog = s.id;
    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(V),(void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(V),(void*)offsetof(V,c));
    glBindVertexArray(0);
}

void UIRenderer::begin(int w, int h){ screenW=w; screenH=h; verts.clear(); }

void UIRenderer::rect(float x,float y,float w,float h,glm::vec4 c){
    verts.push_back({{x,y},c}); verts.push_back({{x+w,y},c}); verts.push_back({{x+w,y+h},c});
    verts.push_back({{x,y},c}); verts.push_back({{x+w,y+h},c}); verts.push_back({{x,y+h},c});
}

void UIRenderer::rectBorder(float x,float y,float w,float h,glm::vec4 fill,glm::vec4 border,float bw){
    rect(x,y,w,h,border);
    rect(x+bw,y+bw,w-2*bw,h-2*bw,fill);
}

void UIRenderer::bar(float x,float y,float w,float h,float f,glm::vec4 bg,glm::vec4 fg,glm::vec4 border){
    f = glm::clamp(f,0.0f,1.0f);
    rect(x-2,y-2,w+4,h+4,border);
    rect(x,y,w,h,bg);
    rect(x,y,w*f,h,fg);
}

void UIRenderer::triangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec4 col){
    verts.push_back({a,col}); verts.push_back({b,col}); verts.push_back({c,col});
}

void UIRenderer::line2D(float x0,float y0,float x1,float y1,float thick,glm::vec4 c){
    glm::vec2 d(x1-x0,y1-y0); float len=std::sqrt(d.x*d.x+d.y*d.y); if(len<1e-4f)return;
    glm::vec2 n(-d.y/len, d.x/len); n*=thick*0.5f;
    glm::vec2 p0(x0,y0),p1(x1,y1);
    verts.push_back({p0-n,c}); verts.push_back({p1-n,c}); verts.push_back({p1+n,c});
    verts.push_back({p0-n,c}); verts.push_back({p1+n,c}); verts.push_back({p0+n,c});
}

void UIRenderer::glyph(char ch, float x, float y, float scale, glm::vec4 color){
    auto segs = glyphSegs(ch);
    float thick = std::max(1.5f, scale*0.9f);
    for (auto& s : segs){
        // grid y is bottom-up; convert to screen (y down)
        float gx0=x+s.x0*scale, gy0=y+(7-s.y0)*scale;
        float gx1=x+s.x1*scale, gy1=y+(7-s.y1)*scale;
        line2D(gx0,gy0,gx1,gy1,thick,color);
    }
}

float UIRenderer::textWidth(const std::string& s, float scale){
    return s.size() * 7.0f * scale;
}

void UIRenderer::text(const std::string& s, float x, float y, float scale, glm::vec4 color){
    float cx=x;
    for (char ch : s){ glyph(ch,cx,y,scale,color); cx += 7.0f*scale; }
}

void UIRenderer::flush(){
    if (verts.empty()) return;
    glUseProgram(prog);
    glUniform2f(glGetUniformLocation(prog,"uScreen"), (float)screenW, (float)screenH);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(V), verts.data(), GL_DYNAMIC_DRAW);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);                 // 2D UI tris have mixed winding
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)verts.size());
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}

void UIRenderer::destroy(){
    if (vbo) glDeleteBuffers(1,&vbo);
    if (vao) glDeleteVertexArrays(1,&vao);
    if (prog) glDeleteProgram(prog);
}
