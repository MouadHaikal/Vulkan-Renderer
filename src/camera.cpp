#include <camera.hpp>


Camera::Camera() { updateCameraVectors(); }



glm::mat4 Camera::getViewMatrix() const                  { return glm::lookAt(position, position + front, worldUp); }
glm::mat4 Camera::getProjMatrix(float aspectRatio) const { return glm::perspective(glm::radians(fov), aspectRatio, near, far); }



void Camera::move(const glm::vec3& inputDir, float deltaTime){
    if (!glm::length(inputDir)) return;

    float distance = movementSpeed * deltaTime;

    auto movement = distance * glm::normalize(inputDir);

    position += movement.x * right;
    position += movement.y * front;
    position += movement.z * worldUp;
}

void Camera::rotate(float xOffset, float yOffset){
    if (!xOffset && !yOffset) return;

    yaw   += xOffset * mouseSensitivity.x / 100;
    pitch += yOffset * mouseSensitivity.y / 100;

    pitch = std::clamp(pitch, -80.f, 80.f);

    updateCameraVectors();
}


void Camera::updateCameraVectors(){
    glm::vec3 updatedFront{
        glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch)),
        glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch)),
        glm::sin(glm::radians(pitch))
    };

    front = glm::normalize(updatedFront);
    right = glm::normalize(glm::cross(front, worldUp));
}
