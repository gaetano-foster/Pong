# Pong

This is a simple single-player implementation of the classic Pong game in C using SDL.

## Prerequisites

Before compiling the game, ensure that you have the SDL library installed on your system. You can download and install it from the [SDL website]([https://www.libsdl.org/download-2.0.php](https://github.com/libsdl-org/SDL/releases/tag/release-2.30.1)https://github.com/libsdl-org/SDL/releases/tag/release-2.30.1).

It should work on any system with any compiler; the code is pretty cross platform.

## Controls
* Up arrow   - Move up
* Down arrow - Move down

## Gameplay
* Select a mode on the title screen.
* The ball will be randomly served to either you or the AI.
* Block it by moving your paddle up and down.
* Try to hit it past the AI's paddle to score a point.
* First to 12 points wins.

## Modes
* Easy        - AI paddle moves at half speed, ball speed does not increase, and AI is innaccurate.
* Normal      - AI paddle moves at same speed, ball speed does not increase, and AI is slightly innacurate
* Hard        - AI paddle moves at same speed, ball speed increases after each point, and AI is accurate
* Impossible  - AI paddle moves at double speed, ball speed increases after each point, and AI is perfect
