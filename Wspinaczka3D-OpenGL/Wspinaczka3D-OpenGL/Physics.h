#ifndef PHYSICS_H
#define PHYSICS_H

#include <glm/glm.hpp>
#include <cmath>
#include "WinZone.h" // Potrzebujemy definicji TableHitbox

class Physics {
public:
    // Parametry fizyczne
    float velocityY = 0.0f;
    bool canJump = true;
    const float GRAVITY = -9.8f;
    const float JUMP_FORCE = 4.0f;
    const float EGG_HALF_HEIGHT = 0.7f;

    Physics() {}

    // Obs�uga grawitacji
    void ApplyGravity(float deltaTime, float& playerY) {
        velocityY += GRAVITY * deltaTime;
        playerY += velocityY * deltaTime;
    }

    // Obs�uga skoku
    void TryJump() {
        if (canJump) {
            velocityY = JUMP_FORCE;
            canJump = false;
        }
    }

    // Resetowanie stanu (np. po wpadni�ciu do wody/�mierci)
    void Reset() {
        velocityY = 0.0f;
        canJump = true;
    }

    // --- Funkcje statyczne do kolizji (przeniesione z main) ---

    // Czy punkt jest w prostok�cie XZ?
    static bool IsInsideXZ(const glm::vec3& pos, const TableHitbox& t) {
        return pos.x > t.minX && pos.x < t.maxX &&
            pos.z > t.minZ && pos.z < t.maxZ;
    }

    // Sprawdza i koryguje wchodzenie w �ciany
    // Zwraca true je�li by�a kolizja
    bool CheckHorizontalCollision(glm::vec3& currentPos, const glm::vec3& oldPos, const TableHitbox& t) {
        float restingCenterY = t.topY + EGG_HALF_HEIGHT;

        // Je�li jeste�my nad obiektem, to nie jest to �ciana
        // (zmniejszy�em margines z 0.10f do 0.05f dla precyzji)
        if (currentPos.y > restingCenterY - 0.05f) {
            return false;
        }

        // Je�li weszli�my w obrys -> cofamy ruch
        if (IsInsideXZ(currentPos, t)) {
            currentPos.x = oldPos.x;
            currentPos.z = oldPos.z;
            return true;
        }
        return false;
    }
};

#endif