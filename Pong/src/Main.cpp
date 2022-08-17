#include "Game.h"

int main(int argc, char **argv) {
	Game game("Pong", 900, 600, 144);
	game.start();
	return game.getExitCode();
}