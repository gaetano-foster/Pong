#include "Display.h"
#include <iostream>

Display::Display(int width, int height, const char* title) 
	: m_width(width)
	, m_height(height)
	, m_title(title)
	, m_window(nullptr)
	, m_renderer(nullptr)
	{ }

Display::~Display() {
	SDL_DestroyWindow(m_window);
	SDL_DestroyRenderer(m_renderer);
}

bool Display::construct(int index) {
	m_window = SDL_CreateWindow(m_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_width, m_height, 0);
	if (m_window == nullptr) {
		std::cout << "Error constructing display: " << SDL_GetError() << std::endl;
		return false;
	}
	m_renderer = SDL_CreateRenderer(m_window, index, 0);
	if (m_renderer == nullptr) {
		std::cout << "Error constructing display: " << SDL_GetError() << std::endl;
		return false;
	}
	return true;
}

SDL_Renderer* Display::getRenderer() {
	return m_renderer;
}

const char* Display::getTitle() {
	return m_title;
}

void Display::setTitle(const char* title) {
	m_title = title;
	SDL_SetWindowTitle(m_window, title);
}
