#include <iostream>
#include "ai_system.hpp"
#include "world_init.hpp"

void AISystem::step(float elapsed_ms)
{
	// (void)elapsed_ms; // placeholder to silence unused warning until implemented

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// !!! TODO A1: scan for invaders and shoot at them
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// invader detection system for towers
	// - for each tower, scan its row:
	//   - if an invader is detected and the tower's shooting timer has expired,
	//     then shoot (create a projectile) and reset the tower's shot timer
	for (const Entity& tower_entity : registry.towers.entities) {
        Motion& tower_motion = registry.motions.get(tower_entity);
        Tower& tower = registry.towers.get(tower_entity);

        tower.timer_ms -= elapsed_ms;

        if (!registry.invaders.entities.empty()) {
            Entity closest_invader = registry.invaders.entities[0];
            float minDistance = glm::distance(tower_motion.position,
                registry.motions.get(closest_invader).position);
            for (size_t i = 1; i < registry.invaders.entities.size(); i++) {
                Entity candidate = registry.invaders.entities[i];
                float candidateDistance = glm::distance(tower_motion.position, registry.motions.get(candidate).position);
                if (candidateDistance < minDistance) {
                    minDistance = candidateDistance;
                    closest_invader = candidate;
                }
            }

            Motion& invader_motion = registry.motions.get(closest_invader);

            float desiredAngle = -glm::degrees(atan2(invader_motion.position.y - tower_motion.position.y, invader_motion.position.x - tower_motion.position.x));
            float currentAngle = tower_motion.angle;
            float deltaAngle = desiredAngle - currentAngle;
            while (deltaAngle > 180.f) {
                deltaAngle -= 360.f;
            }
            while (deltaAngle < -180.f) {
                deltaAngle += 360.f;
            }

            
            float turnSpeed = 175.f;  
            float maxTurn = turnSpeed * (elapsed_ms / 1000.f);

            if (fabs(deltaAngle) < maxTurn) {
                tower_motion.angle = desiredAngle;
            }
            else {
                tower_motion.angle += (deltaAngle > 0 ? maxTurn : -maxTurn);
            }
        }

       
        const float rowTolerance = 5.0f; 

        if (tower.timer_ms <= 0) {
            float range_pixels = tower.range;
            for (const Entity& invader_entity : registry.invaders.entities) {
                Motion& invader_motion = registry.motions.get(invader_entity);
                float distance = glm::distance(tower_motion.position, invader_motion.position);
                if (distance <= range_pixels) {
                    vec2 projectile_position = tower_motion.position;
                    float speed = 1000.f;
                    float angleRad = glm::radians(-tower_motion.angle);
                    vec2 projectile_velocity = { cos(angleRad) * speed, sin(angleRad) * speed };
                    createProjectile(projectile_position, vec2(20.f, 20.f), projectile_velocity);
                    tower.timer_ms = TOWER_TIMER_MS; 
                    break;
                }
            }
        }
	}
}