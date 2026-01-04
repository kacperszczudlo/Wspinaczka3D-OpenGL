#ifndef GLASS_BRIDGE_H
#define GLASS_BRIDGE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Shader.h"
#include "Model.h"

struct GlassTile {
    glm::vec3 position;
    bool isSafe;
    bool isBroken;
    float minX, maxX;
    float minZ, maxZ;

    // Konstruktor zeruj¹cy (zapobiega b³êdom pamiêci)
    GlassTile() : position(0.0f), isSafe(false), isBroken(false), minX(0), maxX(0), minZ(0), maxZ(0) {}
};

class GlassBridge {
public:
    std::vector<GlassTile> tiles;
    float yLevel;
    Model* tileModel;

    GlassBridge(glm::vec3 startPos, float height, Model* modelRef) {
        yLevel = height;
        tileModel = modelRef;

        // Generujemy 8 par wzd³u¿ osi X
        for (int i = 0; i < 8; ++i) {
            float xOffset = i * 1.1f;

            // Lewa p³ytka (Z minus) -> FA£SZYWA
            GlassTile leftTile = {};
            leftTile.position = startPos + glm::vec3(xOffset, 0.0f, -0.6f);
            leftTile.isSafe = false;
            leftTile.isBroken = false;
            updateHitbox(leftTile);
            tiles.push_back(leftTile);

            // Prawa p³ytka (Z plus) -> BEZPIECZNA
            GlassTile rightTile = {};
            rightTile.position = startPos + glm::vec3(xOffset, 0.0f, 0.6f);
            rightTile.isSafe = true;
            rightTile.isBroken = false;
            updateHitbox(rightTile);
            tiles.push_back(rightTile);
        }
    }

    void updateHitbox(GlassTile& tile) {
        tile.minX = tile.position.x - 0.5f;
        tile.maxX = tile.position.x + 0.5f;
        tile.minZ = tile.position.z - 0.5f;
        tile.maxZ = tile.position.z + 0.5f;
    }

    // Standardowa funkcja Draw - model sam ogarnia tekstury
    void Draw(Shader& shader) {
        // Mówimy shaderowi: "Teraz u¿ywamy tekstur z modelu"
        shader.setInt("useTexture", 1);

        // Resetujemy kolor na bia³y (¿eby tekstura mia³a swoje oryginalne barwy)
        shader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));

        for (const auto& tile : tiles) {
            if (!tile.isBroken) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(tile.position.x, yLevel, tile.position.z));
                shader.setMat4("model", model);

                // Tutaj Model.h zrobi ca³¹ magiê:
                // sam aktywuje GL_TEXTURE0 i podepnie glass-texture-3.jpg
                tileModel->Draw(shader);
            }
        }
    }

    bool checkCollision(glm::vec3 playerPos, float& playerY, float& velocityY, float playerHalfHeight) {
        float footLevel = playerPos.y - playerHalfHeight;
        bool atBridgeLevel = (playerPos.y >= yLevel + playerHalfHeight - 0.1f) &&
            (footLevel <= yLevel + 0.1f) &&
            (velocityY <= 0.0f);

        if (!atBridgeLevel) return false;

        for (auto& tile : tiles) {
            if (tile.isBroken) continue;

            if (playerPos.x > tile.minX && playerPos.x < tile.maxX &&
                playerPos.z > tile.minZ && playerPos.z < tile.maxZ) {

                if (tile.isSafe) {
                    playerY = yLevel + playerHalfHeight;
                    velocityY = 0.0f;
                    return true;
                }
                else {
                    tile.isBroken = true;
                    return false;
                }
            }
        }
        return false;
    }

    void Reset() {
        for (auto& tile : tiles) tile.isBroken = false;
    }
};

#endif