#pragma once

#include <utilities.hpp>


constexpr glm::vec3 defaultPos     = glm::vec3(0.f, -10.f, 2.f);
constexpr glm::vec3 defaultWorldUp = glm::vec3(0.f, 0.f, 1.f);

constexpr float     defaultYaw     = 0.f;

constexpr float     defaultPitch   = 0.f;
constexpr float     maxPitch       = 80.f;
constexpr float     minPitch       = -80.f;

constexpr float     defaultFovY    = 60.f;
constexpr float     maxFovY        = 140.f;
constexpr float     minFovY        = 10.f;

constexpr float     defaultNear    = 1.f;
constexpr float     defaultFar     = 10000.f;
constexpr float     minNear        = .1f;
constexpr float     maxFar         = 10000.f;
constexpr float     minDepth       = 10.f;

constexpr float     defaultSpeed   = 800.f;
constexpr glm::vec2 defaultSens    = glm::vec2(.15f, .1f);



class Camera{
public:
    Camera();    

    void setPosition(glm::vec3 pos);
    void setFovY(float fovY);
    void setNearPlane(float near);
    void setFarPlane(float far);
    void setMovementSpeed(float speed);
    void setMouseSensitivity(float xSens, float ySens);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjMatrix(float aspectRatio) const;

    void move(const glm::vec3& inputDir, float deltaTime);
    void rotate(float xOffset, float yOffset);

private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 worldUp;

    float     yaw;
    float     pitch;

    float     fovY;

    float     near;
    float     far;

    float     movementSpeed;
    glm::vec2 mouseSensitivity;


    void updateCameraVectors();
};
