#pragma once
#include <vector>
#include <iostream>

#include "tiny_ecs.hpp"
#include "components.hpp"
#include <unordered_map>

class ECSRegistry
{
	// callbacks to remove a particular or all entities in the system
	std::vector<ContainerInterface*> registry_list;

public:
	// Manually created list of all components this game has
	ComponentContainer<DeathTimer> deathTimers;
	ComponentContainer<Points> points;
	ComponentContainer<Explosion> explosions;
	ComponentContainer<Text> texts;
	ComponentContainer<Character> characters;
	ComponentContainer<Motion> motions;
	ComponentContainer<Collision> collisions;
	ComponentContainer<Player> players;
	ComponentContainer<Mesh*> meshPtrs;
	ComponentContainer<RenderRequest> renderRequests;
	ComponentContainer<ScreenState> screenStates;
	ComponentContainer<Eatable> eatables;
	ComponentContainer<Deadly> deadlys;
	ComponentContainer<DebugComponent> debugComponents;
	ComponentContainer<vec3> colors;
	// IMPORTANT: Add any new CC's below to the registry_list
	ComponentContainer<Tower> towers;
	ComponentContainer<GridLine> gridLines;
	ComponentContainer<Invader> invaders;
	ComponentContainer<Projectile> projectiles;
	ComponentContainer<Tile> tiles;
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// A2: new for A2
	ComponentContainer<FilledTile> filledTiles;
	ComponentContainer<Selectable> selectables;
	ComponentContainer<WalkingPath> walkingPaths;
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// constructor that adds all containers for looping over them
	ECSRegistry()
	{
		registry_list.push_back(&deathTimers);
		registry_list.push_back(&motions);
		registry_list.push_back(&collisions);
		registry_list.push_back(&players);
		registry_list.push_back(&meshPtrs);
		registry_list.push_back(&renderRequests);
		registry_list.push_back(&screenStates);
		registry_list.push_back(&eatables);
		registry_list.push_back(&deadlys);
		registry_list.push_back(&debugComponents);
		registry_list.push_back(&colors);
		registry_list.push_back(&towers);
		registry_list.push_back(&gridLines);
		registry_list.push_back(&invaders);
		registry_list.push_back(&projectiles);
		registry_list.push_back(&tiles);
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// A2
		registry_list.push_back(&filledTiles);
		registry_list.push_back(&selectables);
		registry_list.push_back(&walkingPaths);
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	}

	void clear_all_components() {
		for (ContainerInterface* reg : registry_list)
			reg->clear();
	}

	void list_all_components() {
		printf("Debug info on all registry entries:\n");
		for (ContainerInterface* reg : registry_list)
			if (reg->size() > 0)
				printf("%4d components of type %s\n", (int)reg->size(), typeid(*reg).name());
	}

	void list_all_components_of(Entity e) {
		printf("Debug info on components of entity %u:\n", (unsigned int)e);
		for (ContainerInterface* reg : registry_list)
			if (reg->has(e))
				printf("type %s\n", typeid(*reg).name());
	}

	void remove_all_components_of(Entity e) {
		for (ContainerInterface* reg : registry_list)
			reg->remove(e);
	}

	std::unordered_map<char, Character> character_map;

};

extern ECSRegistry registry;