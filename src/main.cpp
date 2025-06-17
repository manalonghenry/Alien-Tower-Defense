
#define GL3W_IMPLEMENTATION
#include <gl3w.h>

// stdlib
#include <chrono>
#include <iostream>

// internal
#include "ai_system.hpp"
#include "physics_system.hpp"
#include "render_system.hpp"
#include "world_system.hpp"
#include "tinyECS/registry.hpp"

using Clock = std::chrono::high_resolution_clock;

// Entry point
int main()
{
	// global systems
	AISystem	  ai_system;
	WorldSystem   world_system;
	RenderSystem  renderer_system;
	PhysicsSystem physics_system;

	// initialize window
	GLFWwindow* window = world_system.create_window();
	if (!window) {
		// Time to read the error message
		std::cerr << "ERROR: Failed to create window.  Press any key to exit" << std::endl;
		getchar();
		return EXIT_FAILURE;
	}

	if (!world_system.start_and_load_sounds()) {
		std::cerr << "ERROR: Failed to start or load sounds." << std::endl;
	}

	// initialize the main systems
	renderer_system.init(window);
	world_system.init(&renderer_system);

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// A2: see GAME_SCREEN_ID in components.hpp
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	GAME_SCREEN_ID game_screen = world_system.get_game_screen();

	// variable timestep loop
	auto t = Clock::now();
	while (!world_system.is_over()) {
		
		// processes system messages, if this wasn't present the window would become unresponsive
		glfwPollEvents();

		// calculate elapsed times in milliseconds from the previous iteration
		auto now = Clock::now();
		float elapsed_ms =
			(float)(std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count() / 1000;
		t = now;

		// all screens need to update the window caption
		world_system.update_window_caption();

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// A2: draw different game screens
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		game_screen = world_system.get_game_screen();
		switch (game_screen) {

			case GAME_SCREEN_ID::DRAWING:
				// A2: only draw the level, no updates
				// std::cout << "Drawing" << std::endl;
				break;

			case GAME_SCREEN_ID::PLAYING:
				// std::cout << "Playing" << std::endl;
				// A2: draw all the things and update too
				world_system.step(elapsed_ms);
				ai_system.step(elapsed_ms);
				physics_system.physics_step(elapsed_ms);
				world_system.handle_collisions();
				break;

			case GAME_SCREEN_ID::TILE_SELECTOR:
				// A2: only draw the "selectable" tiles, no updates
				// std::cout << "Tile" << std::endl;
				break;
			case GAME_SCREEN_ID::INTRO:
				// std::cout << "Intro" << std::endl;
				world_system.step(elapsed_ms);
				break;
			case GAME_SCREEN_ID::END:
				world_system.step(elapsed_ms);
		}

		// render the current screen
		renderer_system.draw(game_screen);
	}

	return EXIT_SUCCESS;
}
