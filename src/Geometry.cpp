#include "Geometry.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void GeometryBuilder::pushVertex(const glm::mat4& xf, const glm::mat3& nrm,
                                 glm::vec3 p, glm::vec3 n, glm::vec3 c) {
    Vertex v;
    v.position = glm::vec3(xf * glm::vec4(p, 1.0f));
    v.normal   = glm::normalize(nrm * n);
    v.color    = c;
    verts.push_back(v);
}

// Capsule: cylinder body + two hemispherical caps. Aligned along +Y, centered at origin.
void GeometryBuilder::addCapsule(const glm::mat4& xf, float radius, float height,
                                 glm::vec3 color, int seg, int rings) {
    glm::mat3 nrm = glm::transpose(glm::inverse(glm::mat3(xf)));
    unsigned int base = (unsigned int)verts.size();
    float halfCyl = height * 0.5f;

    // Total latitudinal rings: bottom hemisphere + cylinder + top hemisphere
    int totalRings = rings * 2 + 2;
    for (int i = 0; i <= totalRings; ++i) {
        float t = (float)i / totalRings; // 0..1
        float y, r;
        glm::vec3 nBase;
        if (t < 0.25f) {
            // bottom hemisphere
            float a = (t / 0.25f) * (float)M_PI * 0.5f; // 0..pi/2
            y = -halfCyl - radius * std::cos(a);
            r = radius * std::sin(a);
            nBase = glm::vec3(0, -std::cos(a), 0); // approx; refined per-segment below
        } else if (t > 0.75f) {
            float a = ((t - 0.75f) / 0.25f) * (float)M_PI * 0.5f;
            y = halfCyl + radius * std::sin(a);
            r = radius * std::cos(a);
            nBase = glm::vec3(0, std::sin(a), 0);
        } else {
            float u = (t - 0.25f) / 0.5f;
            y = -halfCyl + u * height;
            r = radius;
            nBase = glm::vec3(0, 0, 0);
        }
        for (int j = 0; j <= seg; ++j) {
            float ph = (float)j / seg * 2.0f * (float)M_PI;
            float cx = std::cos(ph), cz = std::sin(ph);
            glm::vec3 p(r * cx, y, r * cz);
            glm::vec3 n = glm::normalize(glm::vec3(cx, nBase.y, cz));
            if (r < 1e-4f) n = glm::vec3(0, (y > 0 ? 1.f : -1.f), 0);
            pushVertex(xf, nrm, p, n, color);
        }
    }
    int stride = seg + 1;
    for (int i = 0; i < totalRings; ++i) {
        for (int j = 0; j < seg; ++j) {
            unsigned int a = base + i * stride + j;
            unsigned int b = base + (i + 1) * stride + j;
            indices.push_back(a); indices.push_back(b); indices.push_back(a + 1);
            indices.push_back(a + 1); indices.push_back(b); indices.push_back(b + 1);
        }
    }
}

void GeometryBuilder::addTaperedCylinder(const glm::mat4& xf, float r0, float r1,
                                         float height, glm::vec3 color, int seg) {
    glm::mat3 nrm = glm::transpose(glm::inverse(glm::mat3(xf)));
    unsigned int base = (unsigned int)verts.size();
    float slope = (r0 - r1) / height; // for normal tilt

    for (int i = 0; i <= 1; ++i) {
        float y = (i == 0) ? 0.0f : height;
        float r = (i == 0) ? r0 : r1;
        for (int j = 0; j <= seg; ++j) {
            float ph = (float)j / seg * 2.0f * (float)M_PI;
            float cx = std::cos(ph), cz = std::sin(ph);
            glm::vec3 p(r * cx, y, r * cz);
            glm::vec3 n = glm::normalize(glm::vec3(cx, slope, cz));
            pushVertex(xf, nrm, p, n, color);
        }
    }
    int stride = seg + 1;
    for (int j = 0; j < seg; ++j) {
        unsigned int a = base + j;
        unsigned int b = base + stride + j;
        indices.push_back(a); indices.push_back(b); indices.push_back(a + 1);
        indices.push_back(a + 1); indices.push_back(b); indices.push_back(b + 1);
    }
    // caps
    unsigned int cBot = (unsigned int)verts.size();
    pushVertex(xf, nrm, glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), color);
    for (int j = 0; j <= seg; ++j) {
        float ph = (float)j / seg * 2.0f * (float)M_PI;
        pushVertex(xf, nrm, glm::vec3(r0 * std::cos(ph), 0, r0 * std::sin(ph)), glm::vec3(0, -1, 0), color);
    }
    for (int j = 0; j < seg; ++j) { indices.push_back(cBot); indices.push_back(cBot + 1 + j + 1); indices.push_back(cBot + 1 + j); }
    unsigned int cTop = (unsigned int)verts.size();
    pushVertex(xf, nrm, glm::vec3(0, height, 0), glm::vec3(0, 1, 0), color);
    for (int j = 0; j <= seg; ++j) {
        float ph = (float)j / seg * 2.0f * (float)M_PI;
        pushVertex(xf, nrm, glm::vec3(r1 * std::cos(ph), height, r1 * std::sin(ph)), glm::vec3(0, 1, 0), color);
    }
    for (int j = 0; j < seg; ++j) { indices.push_back(cTop); indices.push_back(cTop + 1 + j); indices.push_back(cTop + 1 + j + 1); }
}

void GeometryBuilder::addEllipsoid(const glm::mat4& xf, glm::vec3 radii,
                                   glm::vec3 color, int seg, int rings) {
    glm::mat3 nrm = glm::transpose(glm::inverse(glm::mat3(xf)));
    unsigned int base = (unsigned int)verts.size();
    for (int i = 0; i <= rings; ++i) {
        float v = (float)i / rings * (float)M_PI; // 0..pi
        float y = std::cos(v);
        float rr = std::sin(v);
        for (int j = 0; j <= seg; ++j) {
            float u = (float)j / seg * 2.0f * (float)M_PI;
            glm::vec3 unit(rr * std::cos(u), y, rr * std::sin(u));
            glm::vec3 p = unit * radii;
            glm::vec3 n = glm::normalize(unit / radii); // ellipsoid normal
            pushVertex(xf, nrm, p, n, color);
        }
    }
    int stride = seg + 1;
    for (int i = 0; i < rings; ++i)
        for (int j = 0; j < seg; ++j) {
            unsigned int a = base + i * stride + j;
            unsigned int b = base + (i + 1) * stride + j;
            indices.push_back(a); indices.push_back(b); indices.push_back(a + 1);
            indices.push_back(a + 1); indices.push_back(b); indices.push_back(b + 1);
        }
}

void GeometryBuilder::addCrystal(const glm::mat4& xf, float radius, float height,
                                 glm::vec3 color, int seg) {
    glm::mat3 nrm = glm::transpose(glm::inverse(glm::mat3(xf)));
    unsigned int base = (unsigned int)verts.size();
    float mid = height * 0.45f;
    // middle belt
    for (int j = 0; j <= seg; ++j) {
        float ph = (float)j / seg * 2.0f * (float)M_PI;
        float cx = std::cos(ph), cz = std::sin(ph);
        pushVertex(xf, nrm, glm::vec3(radius * cx, mid, radius * cz),
                   glm::normalize(glm::vec3(cx, 0.2f, cz)), color);
    }
    unsigned int top = (unsigned int)verts.size();
    pushVertex(xf, nrm, glm::vec3(0, height, 0), glm::vec3(0, 1, 0), color);
    unsigned int bot = (unsigned int)verts.size();
    pushVertex(xf, nrm, glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), color);
    for (int j = 0; j < seg; ++j) {
        indices.push_back(base + j); indices.push_back(top); indices.push_back(base + j + 1);
        indices.push_back(base + j + 1); indices.push_back(bot); indices.push_back(base + j);
    }
}

void GeometryBuilder::addHorn(const glm::mat4& xf, float r0, float length,
                              float curve, glm::vec3 color, int seg, int rings) {
    glm::mat3 nrm = glm::transpose(glm::inverse(glm::mat3(xf)));
    unsigned int base = (unsigned int)verts.size();
    for (int i = 0; i <= rings; ++i) {
        float t = (float)i / rings;
        float r = r0 * (1.0f - t * 0.95f);
        float y = length * t;
        float bend = curve * t * t;       // curve forward (z)
        for (int j = 0; j <= seg; ++j) {
            float ph = (float)j / seg * 2.0f * (float)M_PI;
            float cx = std::cos(ph), cz = std::sin(ph);
            glm::vec3 p(r * cx, y, r * cz + bend);
            glm::vec3 n = glm::normalize(glm::vec3(cx, 0.3f, cz));
            pushVertex(xf, nrm, p, n, color);
        }
    }
    int stride = seg + 1;
    for (int i = 0; i < rings; ++i)
        for (int j = 0; j < seg; ++j) {
            unsigned int a = base + i * stride + j;
            unsigned int b = base + (i + 1) * stride + j;
            indices.push_back(a); indices.push_back(b); indices.push_back(a + 1);
            indices.push_back(a + 1); indices.push_back(b); indices.push_back(b + 1);
        }
}

void GeometryBuilder::addGroundQuad(glm::vec3 c, float size, glm::vec3 color) {
    unsigned int base = (unsigned int)verts.size();
    float h = size * 0.5f;
    glm::vec3 n(0, 1, 0);
    Vertex a{ {c.x - h, c.y, c.z - h}, n, color };
    Vertex b{ {c.x + h, c.y, c.z - h}, n, color };
    Vertex d{ {c.x + h, c.y, c.z + h}, n, color };
    Vertex e{ {c.x - h, c.y, c.z + h}, n, color };
    verts.push_back(a); verts.push_back(b); verts.push_back(d); verts.push_back(e);
    // CCW winding when viewed from above (+Y) so the up-facing normal is the front
    indices.push_back(base); indices.push_back(base + 2); indices.push_back(base + 1);
    indices.push_back(base); indices.push_back(base + 3); indices.push_back(base + 2);
}
