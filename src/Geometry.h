// Geometry.h - Procedural geometry builder.
// Generates organic, smooth shapes (capsules, tapered cones, ellipsoids, prisms)
// and accumulates them with transforms into a single vertex/index buffer.
// This is how we model the mage, bosses and enemies WITHOUT plain spheres/cubes:
// every body part is a deformed/tapered solid blended into a coherent silhouette.
#pragma once
#include "Mesh.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class GeometryBuilder {
public:
    std::vector<Vertex> verts;
    std::vector<unsigned int> indices;

    void clear() { verts.clear(); indices.clear(); }

    // Append a transformed solid. Normals are transformed by the normal matrix.
    void addCapsule(const glm::mat4& xf, float radius, float height, glm::vec3 color, int seg = 12, int rings = 6);
    // Tapered cylinder: r0 at bottom, r1 at top. Great for limbs, robe, fingers, horns.
    void addTaperedCylinder(const glm::mat4& xf, float r0, float r1, float height, glm::vec3 color, int seg = 12);
    // Ellipsoid (squashed/stretched) - heads, torsos, joints.
    void addEllipsoid(const glm::mat4& xf, glm::vec3 radii, glm::vec3 color, int seg = 14, int rings = 10);
    // Faceted gem / crystal (for staff orb, mana crystals).
    void addCrystal(const glm::mat4& xf, float radius, float height, glm::vec3 color, int seg = 6);
    // Curved horn/claw (tapered + bent).
    void addHorn(const glm::mat4& xf, float r0, float length, float curve, glm::vec3 color, int seg = 8, int rings = 8);
    // Low ground plane patch with subtle color (terrain tiles).
    void addGroundQuad(glm::vec3 center, float size, glm::vec3 color);

    Mesh build() const {
        Mesh m; m.upload(verts, indices); return m;
    }

private:
    void pushVertex(const glm::mat4& xf, const glm::mat3& nrm, glm::vec3 p, glm::vec3 n, glm::vec3 c);
};
