#include <camera.hpp>


Camera::Camera() :
    position(defaultPos), worldUp(defaultWorldUp), yaw(defaultYaw), pitch(defaultPitch), 
    fovY(defaultFovY), near(defaultNear), far(defaultFar),
    movementSpeed(defaultSpeed), mouseSensitivity(defaultSens)  
{
    updateCameraVectors();
}


void Camera::setPosition(glm::vec3 pos) { position = pos; }

void Camera::setFovY(float fovY)      { this->fovY = std::clamp(fovY, minFovY, maxFovY); }
void Camera::setNearPlane(float near) { this->near = std::clamp(near, minNear, far - minDepth); }
void Camera::setFarPlane(float far)   { this->far  = std::clamp(far, near + minDepth, maxFar); }

void Camera::setMovementSpeed(float speed)                 { movementSpeed = speed; }
void Camera::setMouseSensitivity(float xSens, float ySens) { mouseSensitivity.x = xSens; mouseSensitivity.y = ySens; }


const glm::vec3& Camera::getPosition() const             { return position; }
glm::mat4 Camera::getViewMatrix() const                  { return glm::lookAt(position, position + front, worldUp); }
glm::mat4 Camera::getProjMatrix(float aspectRatio) const { return glm::perspective(glm::radians(fovY), aspectRatio, near, far); }



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

    yaw   += xOffset * mouseSensitivity.x;
    pitch += yOffset * mouseSensitivity.y;

    pitch = std::clamp(pitch, minPitch, maxPitch);

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
