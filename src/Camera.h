// Camera.h - Third-person orbit camera that follows the player.
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    glm::vec3 target{0,1,0};   // look-at (player chest)
    float yaw = -1.57f;        // radians (mouse X)
    float pitch = -0.25f;      // radians (mouse Y)
    float distance = 6.0f;
    float minPitch = -1.3f, maxPitch = 0.5f;
    float fov = 60.0f;
    float aspect = 16.0f/9.0f;

    glm::vec3 position{0,3,6};

    void addMouse(float dx, float dy, float sens) {
        yaw   += dx * sens;
        pitch -= dy * sens;
        if (pitch < minPitch) pitch = minPitch;
        if (pitch > maxPitch) pitch = maxPitch;
    }

    // Returns forward direction projected on XZ plane (for movement).
    glm::vec3 flatForward() const {
        glm::vec3 f(std::cos(yaw), 0, std::sin(yaw));
        return glm::normalize(f);
    }
    glm::vec3 flatRight() const {
        glm::vec3 f = flatForward();
        return glm::normalize(glm::vec3(-f.z, 0, f.x));
    }

    void update(glm::vec3 followTarget) {
        target = followTarget + glm::vec3(0,1.4f,0);
        glm::vec3 dir;
        dir.x = std::cos(yaw)*std::cos(pitch);
        dir.y = std::sin(pitch);
        dir.z = std::sin(yaw)*std::cos(pitch);
        position = target - dir * distance;
        if (position.y < 0.4f) position.y = 0.4f;
    }

    glm::mat4 view() const { return glm::lookAt(position, target, glm::vec3(0,1,0)); }
    glm::mat4 proj() const { return glm::perspective(glm::radians(fov), aspect, 0.1f, 400.0f); }
};
