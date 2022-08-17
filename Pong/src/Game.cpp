#include "Game.h"
#include <iostream>

Game::Game(const char* title, int width, int height, int maxFps) 
	: m_title(title)
    , m_width(width)
    , m_height(height)
	, m_display(width, height, title)
	, m_maxFps(maxFps)
    , m_exitCode(0)
	, m_running(false)
	, m_y1((float)(height/2-P_HEIGHT/2))
    , m_y2((float)(height/2-P_HEIGHT/2))
    , b_x((float)(width)/2)
    , b_y((float)(height)/2)
{
	SDL_Init(SDL_INIT_EVERYTHING);
}

void Game::start() {
	m_running = true;
	run();
}

void Game::stop(int exitCode) {
	m_exitCode = exitCode;
	m_running = false;
}

int Game::getExitCode() {
	return m_exitCode;
}

void Game::init() {
    m_display.construct(-1);
    soloud.init();
    b_hit.load("res/b_hit.wav");
    p_die.load("res/p_die.wav");
    startup.load("res/startup.wav");
}

void Game::run() {
    init();

    double delta = 0;
    float deltatime;
    double time_per_tick;
    clock_t now;
    clock_t last_time;
    clock_t timer = 0;
    float ticks = 0;
    int frame = 0;
    SDL_Event e;

    time_per_tick = CLOCKS_PER_SEC / m_maxFps;
    deltatime = 1.0f / m_maxFps;
    last_time = clock();

    while (m_running) {
        now = clock();
        delta += (now - last_time) / time_per_tick;
        timer += now - last_time;
        last_time = now;

        if (timer >= CLOCKS_PER_SEC) {
            deltatime = 1.0f / ticks;
            ticks = 0;
            timer = 0;
        }

        if (delta >= 1) {
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    stop(0);
                    break;
                }
                Input::getInstance().update(e);
            }
            update(deltatime, frame);
            render(m_display.getRenderer());
            frame++;
            ticks++;
            delta--;
        }
    }
}

void Game::render(SDL_Renderer* renderer) {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect p1 = { P_XPOS, (int)m_y1, P_WIDTH, P_HEIGHT }; 
    SDL_Rect p2 = { m_width - P_XPOS - P_WIDTH, (int)m_y2, P_WIDTH, P_HEIGHT };
    SDL_Rect b = { b_x, b_y, B_RADIUS, B_RADIUS };
    SDL_RenderFillRect(renderer, &p1);
    SDL_RenderFillRect(renderer, &p2);
    SDL_RenderFillRect(renderer, &b);
    SDL_RenderDrawLine(renderer, m_width / 2, 0, m_width / 2, m_height);
	SDL_RenderPresent(renderer);
}

void Game::update(float deltaTime, float frame) {
    static float timer = 0;
    static const float bu_speed = B_U_SPEED;
    auto pause = [&](float seconds) {
        timer = seconds;
    };

    if (timer > 0) {
        timer -= deltaTime;
        deltaTime = 0;
    }

    if (frame == 0) {
        soloud.play(startup);
        pause(1.5f);
    }

    Input& i = Input::getInstance();
    SDL_Rect p1 = { P_XPOS, (int)m_y1, P_WIDTH, P_HEIGHT };
    SDL_Rect p2 = { m_width - P_XPOS - P_WIDTH, (int)m_y2, P_WIDTH, P_HEIGHT };
    static int s1 = 0, s2 = 0;

    if ((b_x + v_x * deltaTime <= p1.x + p1.w) && (b_y + B_RADIUS + v_y * deltaTime > p1.y) && (b_y + v_y * deltaTime < p1.y + p1.h)) {
        float relativeIntersectY = (p1.y+(P_HEIGHT/2)) - b_y;
        if (relativeIntersectY == 0) {
            relativeIntersectY += 0.1f;
        }
        float normRelIntY = (relativeIntersectY / (P_HEIGHT / 2));
        float bounceAngle = normRelIntY * MAX_ANGLE;

        v_x = B_SPEED * cosf(bounceAngle);
        v_y = B_SPEED * -sinf(bounceAngle);
        soloud.play(b_hit);
    }
    else if ((b_x + B_RADIUS + v_x * deltaTime >= p2.x) && (b_y + B_RADIUS + v_y * deltaTime > p2.y) && (b_y + v_y * deltaTime < p2.y + p2.h)) {
        float relativeIntersectY = (p2.y + (P_HEIGHT / 2)) - b_y;
        if (relativeIntersectY == 0) {
            relativeIntersectY += 0.1f;
        }
        float normRelIntY = (relativeIntersectY / (P_HEIGHT / 2));
        float bounceAngle = normRelIntY * MAX_ANGLE;

        v_x = -B_SPEED * cosf(bounceAngle);
        v_y = -B_SPEED * -sinf(bounceAngle);
        soloud.play(b_hit);
    }

    if (b_y < 0 || b_y + 10 > m_height) {
        v_y = -v_y;
        soloud.play(b_hit);
    }

    if (b_x < P_XPOS) {
        s2++;
        b_x = m_width / 2 - B_RADIUS / 2;
        b_y = 0;
        v_x = -bu_speed;
        v_y = bu_speed;
        std::cout << "----Score----" << std::endl;
        std::cout << "Player 1: " << s1 << std::endl;
        std::cout << "Player 2: " << s2 << std::endl;
        std::cout << std::endl;
        soloud.play(p_die);
        pause(1);
    }
    else if (b_x > m_width - P_XPOS - P_WIDTH) {
        s1++;
        b_x = m_width / 2 - B_RADIUS / 2;
        b_y = 0;
        v_x = bu_speed;
        v_y = bu_speed;
        std::cout << "----Score----" << std::endl;
        std::cout << "Player 1: " << s1 << std::endl;
        std::cout << "Player 2: " << s2 << std::endl;
        std::cout << std::endl;
        soloud.play(p_die);
        pause(1);
    }

    if (i.keyHeld(SC(W)))
        m_y1 -= P_SPEED * deltaTime;
    if (i.keyHeld(SC(S)))
        m_y1 += P_SPEED * deltaTime;

    if (i.keyHeld(SC(UP)))
        m_y2 -= P_SPEED * deltaTime;
    if (i.keyHeld(SC(DOWN)))
        m_y2 += P_SPEED * deltaTime;

    if (m_y1 < 0)
        m_y1 = 0;
    else if (m_y1 > m_height - P_HEIGHT)
        m_y1 = (float)(m_height - P_HEIGHT);

    if (m_y2 < 0)
        m_y2 = 0;

    if (m_y2 < 0)
        m_y2 = 0;
    else if (m_y2 > m_height - P_HEIGHT)
        m_y2 = (float)(m_height - P_HEIGHT);

    b_x += v_x * deltaTime;
    b_y += v_y * deltaTime;
}
