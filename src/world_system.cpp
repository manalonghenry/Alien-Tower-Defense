// Header
#include "world_system.hpp"
#include "world_init.hpp"

// stlib
#include <cassert>
#include <sstream>		// string-stream for splitting lines
#include <iostream>
#include <map>
#include <vector>

// for A2 level loading
#include <fstream>

#include "physics_system.hpp"
#include "tiles.hpp"
#include "pathing.hpp"

// ADDED
#include "tinyECS/components.hpp";
#include "tinyECS/registry.hpp";
#include <queue>
#include <unordered_set>
#include <string>
#include <unordered_map>


#include <functional>


// create the world
WorldSystem::WorldSystem() :
	points(0),
	max_towers(MAX_TOWERS_START),
	next_invader_spawn(0),
	invader_spawn_rate_ms(INVADER_SPAWN_RATE_MS),
	game_screen(GAME_SCREEN_ID::INTRO)
{
	// seeding rng with random device
	rng = std::default_random_engine(std::random_device()());
}

WorldSystem::~WorldSystem() {
	// Destroy music components
	if (background_music != nullptr)
		Mix_FreeMusic(background_music);
	if (chicken_dead_sound != nullptr)
		Mix_FreeChunk(chicken_dead_sound);
	if (chicken_eat_sound != nullptr)
	if (chicken_eat_sound != nullptr)
		Mix_FreeChunk(chicken_eat_sound);
	Mix_CloseAudio();

	// Destroy all created components
	registry.clear_all_components();

	// Close the window
	glfwDestroyWindow(window);
}

// Debugging
namespace {
	void glfw_err_cb(int error, const char *desc) {
		std::cerr << error << ": " << desc << std::endl;
	}
}

// call to close the window, wrapper around GLFW commands
void WorldSystem::close_window() {
	glfwSetWindowShouldClose(window, GLFW_TRUE);
}

// World initialization
// Note, this has a lot of OpenGL specific things, could be moved to the renderer
GLFWwindow* WorldSystem::create_window() {

	///////////////////////////////////////
	// Initialize GLFW
	glfwSetErrorCallback(glfw_err_cb);
	if (!glfwInit()) {
		std::cerr << "ERROR: Failed to initialize GLFW in world_system.cpp" << std::endl;
		return nullptr;
	}

	//-------------------------------------------------------------------------
	// If you are on Linux or Windows, you can change these 2 numbers to 4 and 3 and
	// enable the glDebugMessageCallback to have OpenGL catch your mistakes for you.
	// GLFW / OGL Initialization
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	// CK: setting GLFW_SCALE_TO_MONITOR to true will rescale window but then you must handle different scalings
	// glfwWindowHint(GLFW_SCALE_TO_MONITOR, GL_TRUE);		// GLFW 3.3+
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GL_FALSE);		// GLFW 3.3+

	// Create the main window (for rendering, keyboard, and mouse input)
	window = glfwCreateWindow(WINDOW_WIDTH_PX, WINDOW_HEIGHT_PX, "Towers vs Invaders Assignment", nullptr, nullptr);
	if (window == nullptr) {
		std::cerr << "ERROR: Failed to glfwCreateWindow in world_system.cpp" << std::endl;
		return nullptr;
	}

	// Setting callbacks to member functions (that's why the redirect is needed)
	// Input is handled using GLFW, for more info see
	// http://www.glfw.org/docs/latest/input_guide.html
	glfwSetWindowUserPointer(window, this);
	auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };
	auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_move({ _0, _1 }); };
	auto mouse_button_pressed_redirect = [](GLFWwindow* wnd, int _button, int _action, int _mods) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_button_pressed(_button, _action, _mods); };
	
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);
	glfwSetMouseButtonCallback(window, mouse_button_pressed_redirect);

	return window;
}

bool WorldSystem::start_and_load_sounds() {
	
	//////////////////////////////////////
	// Loading music and sounds with SDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Failed to initialize SDL Audio");
		return false;
	}

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
		fprintf(stderr, "Failed to open audio device");
		return false;
	}

	background_music = Mix_LoadMUS(audio_path("music.wav").c_str());
	chicken_dead_sound = Mix_LoadWAV(audio_path("chicken_dead.wav").c_str());
	chicken_eat_sound = Mix_LoadWAV(audio_path("chicken_eat.wav").c_str());

	if (background_music == nullptr || chicken_dead_sound == nullptr || chicken_eat_sound == nullptr) {
		fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n make sure the data directory is present",
			audio_path("music.wav").c_str(),
			audio_path("chicken_dead.wav").c_str(),
			audio_path("chicken_eat.wav").c_str());
		return false;
	}

	return true;
}

void WorldSystem::init(RenderSystem* renderer_arg) {

	this->renderer = renderer_arg;
// CK: disabled starting music for A2
#if 0
	// start playing background music indefinitely
	std::cout << "Starting music..." << std::endl;
	Mix_PlayMusic(background_music, -1);
#endif

	// Set all states to default
    restart_game();
}

// A2: separated from WorldSystem::step for screens that do not use ::step
void WorldSystem::update_window_caption() {

	// update window title with various state variables
	std::stringstream title_ss;
	title_ss << "Level: " << world_level_filename;
	title_ss << " | " << "Screen: " << GAME_SCREEN_ID_NAMES[(int)game_screen];
	title_ss << " | " << "Points: " << points;
	title_ss << " | " << "Max Towers: " << max_towers;
	title_ss << " | " << "Debug: " << (debugging.in_debug_mode ? "true" : "false");
	glfwSetWindowTitle(window, title_ss.str().c_str());
}

// Update our game world
bool WorldSystem::step(float elapsed_ms_since_last_update) {
	if (game_screen == GAME_SCREEN_ID::PLAYING) {	
		clearAllText();

		// see if an invader has reached the end
		for (Entity invader : registry.invaders.entities) {
			if (registry.walkingPaths.has(invader)) {
				WalkingPath& wp = registry.walkingPaths.get(invader);
				if (wp.path.empty()) {
					game_screen = GAME_SCREEN_ID::END;
					break;
				}
			}
		}

		for (Entity invader : registry.invaders.entities) {
			Motion& m = registry.motions.get(invader);
			vec2 pos = m.position - vec2(m.scale.x / 2.f, m.scale.y / 2.f);

			if (!registry.points.has(invader)) {
				Entity e = Entity();
				Text& t = registry.texts.emplace(e);
				auto s = std::to_string(registry.invaders.get(invader).points);
				t.content = s;
				t.color = { 0, 0, 0 };
				Motion& m = registry.motions.emplace(e);
				m.position = pos;
				m.scale = { 0.75, 0.75 };


				Points p;
				p.text = e;
				registry.points.emplace(invader, p);
			}
			else {
				// Points& p = registry.points.get(invader);

				Entity e = Entity();
				Text& t = registry.texts.emplace(e);
				auto s = std::to_string(registry.invaders.get(invader).points);
				t.content = s;
				t.color = { 0, 0, 0 };
				Motion& m = registry.motions.emplace(e);
				m.position = pos;
				m.scale = { 0.75, 0.75 };


			}
		}

		for (Entity e : registry.explosions.entities) {
			Explosion& exp = registry.explosions.get(e);
			// Decrement the frame timer by the elapsed time (in ms).
			exp.timer -= elapsed_ms_since_last_update;

			// When the timer reaches 0 or below, advance to the next frame.
			if (exp.timer <= 0.f) {
				exp.frame++;
				// Reset the timer for the next frame (assume 333ms per frame).
				exp.timer = 333.3f;

				if (exp.frame == 1) {
					if (registry.renderRequests.has(e)) {
						RenderRequest& req = registry.renderRequests.get(e);
						req.used_texture = TEXTURE_ASSET_ID::EXPLOSION2;
					}
				}
				else if (exp.frame == 2) {
					if (registry.renderRequests.has(e)) {
						RenderRequest& req = registry.renderRequests.get(e);
						req.used_texture = TEXTURE_ASSET_ID::EXPLOSION3;
					}
				}
				else {
					registry.remove_all_components_of(e);
				}
			}
		}



		// Check if an invader needs to be respawned
		if (invaders_remaining > 0) {
			addUnspawnedText(invaders_remaining);
			next_invader_spawn -= elapsed_ms_since_last_update;
			if (next_invader_spawn <= 0) {
				// Generate a small random offset so invaders don't spawn exactly on top of one another.
				Entity invader = createInvader(renderer, spawn_start_position);
				WalkingPath& invader_path = registry.walkingPaths.emplace(invader);
				invader_path.path = spawn_path;

				invaders_remaining--;

				next_invader_spawn = INVADER_SPAWN_RATE_MS;
			}
		}
		if (invaders_remaining == 0 && registry.invaders.entities.empty()) {
			// start_game();
			game_screen = GAME_SCREEN_ID::END;
			victory = true;
		}
		addScoreText(this->score);
	}
	if (game_screen == GAME_SCREEN_ID::END) {
		ScreenState& screen = registry.screenStates.components[0];
		if (screen.darken_screen_factor < 0.8f) {
			screen.darken_screen_factor += 0.01f;
			if (screen.darken_screen_factor > 0.8f) {
				screen.darken_screen_factor = 1;
			}
		}
		if (victory) {
			addVictoryText();
		}
		else {
			addGameOverText();
		}
		
	}
	
	if (game_screen == GAME_SCREEN_ID::INTRO) {
		addTextForIntro(level);
	}
	return true;
}

void WorldSystem::clearAllText()
{
	for (Entity e : registry.texts.entities) {
		registry.remove_all_components_of(e);
	}
	registry.texts.clear();
}

void WorldSystem::addUnspawnedText(int invadersUnspawned)
{
	Entity e = Entity();
	Text& t = registry.texts.emplace(e);
	auto s = std::to_string(invadersUnspawned);
	t.content = "Invaders Unspawned: " + s;
	t.color = { 0, 0, 0 };
	Motion& m = registry.motions.emplace(e);
	m.position = { 400, 40 };
	m.scale = { 0.75, 0.75 };
}

void WorldSystem::addScoreText(int score)
{
	Entity e = Entity();
	Text& t = registry.texts.emplace(e);
	auto s = std::to_string(score);
	t.content = "Score: " + s;
	t.color = { 0, 0, 0 };
	Motion& m = registry.motions.emplace(e);
	m.position = { 50, 40 };
	m.scale = {0.75, 0.75 };
}

void WorldSystem::addTextForIntro(int level)
{
	Entity e = Entity();
	Text& t = registry.texts.emplace(e);
	t.content = "A Game by Mana Longhenry";
	t.color = { 1, 1, 1 };
	Motion& m = registry.motions.emplace(e);
	m.position = { 300, 100 };
	m.scale = { 1, 1 };

	Entity e1 = Entity();
	Text& t1 = registry.texts.emplace(e1);
	auto s = std::to_string(level);
	t1.content = "Level: " + s;
	t1.color = { 1, 1, 1 };
	Motion& m1 = registry.motions.emplace(e1);
	m1.position = { 300, 200 };
	m1.scale = { 1, 1 };

	Entity e2 = Entity();
	Text& t2 = registry.texts.emplace(e2);
	t2.content = "Help: ";
	t2.color = { 1, 1, 1 };
	Motion& m2 = registry.motions.emplace(e2);
	m2.position = { 300, 300 };
	m2.scale = { 1, 1 };

	Entity e3 = Entity();
	Text& t3 = registry.texts.emplace(e3);
	t3.content = "0 - 9 changes the level";
	t3.color = { 1, 1, 1 };
	Motion& m3 = registry.motions.emplace(e3);
	m3.position = { 300, 350 };
	m3.scale = { 0.75, 0.75 };

	Entity e4 = Entity();
	Text& t4 = registry.texts.emplace(e4);
	t4.content = "Space to start game";
	t4.color = { 1, 1, 1 };
	Motion& m4 = registry.motions.emplace(e4);
	m4.position = { 300, 400 };
	m4.scale = { 0.75, 0.75 };

	Entity e5 = Entity();
	Text& t5 = registry.texts.emplace(e5);
	t5.content = "G - Generate random level and start game";
	t5.color = { 1, 1, 1 };
	Motion& m5 = registry.motions.emplace(e5);
	m5.position = { 300, 450 };
	m5.scale = { 0.75, 0.75 };

	Entity e6 = Entity();
	Text& t6 = registry.texts.emplace(e6);
	t6.content = "R - Restart current level (when playing)";
	t6.color = { 1, 1, 1 };
	Motion& m6 = registry.motions.emplace(e6);
	m6.position = { 300, 500 };
	m6.scale = { 0.75, 0.75 };

	Entity e7 = Entity();
	Text& t7 = registry.texts.emplace(e7);
	t7.content = "Esc - Return to intro or exit game";
	t7.color = { 1, 1, 1 };
	Motion& m7 = registry.motions.emplace(e7);
	m7.position = { 300, 550 };
	m7.scale = { 0.75, 0.75 };
}

void WorldSystem::addIntroError(int level)
{
	clearAllText();
	auto s = std::to_string(level);

	Entity e = Entity();
	Text& t = registry.texts.emplace(e);
	t.content = "Error! This level is not a valid map: " + s;
	t.color = { 1, 0, 0 };
	Motion& m = registry.motions.emplace(e);
	m.position = { 300, 250 };
	m.scale = { 0.75, 0.75 };
}

void WorldSystem::addGameOverText()
{
	Entity e = Entity();
	Text& t = registry.texts.emplace(e);
	t.content = "GAME OVER";
	t.color = { 1, 0, 0 };
	Motion& m = registry.motions.emplace(e);
	m.position = { 500, 250 };
	m.scale = { 1, 1 };

	Entity e1 = Entity();
	Text& t1 = registry.texts.emplace(e1);
	t1.content = "Press ESC to go back to the intro screen";
	t1.color = { 1, 1, 1 };
	Motion& m1 = registry.motions.emplace(e1);
	m1.position = { 300, 400 };
	m1.scale = { 0.75, 0.75 };
}

void WorldSystem::addVictoryText()
{
	Entity e = Entity();
	Text& t = registry.texts.emplace(e);
	t.content = "VICTORY";
	t.color = { 0, 1, 0 };
	Motion& m = registry.motions.emplace(e);
	m.position = { 500, 250 };
	m.scale = { 1, 1 };

	Entity e1 = Entity();
	Text& t1 = registry.texts.emplace(e1);
	t1.content = "Press ESC to go back to the intro screen";
	t1.color = { 1, 1, 1 };
	Motion& m1 = registry.motions.emplace(e1);
	m1.position = { 300, 400 };
	m1.scale = { 0.75, 0.75 };
}

void WorldSystem::end_game()
{

	game_screen = GAME_SCREEN_ID::END;
	
}

void WorldSystem::createExplosion(vec2 position)
{
	auto entity = Entity();

	auto& e = registry.explosions.emplace(entity);
	e.timer = 333.3f;
	e.frame = 0;

	// Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Initialize the motion
	auto& motion = registry.motions.emplace(entity);
	motion.angle = 0;	// A1-TD: CK: rotate to the left 180 degrees to fix orientation
	motion.velocity = { 0.0f, 0.0f };
	motion.position = position;

	motion.scale = vec2({100, 100});

	registry.deadlys.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{
			TEXTURE_ASSET_ID::EXPLOSION1,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE
		}
	);


}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// TODO A2: find first start tile and return true if found with location set in start_tile param
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
bool WorldSystem::find_first_start_tile(glm::ivec2& start_tile)
{
	std::cout << "Searching for Start Tile..." << std::endl;

	for (Entity entity : registry.tiles.entities) {
		Tile& tile = registry.tiles.get(entity);
		// std::cout << "Checking Tile at (" << tile.tx << ", " << tile.ty << ") with ID " << (int)tile.tile_id << std::endl;

		if (START_TILES.find(tile.tile_id) != START_TILES.end()) {
			start_tile = glm::ivec2(tile.tx, tile.ty);
			//std::cout << "Start Tile Found at (" << start_tile.x << ", " << start_tile.y << ")" << std::endl;
			return true;
		}
	}

	return false;
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// TODO A2: find first exit tile and return true if found with location set in exit_tile param
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
bool WorldSystem::find_first_exit_tile(glm::ivec2& exit_tile)
{
	for (Entity entity : registry.tiles.entities) {
		Tile& tile = registry.tiles.get(entity);
		// std::cout << "Checking Tile at (" << tile.tx << ", " << tile.ty << ") with ID " << (int)tile.tile_id << std::endl;

		if (EXIT_TILES.find(tile.tile_id) != EXIT_TILES.end()) {
			exit_tile = glm::ivec2(tile.tx, tile.ty);
			// std::cout << "Exit Tile Found at (" << exit_tile.x << ", " << exit_tile.y << ")" << std::endl;
			return true;
		}
	}
	return false;
}

ivec2 WorldSystem::getStartTile()
{
	return startTile;
}




// Reset the world state to its initial state
void WorldSystem::restart_game() {

	std::cout << "Restarting..." << std::endl;

	// debugging for memory/component leaks
	// std::cout << "Current registry Entities, before restart" << std::endl;
	registry.list_all_components();

	// reset the game speed
	current_speed = 1.f;

	points = 0;
	max_towers = MAX_TOWERS_START;
	next_invader_spawn = 0;
	invader_spawn_rate_ms = INVADER_SPAWN_RATE_MS;

	// remove all motion entities
	while (registry.motions.entities.size() > 0)
	    registry.remove_all_components_of(registry.motions.entities.back());

	// A2: remove the filled tiles too (b/c they do not have motion)
	//     legacy - only motion elements were removed, but we need to remove other things too
	while (registry.filledTiles.entities.size() > 0)
		registry.remove_all_components_of(registry.filledTiles.entities.back());

	// debugging for memory/component leaks
	// std::cout << "Registry Entities after restart" << std::endl;
	registry.list_all_components();


	if (!registry.screenStates.entities.empty()) {
		ScreenState& screen = registry.screenStates.get(registry.screenStates.entities[0]);
		screen.darken_screen_factor = 0.0f;
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// A1: create grid lines (fixed in A2 template)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// 
	// create grid lines if they do not already exist
	// NOTE: grid lines 'grow' from the center of their location, so must center lines
	//       move center 1/2 cell height so we can remove the top row
	int center_vertical = (WINDOW_HEIGHT_PX / 2) + (GRID_CELL_HEIGHT_PX / 2);

	if (game_screen != GAME_SCREEN_ID::INTRO) {
		if (grid_lines.size() == 0) {
			vec3 grid_line_color = { 0.5f, 0.5f, 0.5f };

			// vertical lines
			for (int col = 1; col < NUM_GRID_CELLS_WIDE; col++) {
				grid_lines.push_back(createGridLine(
					vec2(col * GRID_CELL_WIDTH_PX, center_vertical),
					vec2(GRID_LINE_WIDTH_PX, WINDOW_HEIGHT_PX - GRID_CELL_HEIGHT_PX), // remove the top row
					grid_line_color)
				);
			}

			// horizontal lines (from row 1, not row 0)
			for (int row = 1; row < NUM_GRID_CELLS_HIGH + 1; row++) {
				grid_lines.push_back(createGridLine(
					vec2(0, row * GRID_CELL_HEIGHT_PX),
					vec2(2 * WINDOW_WIDTH_PX, GRID_LINE_WIDTH_PX),
					grid_line_color)
				);
			}
		}
	}

	

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: create the selectable tiles unless we have already done so
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	
	// only create the selectable tiles once
	int tx = 0;
	int ty = 1;

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// A2: automatically load current level, if it exists, at restart
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	std::cout << "INFO: Reloading level file: " << world_level_filename << std::endl;
	if (!load_level(world_level_filename)) {
		std::cout << "ERROR: failed to load level file: " << world_level_filename << std::endl;
	}

	int invaders_remaining = 10 * (level + 1);

}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// TODO A2: search for a path from start to exit, only on the yellow tile lines
//     If a path is found, return true and store it in reverse order from
//     exit to start in the vector of ivec2 tile coordinates.
//     If a path is not found, return false.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
bool WorldSystem::find_path(std::vector<ivec2> & path)
{
	// TODO: use A*
	// TODO: return true when a path is found and return the path via the &path vector

	glm::ivec2 start_tile, exit_tile;
	if (!find_first_start_tile(start_tile) || !find_first_exit_tile(exit_tile)) {
		std::cout << "No valid start or exit tile found" << std::endl;
		return false;
	}

	std::unordered_set<ivec2> closed_set;
	std::unordered_map<ivec2, ivec2> parent_map;

	std::priority_queue<Node, std::vector<Node>, std::function<bool(Node, Node)>> open_list(
		[](const Node& a, const Node& b) { return a.get_score() > b.get_score(); }
	);

	open_list.emplace(start_tile, start_tile, 0, glm::distance(glm::vec2(start_tile), glm::vec2(exit_tile)));
	parent_map[start_tile] = start_tile;  // Start node has no real parent

	while (!open_list.empty()) {
		Node current = open_list.top();
		open_list.pop();

		if (current.position == exit_tile) {
			path.clear();
			glm::ivec2 step = exit_tile;
			while (step != start_tile) {
				path.push_back(step);
				step = parent_map[step];  // Move to parent tile
			}
			path.push_back(start_tile);
			std::reverse(path.begin(), path.end());
			// std::cout << "Path Found. Steps: " << path.size() << "\n";
			return true;
		}

		closed_set.insert(current.position);

		// Explore neighbors 
		std::vector<ivec2> neighbors = {
			{current.position.x, current.position.y - 1},
			{current.position.x, current.position.y + 1},
			{current.position.x - 1, current.position.y},
			{current.position.x + 1, current.position.y}
		};

		for (const ivec2& neighbor : neighbors) {
			if (closed_set.find(neighbor) != closed_set.end()) {
				continue;  // Skip visited tiles
			}
			Tile* neighborTilePtr = nullptr;
			Tile* currentTilePtr = nullptr;
			for (Entity entity : registry.tiles.entities) {
				Tile& tile = registry.tiles.get(entity);
				if (tile.tx == neighbor.x && tile.ty == neighbor.y) {
					neighborTilePtr = &tile;
				}
				if (tile.tx == current.position.x && tile.ty == current.position.y) {
					currentTilePtr = &tile;
				}
				if (neighborTilePtr && currentTilePtr)
					break;
			}

			if (!neighborTilePtr || !currentTilePtr)
				continue;

			glm::ivec2 direction = neighbor - current.position;
			if (!canConnect(*currentTilePtr, *neighborTilePtr, direction))
				continue;

			uint g = current.G + 1;
			uint h = glm::distance(glm::vec2(neighbor), glm::vec2(exit_tile));
			open_list.emplace(neighbor, current.position, g, h);
			parent_map[neighbor] = current.position;
		}
	}

	std::cout << "No valid path found\n";
	return false;
}

bool WorldSystem::canConnect(const Tile& current, const Tile& neighbor, const glm::ivec2& direction)
{
	auto current_it = TILE_DIRECTIONS.find(current.tile_id);
	auto neighbor_it = TILE_DIRECTIONS.find(neighbor.tile_id);
	if (current_it == TILE_DIRECTIONS.end() || neighbor_it == TILE_DIRECTIONS.end()) {
		return false;
	}
	tile_dir currentDir = current_it->second;
	tile_dir neighborDir = neighbor_it->second;


	if (direction == glm::ivec2(0, -1)) {         //  up
		return currentDir.top && neighborDir.bottom;
	}
	else if (direction == glm::ivec2(0, 1)) {     //down
		return currentDir.bottom && neighborDir.top;
	}
	else if (direction == glm::ivec2(-1, 0)) {    // left
		return currentDir.left && neighborDir.right;
	}
	else if (direction == glm::ivec2(1, 0)) {     // right
		return currentDir.right && neighborDir.left;
	}
	return false;
}
	


// Compute collisions between entities
void WorldSystem::handle_collisions() {

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A1: Loop over all collisions detected by the physics system
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	ComponentContainer<Collision>& collision_container = registry.collisions;
	for (uint i = 0; i < collision_container.components.size(); i++) {

		Entity e1 = collision_container.entities[i];
		Entity e2 = collision_container.components[i].other;

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// TODO A1: handle collision between projectile and invader
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// Mix_PlayChannel(-1, chicken_dead_sound, 0);

		if ((registry.projectiles.has(e1) && registry.invaders.has(e2)) || (registry.invaders.has(e1) && registry.projectiles.has(e2))) {
			Entity projectile = registry.projectiles.has(e1) ? e1 : e2;
			Entity invader = registry.invaders.has(e1) ? e1 : e2;
			Motion& m = registry.motions.get(invader);

			std::cout << "Projectile hit an invader!" << std::endl;


			registry.remove_all_components_of(projectile);

			Invader& invader_component = registry.invaders.get(invader);
			invader_component.health -= PROJECTILE_DAMAGE;

			if (invader_component.health <= 0) {
				score += invader_component.points;

				createExplosion(m.position);

				registry.remove_all_components_of(invader);
				Mix_PlayChannel(-1, chicken_dead_sound, 0);
			}
			else {
				std::cout << "Invader took damage! Remaining Health: " << invader_component.health << std::endl;
			}
		}

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// TODO A1: handle collision between tower and invader
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// Mix_PlayChannel(-1, chicken_eat_sound, 0);

		if (registry.invaders.has(e1) && registry.towers.has(e2)) {
			registry.remove_all_components_of(e2);
			registry.remove_all_components_of(e1);

			if (max_towers > 0) {
				max_towers--;
			}

			Mix_PlayChannel(-1, chicken_eat_sound, 0);
			std::cout << "Tower destroyed! Remaining max towers: " << max_towers << std::endl;

			// assert(registry.screenStates.components.size() <= 1);
			if (!registry.screenStates.components.empty()) {
				//registry.screenStates.components[0].vignette_intensity = 1.0f;
				/*std::cout << "Vignette triggered! Intensity set to: "
					<< registry.screenStates.components[0].vignette_intensity << std::endl;*/
				Entity vignetteEntity = Entity();
				registry.deathTimers.emplace(vignetteEntity, DeathTimer{ 1000.0f });
				
			}
		}
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: When invaders reach the exit, their walkable path will be empty
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// Done in physics

	registry.collisions.clear();
}

// Should the game be over ?
bool WorldSystem::is_over() const {
	return bool(glfwWindowShouldClose(window));
}

// on key callback
void WorldSystem::on_key(int key, int, int action, int mod) {

	if (key == GLFW_KEY_R) {
		if (game_screen == GAME_SCREEN_ID::PLAYING) {
			restart_game();
			score = 0;
			int invaders_remaining = 0;
		}
		if (game_screen == GAME_SCREEN_ID::END) {
			return;
		}
	}

	

	if (key == GLFW_KEY_SPACE && (game_screen == GAME_SCREEN_ID::INTRO) && validLevel) {
		game_screen = GAME_SCREEN_ID::DRAWING;
		registry.texts.clear();
		restart_game();
	}


	// ESC - exit game
	if (action == GLFW_RELEASE && key == GLFW_KEY_ESCAPE) {
		if (game_screen != GAME_SCREEN_ID::INTRO) {
			while (!registry.invaders.entities.empty()) {
				Entity e = registry.invaders.entities.back();
				registry.remove_all_components_of(e);
			}
			std::vector<Entity> motionsToClear = registry.motions.entities;
			for (Entity e : motionsToClear) {
				registry.remove_all_components_of(e);
			}
			while (!registry.filledTiles.entities.empty()) {
				Entity e = registry.filledTiles.entities.back();
				registry.remove_all_components_of(e);
			}

			for (Entity grid : grid_lines) {
				registry.remove_all_components_of(grid);
			}
			grid_lines.clear();

			game_screen = GAME_SCREEN_ID::INTRO;
			clearAllText();
			addTextForIntro(level);
		}
		else {
			close_window();
		}
	}

	// SHIFT - record whether the SHIFT key is depressed or not to support macOS "right-click" --> SHIFT + left-click
	if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
		if (action == GLFW_RELEASE)
			shift_key_pressed = false;
		else
			shift_key_pressed = true;
	}

	// D - Debugging - not used in A1, but left intact for the debug lines
	if (key == GLFW_KEY_D) {
		if (action == GLFW_RELEASE) {
			if (debugging.in_debug_mode) {
				debugging.in_debug_mode = false;
				std::cout << "INFO: debug_mode disabled" << std::endl;
			}
			else {
				debugging.in_debug_mode = true;
				std::cout << "INFO: debug_mode enabled" << std::endl;
			}
		}
	}

	// A2: E - tile selector toggle
	if (key == GLFW_KEY_E) {
		if (action == GLFW_RELEASE) {
			clear_filled_tiles();

			// toggle between the drawing screen and the tile selector screen
			if (game_screen == GAME_SCREEN_ID::DRAWING) {
				game_screen = GAME_SCREEN_ID::TILE_SELECTOR;
				std::cout << "tile-selector screen" << std::endl;

				// ADDED
				if (registry.selectables.size() == 0) {
					int columns = WINDOW_WIDTH_PX / GRID_CELL_WIDTH_PX;
					int tile_size = GRID_CELL_WIDTH_PX;
					int row = 0, col = 0;
					int start_x = 0, start_y = 100;

					for (int tile_id = (int)TEXTURE_ASSET_ID::MAPTILE_121; tile_id <= (int)TEXTURE_ASSET_ID::MAPTILE_281; tile_id++) {
						vec2 position = vec2(start_x + col * tile_size, start_y + row * tile_size);
						createSelectableTile(renderer, position, (TEXTURE_ASSET_ID)tile_id);
						                                      
						col++;
						if (col >= columns) {
							col = 0;
							row++;
						}
					}
				}
				else if (game_screen == GAME_SCREEN_ID::TILE_SELECTOR) {
					game_screen = GAME_SCREEN_ID::DRAWING;
					for (Entity entity : registry.selectables.entities) {
						registry.remove_all_components_of(entity);
					}
				}

			}
			else if (game_screen == GAME_SCREEN_ID::TILE_SELECTOR) {
				game_screen = GAME_SCREEN_ID::DRAWING;
				std::cout << "level drawing screen" << std::endl;

				// ADDED
				for (Entity entity : registry.selectables.entities) {
					registry.remove_all_components_of(entity);
				}
			}
		}
	}

	// A2: K - save your level map
	if (key == GLFW_KEY_K) {
		if (action == GLFW_RELEASE) {
			if (save_level(world_level_filename)) {
				std::cout << "level saved to: " << world_level_filename << std::endl;
			}
			else {
				std::cout << "ERROR: failed to save level: " << world_level_filename << std::endl;
			}
		}
	}

	// A2: L - load level map
	if (key == GLFW_KEY_L) {
		if (action == GLFW_RELEASE) {
			if (load_level(world_level_filename)) {
				std::cout << "INFO: level loaded: " << world_level_filename << std::endl;
			}
			else {
				std::cout << "ERROR: failed to load level: " << world_level_filename << std::endl;
			}
		}
	}

	// A2: O - calculate path manually
	if (action == GLFW_RELEASE && key == GLFW_KEY_O && game_screen == GAME_SCREEN_ID::DRAWING) {
		std::vector<glm::ivec2> final_path;
		std::unordered_set<glm::ivec2> visited_tiles;
		if (showpath) {
			showpath = false;
			bool path_found = find_path_with_visitation(final_path, visited_tiles);

			clear_filled_tiles();

			for (const auto& tile_coord : visited_tiles) {
				vec2 tile_pos = { tile_coord.x * GRID_CELL_WIDTH_PX, tile_coord.y * GRID_CELL_HEIGHT_PX };
				createFilledTile(renderer, tile_pos, vec2(GRID_CELL_WIDTH_PX, GRID_CELL_HEIGHT_PX), vec3(0, 0, 1));
			}

			if (path_found) {
				for (const auto& tile_coord : final_path) {
					vec2 tile_pos = { tile_coord.x * GRID_CELL_WIDTH_PX, tile_coord.y * GRID_CELL_HEIGHT_PX };
					createFilledTile(renderer, tile_pos, vec2(GRID_CELL_WIDTH_PX, GRID_CELL_HEIGHT_PX), vec3(1, 0, 1));
				}
				vec2 start_pos = { final_path.front().x * GRID_CELL_WIDTH_PX, final_path.front().y * GRID_CELL_HEIGHT_PX };
				createFilledTile(renderer, start_pos, vec2(GRID_CELL_WIDTH_PX, GRID_CELL_HEIGHT_PX), vec3(0, 1, 0));

				vec2 exit_pos = { final_path.back().x * GRID_CELL_WIDTH_PX, final_path.back().y * GRID_CELL_HEIGHT_PX };
				createFilledTile(renderer, exit_pos, vec2(GRID_CELL_WIDTH_PX, GRID_CELL_HEIGHT_PX), vec3(1, 0, 0));
			}
			else {
				std::cout << "No valid path found, not displaying a final magenta path" << std::endl;
			}
		}
		else {
			showpath = true;
			clear_filled_tiles();
		}
		
	}

	// A2: P - switch to playing mode
	if (key == GLFW_KEY_P) {
		if (action == GLFW_RELEASE) {
			// Toggle between playing and drawing modes
			if (game_screen == GAME_SCREEN_ID::DRAWING) {
				clear_filled_tiles();
				game_screen = GAME_SCREEN_ID::PLAYING;
				std::cout << "playing screen" << std::endl;

				// Only spawn an invader if none is currently active
				if (registry.invaders.entities.empty()) {
					start_game();
				}
			}
			else if (game_screen == GAME_SCREEN_ID::PLAYING) {
				clear_filled_tiles();
				game_screen = GAME_SCREEN_ID::DRAWING;

				std::cout << "level drawing screen" << std::endl;
			}
		}
	}

	// R - Resetting game
	if (action == GLFW_RELEASE && key == GLFW_KEY_R) {
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		restart_game();
	}

	// A2: number keys for different levels
	if (action == GLFW_RELEASE && (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)) {
		if (game_screen == GAME_SCREEN_ID::PLAYING) {
			return;
		}
		int key_num = key - GLFW_KEY_0;
		world_level_filename = "level" + std::to_string(key_num) + ".txt";

		std::string fullPath = std::string(PROJECT_SOURCE_DIR) + "data/levels/" + world_level_filename;
		std::ifstream testFile(fullPath);

		level = key_num;

		validLevel = testFile.is_open();

		if (game_screen == GAME_SCREEN_ID::INTRO) {
			clearAllText();
			addTextForIntro(level);
			if (!testFile.is_open()) {
				addIntroError(key_num);
			}
		}
		else {
			if (!testFile.is_open()) {
				clearAllText();
				addIntroError(key_num);
			}
			else {
				restart_game();
			}
		}
		
	}

}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// TODO A2: load a level file
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
bool WorldSystem::load_level(const std::string& filename) {

	std::cout << "Loading level file: " << filename << std::endl;

	std::ifstream ifs(std::string(PROJECT_SOURCE_DIR) + "data/levels/" + filename);
	if (!ifs.is_open()) {
		std::cout << "ERROR: Could not open file: " << filename << std::endl;
		return false;
	}

	std::vector<Entity> toRemove = registry.motions.entities;
	for (Entity e : toRemove) {
		if (!registry.selectables.has(e)) {
			registry.remove_all_components_of(e);
		}
	}

	std::string line;
	while (std::getline(ifs, line)) {
		std::stringstream ss(line);
		std::string token;
		ss >> token;
		if (token == "tile") {
			int tx, ty, tex_id;
			ss >> tx >> ty >> tex_id;
			float px = tx * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f;
			float py = ty * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f;
			createLevelTile(renderer, vec2(px, py), (TEXTURE_ASSET_ID)tex_id);
		}
	}

	ifs.close();
	std::cout << "Level loaded from " << filename << std::endl;
	return true;
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// TODO A2: save a level file
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
bool WorldSystem::save_level(const std::string& filename) {

	std::cout << "Saving level file: " << filename << std::endl;
	std::ofstream ofs(std::string(PROJECT_SOURCE_DIR) + "data/levels/" + filename);
	if (!ofs.is_open()) {
		std::cout << "ERROR: Could not open file for saving: " << filename << std::endl;
		return false;
	}

	for (Entity e : registry.motions.entities) {
		if (registry.selectables.has(e))
			continue;

		Motion& motion = registry.motions.get(e);
		if (!registry.renderRequests.has(e))
			continue;

		RenderRequest& req = registry.renderRequests.get(e);
		int tile_x = int(motion.position.x / GRID_CELL_WIDTH_PX);
		int tile_y = int(motion.position.y / GRID_CELL_HEIGHT_PX);
		if (tile_y == 0)
			continue;

		if (req.used_texture == TEXTURE_ASSET_ID::TEXTURE_COUNT)
			continue;

		// ofs << "tile " << tile_x << " " << tile_y << " " << (int)req.used_texture << "\n";
	}

	ofs.close();
	// std::cout << "Level saved to " << filename << std::endl;
	return true;
}

void WorldSystem::on_mouse_move(vec2 mouse_position) {

	mouse_pos_x = mouse_position.x;
	mouse_pos_y = mouse_position.y;
}

void WorldSystem::on_mouse_button_pressed(int button, int action, int mods) {
	if (action != GLFW_PRESS) return;

	int tile_x = (int)(mouse_pos_x / GRID_CELL_WIDTH_PX);
	int tile_y = (int)(mouse_pos_y / GRID_CELL_HEIGHT_PX);

	std::cout << "mouse position: " << mouse_pos_x << ", " << mouse_pos_y << std::endl;
	std::cout << "mouse tile position: " << tile_x << ", " << tile_y << std::endl;

	if (game_screen == GAME_SCREEN_ID::TILE_SELECTOR && button == GLFW_MOUSE_BUTTON_LEFT) {
		for (Entity entity : registry.selectables.entities) {
			Tile& tile = registry.tiles.get(entity);

			if (tile.tx == tile_x && tile.ty == tile_y) {
				drawing_tile = tile.tile_id;  
				//std::cout << "Selected tile ID: " << (int)drawing_tile << "\n";
				return;  
			}
		}
	}

	if (game_screen == GAME_SCREEN_ID::PLAYING && button == GLFW_MOUSE_BUTTON_LEFT) {
		if (tile_y > 0) {
			bool towerExists = false;
			for (Entity e : registry.towers.entities) {
				Motion& m = registry.motions.get(e);
				int ex = (int)(m.position.x / GRID_CELL_WIDTH_PX);
				int ey = (int)(m.position.y / GRID_CELL_HEIGHT_PX);
				if (ex == tile_x && ey == tile_y) {
					towerExists = true;
					break;
				}
			}
			bool tileExists = false;
			for (Entity e : registry.tiles.entities) {
				Tile& t = registry.tiles.get(e);
				if (t.tx == tile_x && t.ty == tile_y) {
					tileExists = true;
					break;
				}
			}
			if (!towerExists && !tileExists && registry.towers.size() < 5) {
				vec2 pos(tile_x * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f,
					tile_y * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f);
				createTower(renderer, pos);
			}
		}
	}
	if (game_screen == GAME_SCREEN_ID::PLAYING && (button == GLFW_MOUSE_BUTTON_RIGHT || (button == GLFW_MOUSE_BUTTON_LEFT && shift_key_pressed))) {
		vec2 center(tile_x * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f, tile_y * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f);
		removeTower(center);
	}


	if (game_screen == GAME_SCREEN_ID::DRAWING) {
		if (tile_y == 0) return;  

		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			remove_tile(tile_x, tile_y);  
			place_tile(tile_x, tile_y, drawing_tile);  
		}

		if (button == GLFW_MOUSE_BUTTON_RIGHT || (button == GLFW_MOUSE_BUTTON_LEFT && shift_key_pressed)) {
			remove_tile(tile_x, tile_y);  
		}
	}
}

void WorldSystem::remove_tile(int x, int y) {
	bool tile_removed = false;
	for (Entity e : registry.tiles.entities) {
		Tile& tile = registry.tiles.get(e);
		if (tile.tx == x && tile.ty == y) {
			registry.remove_all_components_of(e);
			// std::cout << "Tile removed at (" << x << ", " << y << ")" << std::endl;
			tile_removed = true;
			break;
		}
	}
	if (!tile_removed) {
		// std::cout << "No matching tile found at (" << x << ", " << y << ")" << std::endl;
	}
	
}

void WorldSystem::place_tile(int x, int y, TEXTURE_ASSET_ID tile_type) {
	vec2 position = vec2(x * GRID_CELL_WIDTH_PX, y * GRID_CELL_HEIGHT_PX);
	Entity new_tile = createLevelTile(renderer, position, tile_type);
}

void WorldSystem::start_game() {
	std::cout << "Starting the game" << std::endl;

	glm::ivec2 start_tile, exit_tile;
	if (!find_first_start_tile(start_tile) || !find_first_exit_tile(exit_tile)) {
		std::cout << "ERROR: No valid start or exit tile found! No invader will spawn.\n";
		return;
	}


	std::vector<glm::ivec2> path;
	if (!find_path(path)) {
		std::cout << "ERROR: No valid path found from start to exit! No invader will spawn.\n";
		return;
	}
	ivec2 first_tile = path.front();

	spawn_start_position = vec2((start_tile.x * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f) - GRID_CELL_WIDTH_PX,
		(start_tile.y * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f) - GRID_CELL_HEIGHT_PX);

	// Entity invader = createInvader(renderer, start_position);
	/*Entity invader = createInvader(renderer, spawn_start_position);
	WalkingPath& invader_path = registry.walkingPaths.emplace(invader);
	invader_path.path = path;*/

	invaders_remaining = 10 * (level + 1);
	//invaders_remaining = 5; // for testing

	spawn_path = path;

	next_invader_spawn = 0.f;
}



bool WorldSystem::find_path_with_visitation(std::vector<ivec2>& path, std::unordered_set<ivec2>& visited) {
	glm::ivec2 start_tile, exit_tile;
	if (!this->find_first_start_tile(start_tile) || !this->find_first_exit_tile(exit_tile)) {
		std::cout << "No valid start or exit tile found" << std::endl;
		return false;
	}

	/*std::cout << "Finding path with visitation from (" << start_tile.x << ", " << start_tile.y
		<< ") to (" << exit_tile.x << ", " << exit_tile.y << ")" << std::endl;*/

	std::unordered_set<glm::ivec2> closed_set;
	std::unordered_map<glm::ivec2, glm::ivec2> parent_map;

	std::priority_queue<Node, std::vector<Node>, std::function<bool(Node, Node)>> open_list(
		[](const Node& a, const Node& b) { return a.get_score() > b.get_score(); }
	);

	// Start A* search.
	open_list.emplace(start_tile, start_tile, 0, glm::distance(glm::vec2(start_tile), glm::vec2(exit_tile)));
	parent_map[start_tile] = start_tile;

	visited.clear();

	while (!open_list.empty()) {
		Node current = open_list.top();
		open_list.pop();

		closed_set.insert(current.position);
		visited.insert(current.position);

		if (current.position == exit_tile) {
			path.clear();
			glm::ivec2 step = exit_tile;
			while (step != start_tile) {
				path.push_back(step);
				step = parent_map[step];  
			}
			path.push_back(start_tile);
			std::reverse(path.begin(), path.end());
			// std::cout << "Path found. Steps: " << path.size() << std::endl;
			return true;
		}

		std::vector<glm::ivec2> neighbors = {
			{ current.position.x,     current.position.y - 1 },
			{ current.position.x,     current.position.y + 1 },
			{ current.position.x - 1, current.position.y     },
			{ current.position.x + 1, current.position.y     }
		};

		for (const glm::ivec2& neighbor : neighbors) {
			if (closed_set.find(neighbor) != closed_set.end()) {
				continue;
			}
				  
			Tile* neighborTilePtr = nullptr;
			Tile* currentTilePtr = nullptr;
			for (Entity entity : registry.tiles.entities) {
				Tile& tile = registry.tiles.get(entity);
				if (tile.tx == neighbor.x && tile.ty == neighbor.y)
					neighborTilePtr = &tile;
				if (tile.tx == current.position.x && tile.ty == current.position.y)
					currentTilePtr = &tile;
				if (neighborTilePtr && currentTilePtr)
					break;
			}
			if (!neighborTilePtr || !currentTilePtr) {
				continue;
			}
			
			glm::ivec2 direction = neighbor - current.position;
			if (!canConnect(*currentTilePtr, *neighborTilePtr, direction)) {
				continue;
			}
			uint g = current.G + 1;
			uint h = glm::distance(glm::vec2(neighbor), glm::vec2(exit_tile));
			open_list.emplace(neighbor, current.position, g, h);
			parent_map[neighbor] = current.position;
		}
	}

	std::cout << "no valid path" << std::endl;
	return false;
}

void WorldSystem::clear_filled_tiles() {
	std::vector<Entity> toRemove = registry.filledTiles.entities;
	for (Entity e : toRemove) {
		registry.remove_all_components_of(e);
	}
}