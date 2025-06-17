// internal
#include "physics_system.hpp"
#include "world_system.hpp"
#include "world_init.hpp"
#include <iostream>


// Returns the local bounding coordinates scaled by the current size of the entity
vec2 get_bounding_box(const Motion& motion)
{
	// abs is to avoid negative scale due to the facing direction.
	return { abs(motion.scale.x), abs(motion.scale.y) };
}

// This is a SUPER APPROXIMATE check that puts a circle around the bounding boxes and sees
// if the center point of either object is inside the other's bounding-box-circle. You can
// surely implement a more accurate detection
bool collides(const Motion& motion1, const Motion& motion2)
{
	vec2 dp = motion1.position - motion2.position;
	float dist_squared = dot(dp,dp);
	const vec2 other_bonding_box = get_bounding_box(motion1) / 2.f;
	const float other_r_squared = dot(other_bonding_box, other_bonding_box);
	const vec2 my_bonding_box = get_bounding_box(motion2) / 2.f;
	const float my_r_squared = dot(my_bonding_box, my_bonding_box);
	const float r_squared = max(other_r_squared, my_r_squared);
	if (dist_squared < r_squared)
		return true;
	return false;
}

void PhysicsSystem::physics_step(float elapsed_ms) {
    auto& motion_registry = registry.motions;
    float step_seconds = elapsed_ms / 1000.f;

    for (int i = (int)motion_registry.components.size() - 1; i >= 0; i--) {
        Motion& motion = motion_registry.components[i];
        Entity entity = motion_registry.entities[i];

        if (registry.invaders.has(entity)) {
            if (registry.walkingPaths.has(entity)) {
                WalkingPath& path = registry.walkingPaths.get(entity);
                if (!path.path.empty()) {
                    // Compute the center of the next tile in the path.
                    glm::ivec2 next_tile = path.path.front();
                    vec2 next_position = vec2(
                        next_tile.x * GRID_CELL_WIDTH_PX + GRID_CELL_WIDTH_PX / 2.f,
                        next_tile.y * GRID_CELL_HEIGHT_PX + GRID_CELL_HEIGHT_PX / 2.f
                    );
                    // Compute direction from current position toward the target.
                    vec2 direction = glm::normalize(next_position - motion.position);
                    motion.velocity = direction * 100.f;  // Use desired invader speed.

                    if (glm::distance(motion.position, next_position) < 1.f) {
                        motion.position = next_position;
                        path.path.erase(path.path.begin());
                    }
                }
                else {
                    // If the path is empty, remove the invader.
                    registry.remove_all_components_of(entity);
                    registry.invaders.remove(entity);
                    continue;
                }
            }
           
        }

        // Update position.
        motion.position += motion.velocity * step_seconds;

        // If this entity is a projectile, remove it if it's off-screen.
        if (registry.projectiles.has(entity)) {
            if (motion.position.x < 0 || motion.position.x > WINDOW_WIDTH_PX ||
                motion.position.y < 0 || motion.position.y > WINDOW_HEIGHT_PX) {

                registry.remove_all_components_of(entity);
                continue;
            }
        }
    }
    ComponentContainer<Motion>& motion_container = registry.motions;
    for (uint i = 0; i < motion_container.components.size(); i++)
    {
        Motion& motion_i = motion_container.components[i];
        Entity entity_i = motion_container.entities[i];

        // note starting j at i+1 to compare all (i,j) pairs only once (and to not compare with itself)
        for (uint j = i + 1; j < motion_container.components.size(); j++)
        {
            Motion& motion_j = motion_container.components[j];
            Entity entity_j = motion_container.entities[j];
            if (collides(motion_i, motion_j))
            {
                Entity entity_j = motion_container.entities[j];
                // Create a collisions event
                // We are abusing the ECS system a bit in that we potentially insert muliple collisions for the same entity
                // CK: why the duplication, except to allow searching by entity_id
                registry.collisions.emplace_with_duplicates(entity_i, entity_j);
                registry.collisions.emplace_with_duplicates(entity_j, entity_i);
            }
        }
    }
   

}

	
