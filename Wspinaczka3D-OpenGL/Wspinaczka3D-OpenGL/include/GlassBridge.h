#ifndef GLASS_BRIDGE_H
#define GLASS_BRIDGE_H

#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"
#include "Model.h"

struct GlassTile {
    glm::vec3 position;
    bool isSafe;
    bool isBroken;
    float minX, maxX;
    float minZ, maxZ;

    // Konstruktor domyœlny struktury
    GlassTile() : position(0.0f), isSafe(false), isBroken(false), minX(0), maxX(0), minZ(0), maxZ(0) {}
};

class GlassBridge {
public:
    std::vector<GlassTile> tiles;
    float yLevel;
    Model* tileModel;

    // Konstruktor
    GlassBridge(glm::vec3 startPos, float height, Model* modelRef);

    // Metody
    void Draw(Shader& shader);
    bool checkCollision(glm::vec3 playerPos, float& playerY, float& velocityY, float playerHalfHeight);
    void Reset();

private:
    void updateHitbox(GlassTile& tile);
};

#endif