#include <inputHandler.hpp>

#include <logger.hpp>


static InputHandler* currentInstance = nullptr;

InputHandler::InputHandler(KeyboardLayout layout) { 
    setKeyboardLayout(layout); 
    currentInstance = this;
}

void InputHandler::bindWindow(GLFWwindow* window) { 
    this->window = window; 

    glfwSetInputMode(this->window, GLFW_STICKY_KEYS, GLFW_TRUE);

    if (mouseEnabled) glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    glfwSetKeyCallback(this->window, keyCallback);
}


void InputHandler::setKeyboardLayout(KeyboardLayout layout){
    switch (layout) {
        case FR:
            keyBindings = frKeyBindings; break;
        case EN:
            keyBindings = enKeyBindings; break;
    }
}


InputData::InputData() : movement(0.f), mouseOffset(0.f){}

InputData InputHandler::getInputData(){
    InputData inputData{};

    if (!window) {
        LOG_WARNING("Input handler window not bound"); 
        return inputData;
    }

    // Keyboard
    if (glfwGetKey(window, keyBindings[MOVE_FORWARD])  == GLFW_PRESS) 
        inputData.movement.y += 1.f;
    if (glfwGetKey(window, keyBindings[MOVE_BACKWARD]) == GLFW_PRESS) 
        inputData.movement.y -= 1.f;
    if (glfwGetKey(window, keyBindings[MOVE_RIGHT])    == GLFW_PRESS) 
        inputData.movement.x += 1.f;
    if (glfwGetKey(window, keyBindings[MOVE_LEFT])     == GLFW_PRESS) 
        inputData.movement.x -= 1.f;
    if (glfwGetKey(window, keyBindings[MOVE_UP])       == GLFW_PRESS) 
        inputData.movement.z += 1.f;
    if (glfwGetKey(window, keyBindings[MOVE_DOWN])     == GLFW_PRESS) 
        inputData.movement.z -= 1.f;



    // Mouse
    if (mouseEnabled) {
        double xPos, yPos;
        glfwGetCursorPos(window, &xPos, &yPos);

        float xPosF = static_cast<float>(xPos);
        float yPosF = static_cast<float>(yPos);

        if (!firstMouse) {
            inputData.mouseOffset.x = xPosF - lastMousePos.x;
            inputData.mouseOffset.y = lastMousePos.y - yPosF;
        } 
        else {
            firstMouse = false;
        }

        lastMousePos = {xPosF, yPosF};
    }

    return inputData;
}


void InputHandler::keyCallback(GLFWwindow* window, int key, int scanCode, int action, int mods){
    if (currentInstance) currentInstance->handleKeyPress(key, action);
}

void InputHandler::handleKeyPress(int key, int action){
    if (key == keyBindings[TOGGLE_MOUSE] && action == GLFW_PRESS) {
        mouseEnabled = !mouseEnabled;
        glfwSetInputMode(window, GLFW_CURSOR, mouseEnabled ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

        firstMouse = true;
    }
}
