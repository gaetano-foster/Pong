#pragma once
#include <SDL.h>

#define SC(x)           SDL_SCANCODE_ ##x

class Input {
public:
    Input(Input const&) = delete;
    void operator=(Input const&) = delete;

public:
    bool keyHeld(int id);
    void update(SDL_Event e);

public:
    static Input& getInstance();

private:
    Input();
    ~Input();

private:
    bool m_keys[282];
};

