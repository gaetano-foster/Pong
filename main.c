#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <SDL.h>
#include <SDL_mixer.h>

#define SCR_WIDTH		(1280)
#define SCR_HEIGHT		(720)
#define PADDLE_HEIGHT	(SCR_HEIGHT / 8) // paddle covers 1/5 of the screen
#define PADDLE_WIDTH	(SCR_WIDTH / 160)
#define PADDLE_SPEED	(15)
#define MAX_ANGLE		(0.75f)			// about 40 degrees; the max angle that the ball can change by after getting hit
#define TRUE			(1)
#define FALSE			(0)
#define MAX_SCORE		(12)

typedef uint32_t bool_t; // using uint32_t to keep natural word size

struct display_t
{
	SDL_Window *w;
	SDL_Renderer *r;
} display;

struct gameloop_t
{
	bool_t running;
	clock_t last_time;			// time since last iteration of loop
	clock_t time;				// total time since the loop started
	clock_t timer;
	int32_t frames_per_second;
	double ns;					// time per tick
	double delta; 
	double delta_time;
	int32_t frames;				// frames elapsed since we last displayed fps
} gameloop;

struct ball_t
{
	struct p				// position
	{ 
		float x;
		float y;
	} p;

	struct v				// velocity
	{
		float x;
		float y;
	} v;

	int32_t w;				// width
	int32_t h;				// height
	float speed;
	clock_t pause_moment;	// moment of time when the game is paused
} ball;

struct user_t
{
	bool_t  up;		// is the up button held
	bool_t  down;	// is the down button held
	int32_t goal;	// goal position along x axis
	int32_t score;
	float y;		// player position along y axis
} user;

struct ai_t
{
	int32_t goal;		// ai goal position along x axis
	int32_t score;
	float target;		// where the ai wants to go
	float direction;	// where the ai is currently going

	struct last_vel		// ball velocity of last frame
	{
		float x;
		float y;
	} last_vel;		

	float y;			// ai position along y axis 
} ai;

struct sfx_t
{
	Mix_Music *title_screen; // music for title screen
	Mix_Chunk *bounce;		 // when ball bounces
	Mix_Chunk *gameover;	 // game over
	Mix_Chunk *lose;		 // when player loses
	Mix_Chunk *win;			 // when player wins
	Mix_Chunk *userpoint;	 // when player scores
	Mix_Chunk *aipoint;		 // when ai scores
} sfx;

struct screens_t
{
	SDL_Texture *title;
	SDL_Texture *lose;
	SDL_Texture *win;
	SDL_Texture *winningest;
} screens;

/* Globals */
bool_t game_start = FALSE; // lazy state machine. False = title screen, true = game


enum
{
	TITLE,
	LOSE,
	WIN,
	WINNINGEST
} screen_state;

enum 
{
	MODE_EASY,
	MODE_NORMAL,
	MODE_HARD,
	MODE_IMPOSSIBLE
} difficulty;

int32_t RandomRange(int32_t min, int32_t max);
void UpdateTitle();
bool_t LoadSoundEffects();
void FreeSoundEffects();
void InitGameLoop();
void InitBall();
void InitUser();
void InitAI();
bool_t CheckWin();
void BallCheckWallCollisions();
void BallCheckPaddleCollisions();
void BallCheckGoal();
void TickBall();
void TickUser();
void CalculateTargetAI(int32_t random_offset);
void CalculateDirectionAI();
void MoveAI(int32_t speed);
void CalculateOffsetsAI(int32_t* target_offset, int32_t* paddle_speed);
void TickAI();
void DrawBall();
void DrawUser();
void DrawAI();
bool_t LoadScreens();
void FreeScreens();
bool_t ConstructDisplay(struct display_t* display);
void InitGame();
bool_t Initialize();
bool_t Tick();
bool_t Render();
bool_t Run();
void PlayAndWait(int channel, Mix_Chunk *c, int loops);
void HandleTitleMusic();

int32_t
RandomRange(int32_t min, int32_t max)
{
	return min + rand() % (max + 1 - min);
}

void
UpdateTitle()
{
	char user_score_tenth; // 10's place
	char user_score_first; // 1's place
	char ai_score_tenth;
	char ai_score_first;

	if (user.score < 10) { // score capped at 15; no need to check for anything above 1 in the 10's place
		user_score_tenth = '0';
		user_score_first = user.score + '0';
	}
	else {
		user_score_tenth = '1';
		user_score_first = user.score - 10 + '0';
	}

	if (ai.score < 10) {
		ai_score_tenth = '0';
		ai_score_first = ai.score + '0';
	}
	else {
		ai_score_tenth = '1';
		ai_score_first = ai.score - 10 + '0';
	}

	char title[22] = { 'P', 'o', 'n', 'g', ' ', '|', ' ', 'S', 'c', 'o', 'r', 'e', ':', ' ', user_score_tenth, user_score_first, ' ', '-', ' ', ai_score_tenth, ai_score_first, '\0' };
	SDL_SetWindowTitle(display.w, title);
}

bool_t
LoadSoundEffects()
{
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		fprintf(stderr, "Error: Could not initialize SDL_mixer.\nMixer: %s\n", Mix_GetError());
		return FALSE;
	}
	sfx.title_screen = Mix_LoadMUS("sound/title_screen.wav");
	if (!sfx.title_screen) {
		fprintf(stderr, "Error: Could not load title screen music.\nMixer: %s\n", Mix_GetError());
		return FALSE;
	}
	sfx.bounce = Mix_LoadWAV("sound/bounce.wav");
	if (!sfx.bounce) {
		fprintf(stderr, "Error: Could not load bounce sound.\nMixer: %s\n", Mix_GetError());
		return FALSE;
	}
	sfx.lose = Mix_LoadWAV("sound/lose.wav");
	if (!sfx.lose) {
		fprintf(stderr, "Error: Could not load lose sound.\nMixer: %s\n", Mix_GetError());
		return FALSE;
	}
	sfx.win = Mix_LoadWAV("sound/win.wav");
	if (!sfx.win) {
		fprintf(stderr, "Error: Could not load win sound..\nMixer: %s\n", Mix_GetError());
		return FALSE;
	}
	sfx.gameover = Mix_LoadWAV("sound/gameover.wav");
	if (!sfx.gameover) {
		fprintf(stderr, "Error: Could not load gameover sound.\nMixer: %s\n", Mix_GetError());
		return FALSE;
	}
	sfx.userpoint = Mix_LoadWAV("sound/userpoint.wav");
	if (!sfx.userpoint) {
		fprintf(stderr, "Error: Could not load user point sound.\nMixer: %s\n", Mix_GetError());
		return FALSE;
	}
	sfx.aipoint = Mix_LoadWAV("sound/aipoint.wav");
	if (!sfx.aipoint) {
		fprintf(stderr, "Error: Could not load ai point sound.\nMixer: %s\n", Mix_GetError());
		return FALSE;
	}
	return TRUE;
}

void FreeSoundEffects()
{
	Mix_FreeMusic(sfx.title_screen); 
	Mix_FreeChunk(sfx.aipoint);
	Mix_FreeChunk(sfx.userpoint);
	Mix_FreeChunk(sfx.gameover);
	Mix_FreeChunk(sfx.win);
	Mix_FreeChunk(sfx.lose);
	Mix_FreeChunk(sfx.bounce);
}

void 
InitGameLoop()
{
	gameloop.running = TRUE;
	gameloop.last_time = clock();
	gameloop.time = 0;
	gameloop.timer = 0;
	gameloop.frames_per_second = 32;
	gameloop.ns	= CLOCKS_PER_SEC / gameloop.frames_per_second;
	gameloop.delta = 0; 
	gameloop.delta_time = 1 / gameloop.frames_per_second;
	gameloop.frames = 0;
}

void
InitBall()
{
	ball.w = 10;
	ball.h = 10;
	ball.p.x = SCR_WIDTH / 2;
	ball.p.y = SCR_HEIGHT / 2;
	ball.v.x = RandomRange(0, 1) ? 1 : -1; // Selects either -1 or +1 randomly
	ball.v.y = RandomRange(0, 1) ? 1 : -1;
	ball.speed = 20;
	ball.pause_moment = gameloop.time;
}

void
InitUser()
{
	user.up = FALSE;
	user.down = FALSE;
	user.goal = SCR_WIDTH / 20;
	user.score = 0;
	user.y = (SCR_HEIGHT / 2) - PADDLE_HEIGHT / 2;
}

void
InitAI()
{
	ai.goal = SCR_WIDTH - (SCR_WIDTH / 20);
	ai.score = 0;
	ai.target = SCR_HEIGHT / 2;
	ai.direction = 0;
	ai.last_vel.x = ball.v.x;
	ai.last_vel.y = ball.v.y;
	ai.y = (SCR_HEIGHT / 2) - PADDLE_HEIGHT / 2;	
}

void 
PlayAndWait(int channel, 
			Mix_Chunk* chunk, 
			int loops)
{
	channel = Mix_PlayChannel(channel, chunk, loops);
	if (channel < 0) {
		return; // error
	}
	while (Mix_Playing(channel) != 0) {
		SDL_Delay(200);
	}
}

bool_t
CheckWin() // check if someone won
{
	if (user.score == MAX_SCORE) {
		if (difficulty == MODE_IMPOSSIBLE)
			screen_state = WINNINGEST;
		else
			screen_state = WIN;
		game_start = FALSE; // reset game upon winning / losing
		InitGame();
		return TRUE;
	}

	if (ai.score == MAX_SCORE) {
		screen_state = LOSE;
		game_start = FALSE;
		InitGame();
		return TRUE;
	}
	return FALSE;
}

void
BallCheckWallCollisions()
{
	if (ball.p.y <= 0) {
		ball.p.y = 0;
		ball.v.y = - ball.v.y;
		Mix_PlayChannel(-1, sfx.bounce, 0);
	}
	if (ball.p.y + ball.h >= SCR_HEIGHT) {
		ball.p.y = SCR_HEIGHT - ball.h;
		ball.v.y = -ball.v.y;
		Mix_PlayChannel(-1, sfx.bounce, 0);
	}
}

void
BallCheckPaddleCollisions()
{
	// collision with user
	if (ball.p.x + ball.v.x * ball.speed * gameloop.delta_time <= user.goal + PADDLE_WIDTH
		&&  ball.p.y + ball.v.y * ball.speed * gameloop.delta_time + ball.h >= user.y
		&&  ball.p.y + ball.v.y * ball.speed * gameloop.delta_time <= user.y + PADDLE_HEIGHT)
	{
		// calculate the angle that the ball changes by after getting hit
		float y_intersect = (user.y + (PADDLE_HEIGHT / 2)) - ball.p.y;
		if (y_intersect == 0) {
			y_intersect += 0.1f;
		}
		float norm_y_intersect = (y_intersect / (PADDLE_HEIGHT / 2));
		float bounce_angle = norm_y_intersect * MAX_ANGLE;

		ball.v.x = cosf(bounce_angle);
		ball.v.y = -sinf(bounce_angle);
		Mix_PlayChannel(-1, sfx.bounce, 0);
	}

	// collision with ai
	if  (ball.p.x + ball.v.x * ball.speed * gameloop.delta_time >= ai.goal - PADDLE_WIDTH
		&&   ball.p.y + ball.v.y * ball.speed * gameloop.delta_time + ball.h >= ai.y
		&&   ball.p.y + ball.v.y * ball.speed * gameloop.delta_time <= ai.y + PADDLE_HEIGHT)
	{
		float y_intersect = (ai.y + (PADDLE_HEIGHT / 2)) - ball.p.y;
		if (y_intersect == 0) {
			y_intersect += 0.1f;
		}
		float norm_y_intersect = (y_intersect / (PADDLE_HEIGHT / 2));
		float bounce_angle = norm_y_intersect * MAX_ANGLE;

		ball.v.x = -cosf(bounce_angle);
		ball.v.y = sinf(bounce_angle);
		Mix_PlayChannel(-1, sfx.bounce, 0);
	}
}

void 
BallCheckGoal()
{
	if (ball.p.x > ai.goal) {
		user.score++;
		InitBall();
		ball.pause_moment = gameloop.time;
		UpdateTitle();
		if (difficulty >= MODE_HARD) {
			ball.speed += 0.5f;
		}
		if (!CheckWin()) Mix_PlayChannel(-1, sfx.userpoint, 0); // don't play the sound if the win screen is on
	}
	if (ball.p.x < user.goal) {
		ai.score++;
		InitBall();
		ball.pause_moment = gameloop.time;
		UpdateTitle();
		if (difficulty >= MODE_HARD) {
			ball.speed += 0.5f;
		}
		if (!CheckWin()) Mix_PlayChannel(-1, sfx.aipoint, 0);
	}
}

void
TickBall()
{
	if (gameloop.time < ball.pause_moment + 2 * CLOCKS_PER_SEC) // wait 2 seconds before starting
		return;

	ball.p.x += ball.v.x * ball.speed * gameloop.delta_time;
	ball.p.y += ball.v.y * ball.speed * gameloop.delta_time;

	BallCheckWallCollisions();
	BallCheckPaddleCollisions();
	BallCheckGoal();
}

void
TickUser()
{
	if (user.down) {
		user.y += PADDLE_SPEED * gameloop.delta_time;
	}
	if (user.up) {
		user.y -= PADDLE_SPEED * gameloop.delta_time;
	}

	if (user.y < 0) user.y = 0;
	if (user.y + PADDLE_HEIGHT > SCR_HEIGHT) user.y = SCR_HEIGHT - PADDLE_HEIGHT;
}

void
CalculateTargetAI(int32_t random_offset)
{
	if (ball.v.x > 0) { // if the ball is coming toward it
		if (ai.last_vel.x != ball.v.x	// if the ball x velocity changed
		|| ai.last_vel.y != ball.v.y)
		{ 
			ai.target = ball.p.y + ball.v.y * (ai.goal - PADDLE_WIDTH - ball.p.x) - PADDLE_HEIGHT / 2; // calculates where the ball will hit the paddle on the y axis
			ai.target += RandomRange(-random_offset, random_offset);
			ai.last_vel.x = ball.v.x;
			ai.last_vel.y = ball.v.y;
		}
		if (difficulty >= MODE_HARD
		&& (ai.goal - PADDLE_WIDTH - ball.p.x < 30 * ball.w && ai.goal - PADDLE_WIDTH - ball.p.x > 20 * ball.w)) // ai recalculates position when ball is in a certain range )
		{
			ai.target = ball.p.y + ball.v.y * (ai.goal - PADDLE_WIDTH - ball.p.x) - PADDLE_HEIGHT / 2; // calculates where the ball will hit the paddle on the y axis
			ai.target += RandomRange(-random_offset, random_offset);
			ai.last_vel.x = ball.v.x;
			ai.last_vel.y = ball.v.y;
		}
	}
	if (ball.v.x < 0 
	||  gameloop.time < ball.pause_moment + 2 * CLOCKS_PER_SEC) // if ball is paused
	{ 
		ai.target = SCR_HEIGHT / 2 - PADDLE_HEIGHT / 2;
	}
}

void
CalculateDirectionAI() // orient the ai toward target
{
	if (ai.target < ai.y) { // if target is above
		ai.direction = -1;	// move up (y decreases)
	}
	else if (ai.target > ai.y) { // if target is below
		ai.direction = 1;		 // move down (y increases)
	}
	else {
		ai.direction = 0; // don't move
	}
}

void
MoveAI(int32_t speed) // move the ai in its direction
{
	if (ai.direction == -1 && ai.y + ai.direction * PADDLE_SPEED * gameloop.delta_time < ai.target) // if the paddle would go past the target, just snap to the target
		ai.y = ai.target;
	if (ai.direction == 1 && ai.y + ai.direction * PADDLE_SPEED * gameloop.delta_time > ai.target)
		ai.y = ai.target;
	else
		ai.y += ai.direction * speed * gameloop.delta_time;
}

void
CalculateOffsetsAI(int32_t *target_offset, // changes the random offset and paddle speed based on difficulty
				   int32_t *paddle_speed)
{
	switch (difficulty)
	{
	case MODE_EASY:
		*target_offset = PADDLE_HEIGHT;
		*paddle_speed = PADDLE_SPEED / 2;
		break;
	case MODE_NORMAL:
		*target_offset = PADDLE_HEIGHT / 2;
		*paddle_speed = PADDLE_SPEED;
		break;
	case MODE_HARD:
		*target_offset = PADDLE_HEIGHT / 4;
		*paddle_speed = PADDLE_SPEED;
		break;
	case MODE_IMPOSSIBLE:
		*target_offset = 0;
		*paddle_speed = PADDLE_SPEED * 2;
		break;
	}
}

void
TickAI()
{
	int32_t target_offset, paddle_speed;
	CalculateOffsetsAI(&target_offset, &paddle_speed);
	CalculateTargetAI(target_offset);
	CalculateDirectionAI();
	MoveAI(paddle_speed);

	if (ai.y < 0) ai.y = 0;
	if (ai.y + PADDLE_HEIGHT > SCR_HEIGHT) ai.y = SCR_HEIGHT - PADDLE_HEIGHT;
}

void
DrawBall()
{
	SDL_Rect r = (SDL_Rect) { ball.p.x, ball.p.y, ball.w, ball.h };

	SDL_SetRenderDrawColor(display.r, 255, 255, 255, 255);
	SDL_RenderFillRect(display.r, &r);
}

void
DrawUser()
{
	SDL_Rect r = (SDL_Rect) { user.goal, user.y, PADDLE_WIDTH, PADDLE_HEIGHT };

	SDL_SetRenderDrawColor(display.r, 255, 255, 255, 255);
	SDL_RenderFillRect(display.r, &r);
}

void
DrawAI()
{
	SDL_Rect r = (SDL_Rect) { ai.goal - PADDLE_WIDTH, ai.y, PADDLE_WIDTH, PADDLE_HEIGHT }; // ai.goal - PADDLE_WIDTH -- because SDL draws from left to right
	SDL_Rect t = (SDL_Rect) { ai.goal - PADDLE_WIDTH, ai.target, 10, 10 };

	SDL_SetRenderDrawColor(display.r, 255, 255, 255, 255);
	SDL_RenderDrawRect(display.r, &r);
}

bool_t
LoadScreens()
{
	SDL_Surface *title_surface = SDL_LoadBMP("title.bmp");
	SDL_Surface *lose_surface = SDL_LoadBMP("lose.bmp");
	SDL_Surface *win_surface = SDL_LoadBMP("win.bmp");
	SDL_Surface *winningest_surface = SDL_LoadBMP("realwin.bmp");
	if (!title_surface || !lose_surface || !win_surface || !winningest_surface) {
		return FALSE;
	}
	screens.title = SDL_CreateTextureFromSurface(display.r, title_surface);
	screens.lose = SDL_CreateTextureFromSurface(display.r, lose_surface);
	screens.win = SDL_CreateTextureFromSurface(display.r, win_surface);
	screens.winningest = SDL_CreateTextureFromSurface(display.r, winningest_surface);

	SDL_FreeSurface(title_surface);
	SDL_FreeSurface(lose_surface);
	SDL_FreeSurface(win_surface);
	SDL_FreeSurface(winningest_surface);

	return TRUE;
}

void
FreeScreens()
{
	SDL_DestroyTexture(screens.title);
	SDL_DestroyTexture(screens.lose);
	SDL_DestroyTexture(screens.win);
	SDL_DestroyTexture(screens.winningest);
}

bool_t
ConstructDisplay(struct display_t *display)
{
	SDL_Window *w = SDL_CreateWindow("Pong | Score: 00 - 00", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCR_WIDTH, SCR_HEIGHT, 0);
	SDL_Renderer *r = SDL_CreateRenderer(w, -1, 0);

	if (!w) {
		fprintf(stderr, "Error: Could not create window.\nSDL: %s\n", SDL_GetError());
		return FALSE;
	}

	if (!r) {
		fprintf(stderr, "Error: Could not create renderer.\nSDL: %s\n", SDL_GetError());
		return FALSE;
	}

	display->w = w;
	display->r = r;

	return TRUE;
}

void
InitGame()
{
	InitBall();
	InitUser();
	InitAI();
	UpdateTitle();
}

bool_t
Initialize()
{
	SDL_Init(SDL_INIT_EVERYTHING);
	Mix_Init(0);
	if (!LoadSoundEffects()) {
		fprintf(stderr, "Error: Could not load sound effects.\n");
		return FALSE;
	}
	srand(time(0));
	if (!ConstructDisplay(&display)) {
		fprintf(stderr, "Could not initialize display window.\n");
		return FALSE;
	}
	if (!LoadScreens()) {
		fprintf(stderr, "Could not load the title screen.\n");
		return FALSE;
	}
	InitGameLoop();
	InitGame();

	return TRUE;
}

void
HandleTitleMusic()
{
	if (!Mix_PlayingMusic()) {
		switch (screen_state)
		{
		case TITLE: Mix_PlayMusic(sfx.title_screen, 1); break;
		case LOSE: PlayAndWait(-1, sfx.gameover, 0); PlayAndWait(-1, sfx.lose, 0); screen_state = TITLE; break;
		case WIN: PlayAndWait(-1, sfx.gameover, 0); PlayAndWait(-1, sfx.win, 0); screen_state = TITLE; break;
		case WINNINGEST: PlayAndWait(-1, sfx.gameover, 0); PlayAndWait(-1, sfx.win, 0); screen_state = TITLE; break;
		}
	}
}

bool_t
Tick()
{
	if (!game_start) {
		HandleTitleMusic();
		return TRUE;
	}
	if (Mix_PlayingMusic()) {
		Mix_HaltMusic(); // stop music when title screen gone
	}

	TickBall();
	TickUser();
	TickAI();

	return TRUE;
}

bool_t
Render()
{
	if (!game_start) {
		SDL_Rect location = (SDL_Rect) { 0, 0, SCR_WIDTH, SCR_HEIGHT };
		SDL_RenderClear(display.r);

		switch (screen_state) // which screen should be rendered
		{
		case TITLE:
			SDL_RenderCopy(display.r, screens.title, NULL, &location);
			break;
		case LOSE:
			SDL_RenderCopy(display.r, screens.lose, NULL, &location);
			break;
		case WIN:
			SDL_RenderCopy(display.r, screens.win, NULL, &location);
			break;
		case WINNINGEST:
			SDL_RenderCopy(display.r, screens.winningest, NULL, &location);
			break;
		}
		
		SDL_RenderPresent(display.r);
		return TRUE;
	}

	SDL_SetRenderDrawColor(display.r, 0, 0, 0, 255); // clear screen to black
	SDL_RenderClear(display.r);
	DrawBall();
	DrawUser();
	DrawAI();
	SDL_RenderPresent(display.r);

	return TRUE;
}

bool_t
Run()
{
	SDL_Event e;

	if (!Initialize()) { // something went wrong
		fprintf(stderr, "Could not initialize game.\n");
		SDL_DestroyRenderer(display.r);
		SDL_DestroyWindow(display.w);
		return FALSE;
	}

	while (gameloop.running) {
		clock_t now = clock();
		gameloop.delta += (now - gameloop.last_time) / gameloop.ns;
		gameloop.timer += now - gameloop.last_time;
		gameloop.last_time = now;

		if (gameloop.delta >= 1) {
			while (SDL_PollEvent(&e)) {
				gameloop.running = e.type != SDL_QUIT; // stop running if sdl event says quit

				if (e.type == SDL_KEYDOWN) {
					if (!game_start) {
						difficulty = MODE_EASY; // default to easy mode
						if (e.key.keysym.scancode == SDL_SCANCODE_1) difficulty = MODE_EASY;
						if (e.key.keysym.scancode == SDL_SCANCODE_2) difficulty = MODE_NORMAL;
						if (e.key.keysym.scancode == SDL_SCANCODE_3) difficulty = MODE_HARD;
						if (e.key.keysym.scancode == SDL_SCANCODE_4) difficulty = MODE_IMPOSSIBLE;
						game_start = TRUE;
						InitGame();
					}
					else {
						if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)  { // End game if ESC pressed
							screen_state = TITLE;
							game_start = FALSE; 
						}
						if (e.key.keysym.scancode == SDL_SCANCODE_UP) user.up = TRUE;
						if (e.key.keysym.scancode == SDL_SCANCODE_DOWN) user.down = TRUE;
					}
				}
				else if (e.type == SDL_KEYUP) {
					if (e.key.keysym.scancode == SDL_SCANCODE_UP) user.up = FALSE;
					if (e.key.keysym.scancode == SDL_SCANCODE_DOWN) user.down = FALSE;
				}
			}

			if (!Tick()) gameloop.running = FALSE;
			if (!Render()) gameloop.running = FALSE;
			gameloop.frames++;
			gameloop.delta--;
			if (gameloop.timer >= CLOCKS_PER_SEC && game_start) {
				gameloop.time += CLOCKS_PER_SEC;
				gameloop.delta_time = (double)(1 / (double)gameloop.frames) * 25;
				gameloop.timer = 0;
				gameloop.frames = 0;
			}
		}
	}

	SDL_DestroyRenderer(display.r);
	SDL_DestroyWindow(display.w);
	FreeScreens();
	FreeSoundEffects();
	return TRUE;
}

int
main(int argc,
	char **argv)
{
	Run();

	return 0;
}