#pragma once

#include <utilities.hpp>


class Camera{
public:
    glm::vec3 position = glm::vec3(0.f);
    float     fov = 60.f;

    float     near = 1.f;
    float     far  = 5000.f;

    float     movementSpeed    = 800.f;
    glm::vec2 mouseSensitivity = glm::vec2(15.f, 10.f);


    Camera();    

    glm::mat4        getViewMatrix() const;
    glm::mat4        getProjMatrix(float aspectRatio) const;

    void move(const glm::vec3& inputDir, float deltaTime);
    void rotate(float xOffset, float yOffset);

private:
    glm::vec3 worldUp  = glm::vec3(0.f, 0.f, 1.f);
    glm::vec3 front;
    glm::vec3 right;

    float     yaw   = 0.f;
    float     pitch = 0.f;


    void updateCameraVectors();
};
