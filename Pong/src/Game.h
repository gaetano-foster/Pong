#pragma once
#include "io/display/Display.h"
#include "io/Input.h"
#include <soloud.h>
#include <soloud_wav.h>
#include <ctime>
#include <cmath>

#define P_XPOS      20
#define P_HEIGHT    50
#define P_WIDTH     10
#define P_SPEED     600
#define B_SPEED     575
#define B_U_SPEED	sqrtf((B_SPEED*B_SPEED)/2.0f)
#define B_RADIUS    10
#define MAX_ANGLE	0.75f

class Game {
public:
	Game(const char* title, int width, int height, int maxFps);

public:
	void start();
	void stop(int exitCode);
	int getExitCode();

private:
	void init();
	void run();
	void render(SDL_Renderer* renderer);
	void update(float deltaTime, float frame);

private:
	const char* m_title;
	const int m_width, m_height;
	Display m_display;
	int m_maxFps;
	int m_exitCode;
	bool m_running;
	float m_y1;
	float m_y2;
	float b_x, b_y, v_x = -B_SPEED, v_y = 0;

private:
	SoLoud::Soloud soloud;
	SoLoud::Wav b_hit;
	SoLoud::Wav p_die;
	SoLoud::Wav startup;
};

