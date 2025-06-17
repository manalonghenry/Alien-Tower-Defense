#pragma once

#include "common.hpp"
#include "tinyECS/tiny_ecs.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/registry.hpp"

// A simple physics system that moves rigid bodies and checks for collision
class PhysicsSystem
{
public:
	
	// void init(WorldSystem* world);
	void physics_step(float elapsed_ms);

	PhysicsSystem()
	{
	}
private:
	// WorldSystem* world_system = nullptr;
	
};