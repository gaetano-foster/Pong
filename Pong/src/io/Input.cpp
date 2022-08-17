#include "Input.h"

bool Input::keyHeld(int id) {
    return m_keys[id];
}

void Input::update(SDL_Event e) {
    if (e.type == SDL_KEYDOWN) {
        m_keys[e.key.keysym.scancode] = true;
    }
    else if (e.type == SDL_KEYUP) {
        m_keys[e.key.keysym.scancode] = false;
    }
}

Input& Input::getInstance() {
    static Input me;
    return me;
}

Input::Input() { 
    for (int i = 0; i < 282; i++)
        m_keys[i] = false;
}

Input::~Input()
{ }
