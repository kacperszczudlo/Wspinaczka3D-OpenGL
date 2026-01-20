#include "GlassBridge.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib> // rand

GlassBridge::GlassBridge(glm::vec3 startPos, float height, Model* modelRef) {
    yLevel = height;
    tileModel = modelRef;

    float stepX = 2.0f;      // Odleg³oœæ miêdzy rzêdami
    float spreadZ = 0.9f;    // Odsuniêcie od œrodka

    for (int i = 0; i < 8; ++i) {
        float xOffset = i * stepX;
        bool rightSideIsSafe;

        //rightSideIsSafe = (rand() % 2 == 0);

       
        if (i < 4) {
            rightSideIsSafe = true;
        }
        else {
            rightSideIsSafe = false;
        }

        // LEWA P£YTKA
        GlassTile leftTile = {};
        leftTile.position = startPos + glm::vec3(xOffset, 0.0f, -spreadZ);
        leftTile.isSafe = !rightSideIsSafe;
        leftTile.isBroken = false;
        updateHitbox(leftTile);
        tiles.push_back(leftTile);

        // PRAWA P£YTKA
        GlassTile rightTile = {};
        rightTile.position = startPos + glm::vec3(xOffset, 0.0f, spreadZ);
        rightTile.isSafe = rightSideIsSafe;
        rightTile.isBroken = false;
        updateHitbox(rightTile);
        tiles.push_back(rightTile);
    }
}

void GlassBridge::updateHitbox(GlassTile& tile) {
    tile.minX = tile.position.x - 0.5f;
    tile.maxX = tile.position.x + 0.5f;
    tile.minZ = tile.position.z - 0.5f;
    tile.maxZ = tile.position.z + 0.5f;
}

void GlassBridge::Draw(Shader& shader) {
    shader.setInt("useTexture", 1);
    // Pó³przezroczystoœæ dla szk³a
    shader.setVec4("objectColor", glm::vec4(1.0f, 1.0f, 1.0f, 0.5f));

    for (const auto& tile : tiles) {
        if (!tile.isBroken) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(tile.position.x, yLevel, tile.position.z));
            shader.setMat4("model", model);
            tileModel->Draw(shader);
        }
    }

    // Reset koloru
    shader.setVec4("objectColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

bool GlassBridge::checkCollision(glm::vec3 playerPos, float& playerY, float& velocityY, float playerHalfHeight) {
    float footLevel = playerPos.y - playerHalfHeight;

    bool atBridgeLevel = (playerPos.y >= yLevel + playerHalfHeight - 0.2f) &&
        (footLevel <= yLevel + 0.2f) &&
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
                return false; // Spadamy
            }
        }
    }
    return false;
}

void GlassBridge::Reset() {
    for (auto& tile : tiles) tile.isBroken = false;
}