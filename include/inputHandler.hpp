#pragma once

#include <utilities.hpp>


enum KeyboardLayout{
    FR,
    EN
};

enum Action{
    MOVE_FORWARD,
    MOVE_BACKWARD,
    MOVE_RIGHT,
    MOVE_LEFT,
    MOVE_UP,
    MOVE_DOWN,

    TOGGLE_MOUSE
};


const std::unordered_map<Action, int> frKeyBindings = {
    {MOVE_FORWARD , GLFW_KEY_Z},
    {MOVE_BACKWARD, GLFW_KEY_S},
    {MOVE_RIGHT   , GLFW_KEY_D},
    {MOVE_LEFT    , GLFW_KEY_Q},
    {MOVE_UP      , GLFW_KEY_SPACE},
    {MOVE_DOWN    , GLFW_KEY_E},
    {TOGGLE_MOUSE , GLFW_KEY_ESCAPE}
};

const std::unordered_map<Action, int> enKeyBindings = {
    {MOVE_FORWARD , GLFW_KEY_W},
    {MOVE_BACKWARD, GLFW_KEY_S},
    {MOVE_RIGHT   , GLFW_KEY_D},
    {MOVE_LEFT    , GLFW_KEY_A},
    {MOVE_UP      , GLFW_KEY_SPACE},
    {MOVE_DOWN    , GLFW_KEY_E},
    {TOGGLE_MOUSE , GLFW_KEY_ESCAPE}
};


struct InputData{
    glm::vec3 movement;
    glm::vec2 mouseOffset;

    InputData();
};


constexpr KeyboardLayout defaultLayout = EN;


class InputHandler{
public:
    InputHandler(KeyboardLayout layout = defaultLayout);

    void bindWindow(GLFWwindow* window);

    InputData getInputData();

    void setKeyboardLayout(KeyboardLayout layout);
private:
    
    GLFWwindow* window = nullptr;
    std::unordered_map<Action, int> keyBindings;

    bool  mouseEnabled = true;
    bool  firstMouse   = true;

    glm::vec2 lastMousePos;


    static void keyCallback(GLFWwindow* window, int key, int scanCode, int action, int mods);
    void handleKeyPress(int key, int action);
};
