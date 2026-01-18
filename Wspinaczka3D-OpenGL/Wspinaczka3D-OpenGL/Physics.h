#ifndef PHYSICS_H
#define PHYSICS_H

#include <glm/glm.hpp>
#include <cmath>
#include "WinZone.h" 

class Physics {
public:
    // Parametry fizyczne
    float velocityY = 0.0f;
    bool canJump = true;
    bool isClimbing = false; // <--- FLAGA WSPINANIA

    const float GRAVITY = -9.8f;
    const float JUMP_FORCE = 4.0f;
    const float EGG_HALF_HEIGHT = 0.7f;

    Physics() {}

    // Obsługa grawitacji
    void ApplyGravity(float deltaTime, float& playerY) {
        // Jeśli się wspinamy, grawitacja nie działa (zatrzymujemy spadanie)
        if (isClimbing) {
            velocityY = 0.0f;
            return;
        }

        velocityY += GRAVITY * deltaTime;
        playerY += velocityY * deltaTime;
    }

    void TryJump() {
        if (canJump || isClimbing) {
            velocityY = JUMP_FORCE;
            canJump = false;
            isClimbing = false;
        }
    }

    void Reset() {
        velocityY = 0.0f;
        canJump = true;
        isClimbing = false;
    }
    // Ogłusga wiatru na moście
    glm::vec3 windForce = glm::vec3(0.0f); // NOWA: Siła wiatru
    bool isOnFlyover = false;

    // --- Funkcje statyczne do kolizji ---
    static bool IsInsideXZ(const glm::vec3& pos, const TableHitbox& t) {
        return pos.x > t.minX && pos.x < t.maxX &&
            pos.z > t.minZ && pos.z < t.maxZ;
    }

    bool CheckHorizontalCollision(glm::vec3& currentPos, const glm::vec3& oldPos, const TableHitbox& t) {
        float restingCenterY = t.topY + EGG_HALF_HEIGHT;
        // Jeśli jesteśmy nad obiektem, to nie jest to ściana
        if (currentPos.y > restingCenterY - 0.05f) return false;

        // Jeśli weszliśmy w obrys -> cofamy ruch
        if (IsInsideXZ(currentPos, t)) {
            currentPos.x = oldPos.x;
            currentPos.z = oldPos.z;
            return true;
        }
        return false;
    }
};

#endif