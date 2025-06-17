#pragma once
#include "common.hpp"
#include <vector>
#include <unordered_map>
#include <cstdlib>
#include "../ext/stb_image/stb_image.h"

// Player component
struct Player
{

};

struct Text {
	std::string content;
	glm::vec3 color;
};

struct Character {
	GLuint TextureID;
	glm::ivec2 Size;
	glm::ivec2 Bearing;
	GLuint Advance;
	char Character;
};

// Tower
struct Tower {
	float range;	// for vision / detection
	int timer_ms;	// when to shoot - this could also be a separate timer component...
};

// Invader
struct Invader {
	int health;
	int points = 1 + (rand() % 5); 
};

struct Points {
	Entity text;
};

// Projectile
struct Projectile {
	int damage;
};

struct Explosion {
	float timer;
	float frame;
};

// =============================================
// CK: ignore these legacy components

// used for Entities that cause damage
struct Deadly
{

};

// used for edible entities
struct Eatable
{

};
// =============================================

// All data relevant to the shape and motion of entities
struct Motion {
	vec2  position = { 0, 0 };
	float angle    = 0;
	vec2  velocity = { 0, 0 };
	vec2  scale    = { 10, 10 };
};

// Stucture to store collision information
struct Collision
{
	// Note, the first object is stored in the ECS container.entities
	Entity other; // the second object involved in the collision
	Collision(Entity& other) { this->other = other; };
};

// Data structure for toggling debug mode
struct Debug {
	bool in_debug_mode = 0;
	bool in_freeze_mode = 0;
};
extern Debug debugging;

// Sets the brightness of the screen
struct ScreenState
{
	float darken_screen_factor = -1;
};

// A struct to refer to debugging graphics in the ECS
struct DebugComponent
{
	// Note, an empty struct has size 1
};

// used to hold grid line start and end positions
struct GridLine {
	vec2 start_pos = {  0,  0 };
	vec2 end_pos   = { 10, 10 };	// default to diagonal line
};

// A timer that will be associated to dying chicken
struct DeathTimer
{
	float counter_ms = 3000;
};

// Single Vertex Buffer element for non-textured meshes (coloured.vs.glsl & chicken.vs.glsl)
struct ColoredVertex
{
	vec3 position;
	vec3 color;
};

// Single Vertex Buffer element for textured sprites (textured.vs.glsl)
struct TexturedVertex
{
	vec3 position;
	vec2 texcoord;
};

// Mesh datastructure for storing vertex and index buffers
struct Mesh
{
	static bool loadFromOBJFile(std::string obj_path, std::vector<ColoredVertex>& out_vertices, std::vector<uint16_t>& out_vertex_indices, vec2& out_size);
	vec2 original_size = {1,1};
	std::vector<ColoredVertex> vertices;
	std::vector<uint16_t> vertex_indices;
};

/**
 * The following enumerators represent global identifiers refering to graphic
 * assets. For example TEXTURE_ASSET_ID are the identifiers of each texture
 * currently supported by the system.
 *
 * So, instead of referring to a game asset directly, the game logic just
 * uses these enumerators and the RenderRequest struct to inform the renderer
 * how to structure the next draw command.
 *
 * There are 2 reasons for this:
 *
 * First, game assets such as textures and meshes are large and should not be
 * copied around as this wastes memory and runtime. Thus separating the data
 * from its representation makes the system faster.
 *
 * Second, it is good practice to decouple the game logic from the render logic.
 * Imagine, for example, changing from OpenGL to Vulkan, if the game logic
 * depends on OpenGL semantics it will be much harder to do the switch than if
 * the renderer encapsulates all asset data and the game logic is agnostic to it.
 *
 * The final value in each enumeration is both a way to keep track of how many
 * enums there are, and as a default value to represent uninitialized fields.
 */

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// A2
enum class TEXTURE_ASSET_ID {
	INVADER_BLUE = 0,
	TOWER = INVADER_BLUE + 1,
	PROJECTILE = TOWER + 1,

	// map tiles - could be a single indexed texture, but individual images are used instead
	// NOTE: we are not using all of the map tiles from the original asset pack, so the 
	//       filename numbers are not sequential nor contiguous
	
	/* general path tiles */
	MAPTILE_121 = PROJECTILE + 1,
	MAPTILE_122 = MAPTILE_121 + 1,
	MAPTILE_123 = MAPTILE_122 + 1,
	MAPTILE_124 = MAPTILE_123 + 1,
	MAPTILE_125 = MAPTILE_124 + 1,
	MAPTILE_126 = MAPTILE_125 + 1,
	MAPTILE_127 = MAPTILE_126 + 1,
	MAPTILE_128 = MAPTILE_127 + 1,
	MAPTILE_129 = MAPTILE_128 + 1,
	MAPTILE_130 = MAPTILE_129 + 1,
	MAPTILE_142 = MAPTILE_130 + 1,
	MAPTILE_143 = MAPTILE_142 + 1,
	MAPTILE_144 = MAPTILE_143 + 1,
	MAPTILE_146 = MAPTILE_144 + 1,
	MAPTILE_147 = MAPTILE_146 + 1,
	
	/* start tiles */
	MAPTILE_155 = MAPTILE_147 + 1,	// connect to last general tile
	MAPTILE_156 = MAPTILE_155 + 1,
	MAPTILE_159 = MAPTILE_156 + 1,
	MAPTILE_160 = MAPTILE_159 + 1,
	MAPTILE_161 = MAPTILE_160 + 1,
	MAPTILE_162 = MAPTILE_161 + 1,
	MAPTILE_163 = MAPTILE_162 + 1,
	MAPTILE_164 = MAPTILE_163 + 1,
	MAPTILE_172 = MAPTILE_164 + 1,
	MAPTILE_173 = MAPTILE_172 + 1,
	MAPTILE_176 = MAPTILE_173 + 1,
	MAPTILE_177 = MAPTILE_176 + 1,
	MAPTILE_178 = MAPTILE_177 + 1,
	MAPTILE_180 = MAPTILE_178 + 1,
	MAPTILE_181 = MAPTILE_180 + 1,
	
	/* exit tiles */
	MAPTILE_255 = MAPTILE_181 + 1,	// connects to last start time
	MAPTILE_256 = MAPTILE_255 + 1,
	MAPTILE_259 = MAPTILE_256 + 1,
	MAPTILE_260 = MAPTILE_259 + 1,
	MAPTILE_261 = MAPTILE_260 + 1,
	MAPTILE_262 = MAPTILE_261 + 1,
	MAPTILE_263 = MAPTILE_262 + 1,
	MAPTILE_264 = MAPTILE_263 + 1,
	MAPTILE_272 = MAPTILE_264 + 1,
	MAPTILE_273 = MAPTILE_272 + 1,
	MAPTILE_276 = MAPTILE_273 + 1,
	MAPTILE_277 = MAPTILE_276 + 1,
	MAPTILE_278 = MAPTILE_277 + 1,
	MAPTILE_280 = MAPTILE_278 + 1,
	MAPTILE_281 = MAPTILE_280 + 1,

	INVADER_RED = MAPTILE_281 + 1,
	INVADER_GREEN = INVADER_RED + 1,

	EXPLOSION1 = INVADER_GREEN + 1,
	EXPLOSION2 = EXPLOSION1 + 1,
	EXPLOSION3 = EXPLOSION2 + 1,

	// TEXTURE_COUNT must always be the last entry
	TEXTURE_COUNT = EXPLOSION3 + 1,
};
const int texture_count = (int)TEXTURE_ASSET_ID::TEXTURE_COUNT;

enum class EFFECT_ASSET_ID {
	COLOURED = 0,
	EGG = COLOURED + 1,
	CHICKEN = EGG + 1,
	TEXTURED = CHICKEN + 1,
	VIGNETTE = TEXTURED + 1,
	FONT = VIGNETTE + 1,
	EFFECT_COUNT = FONT + 1
};
const int effect_count = (int)EFFECT_ASSET_ID::EFFECT_COUNT;

// A2
enum class GAME_SCREEN_ID {
	INTRO = 0,
	DRAWING = INTRO + 1,	
	PLAYING = DRAWING + 1,
	TILE_SELECTOR = PLAYING + 1,
	END = TILE_SELECTOR + 1,
	GAME_SCREEN_COUNT = END + 1
};
const int game_screen_count = (int)GAME_SCREEN_ID::GAME_SCREEN_COUNT;

// indexed to GAME_SCREEN_ID
const std::vector<std::string> GAME_SCREEN_ID_NAMES = {
	"intro",
	"drawing",
	"playing",
	"tile-selector",
	"end",

	"GAME_SCREEN_COUNT"
};

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

enum class GEOMETRY_BUFFER_ID {
	CHICKEN = 0,
	SPRITE = CHICKEN + 1,
	EGG = SPRITE + 1,
	DEBUG_LINE = EGG + 1,
	FILLED_QUAD = DEBUG_LINE + 1,
	SCREEN_TRIANGLE = FILLED_QUAD + 1,
	TEXT = SCREEN_TRIANGLE + 1,

	GEOMETRY_COUNT = TEXT + 1
};
const int geometry_count = (int)GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

struct RenderRequest {
	TEXTURE_ASSET_ID   used_texture  = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID    used_effect   = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;
};

// A2: if part of this group, then selectable
struct Selectable {

};

// A2: a tile definition, in tile-coordinates associated to an existing texture ID
struct Tile {
	int tx;
	int ty;
	TEXTURE_ASSET_ID tile_id;
};

// A2: a filled tile cell
struct FilledTile {
	vec2 pos;
	vec2 size;
	vec3 color;
};

// A2: a walking path of vec2(x, y) coordinates
struct WalkingPath {
	std::vector<ivec2> path;
};
