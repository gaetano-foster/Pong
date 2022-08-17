#pragma once
#include <SDL.h>

class Display {
public:
	Display(int width, int height, const char* title);
	~Display();

public:
    bool construct(int index);
    SDL_Renderer* getRenderer();
    const char* getTitle();
    void setTitle(const char* title);

private:
    const char* m_title;
    int m_width, m_height;
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
};

