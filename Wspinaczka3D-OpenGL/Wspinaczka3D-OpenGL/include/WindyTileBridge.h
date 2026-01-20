#ifndef WINDY_TILE_BRIDGE_H
#define WINDY_TILE_BRIDGE_H

#include <vector>
#include <cstdlib>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Model.h"
#include "Shader.h"
#include "Physics.h"

struct WindyTile {
    glm::vec3 position;
    bool active;
    float timer;
    float maxTime;
    TableHitbox hitbox;
    bool isEndTile; // kafelek prowadz¹cy do mety
};

class WindyTileBridge {
public:
    std::vector<WindyTile> tiles;
    Model* tileModel;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    int gridSizeX;
    int gridSizeZ;
    float tileSize;

    glm::vec2 windVelocity;
    float windChangeTimer;
    float windChangeInterval;
    float maxWindStrength;

    TableHitbox bridgeBounds;
    bool isPlayerOnBridge;

    static constexpr float STAND_TIME_THRESHOLD = 3.0f;

    WindyTileBridge(glm::vec3 pos, glm::vec3 rot, glm::vec3 sc, Model* modelRef,
        int gridX = 12, int gridZ = 25, float tileSz = 2.0f) {
        position = pos;
        rotation = rot;
        scale = sc;
        tileModel = modelRef;
        gridSizeX = gridX;
        gridSizeZ = gridZ;
        tileSize = tileSz;

        windVelocity = glm::vec2(0.0f, 0.0f);
        windChangeTimer = 0.0f;
        windChangeInterval = 3.5f;
        maxWindStrength = 2.8f;
        isPlayerOnBridge = false;

        float bridgeWidth = gridSizeX * tileSize;
        float bridgeLength = gridSizeZ * tileSize;
        float startX = pos.x - bridgeWidth / 2.0f;
        float startZ = pos.z;

        std::srand(static_cast<unsigned int>(std::time(nullptr)));

        for (int x = 0; x < gridSizeX; x++) {
            for (int z = 0; z < gridSizeZ; z++) {
                WindyTile tile;
                tile.position = glm::vec3(
                    startX + x * tileSize + tileSize * 0.5f,
                    pos.y,
                    startZ + z * tileSize + tileSize * 0.5f
                );
                tile.active = true;
                tile.timer = 0.0f;
                tile.maxTime = STAND_TIME_THRESHOLD;

                // Oznacz ostatnie 3 rzêdy jako kafelki mety
                tile.isEndTile = (z >= gridSizeZ - 3);

                tile.hitbox.minX = tile.position.x - tileSize * 0.4f;
                tile.hitbox.maxX = tile.position.x + tileSize * 0.4f;
                tile.hitbox.minZ = tile.position.z - tileSize * 0.4f;
                tile.hitbox.maxZ = tile.position.z + tileSize * 0.4f;
                tile.hitbox.topY = tile.position.y + 0.5f;

                tiles.push_back(tile);
            }
        }

        bridgeBounds.minX = startX;
        bridgeBounds.maxX = startX + bridgeWidth;
        bridgeBounds.minZ = startZ;
        bridgeBounds.maxZ = startZ + bridgeLength;
        bridgeBounds.topY = pos.y + 0.5f;
    }

    void Update(float deltaTime, const glm::vec3& playerPos) {
        windChangeTimer += deltaTime;
        if (windChangeTimer >= windChangeInterval) {
            float angle = static_cast<float>(rand()) / RAND_MAX * 6.2831853f;
            float strength = 1.2f + static_cast<float>(rand()) / RAND_MAX * maxWindStrength;
            windVelocity = glm::vec2(cos(angle), sin(angle)) * strength;
            windChangeTimer = 0.0f;
        }

        isPlayerOnBridge = (playerPos.x >= bridgeBounds.minX && playerPos.x <= bridgeBounds.maxX &&
            playerPos.z >= bridgeBounds.minZ && playerPos.z <= bridgeBounds.maxZ &&
            playerPos.y >= bridgeBounds.topY - 3.0f);

        if (isPlayerOnBridge) {
            for (auto& tile : tiles) {
                if (!tile.active) continue;

                bool onTile = (playerPos.x >= tile.hitbox.minX && playerPos.x <= tile.hitbox.maxX &&
                    playerPos.z >= tile.hitbox.minZ && playerPos.z <= tile.hitbox.maxZ &&
                    fabs(playerPos.y - (tile.hitbox.topY + 0.7f)) < 0.6f);

                if (onTile) {
                    tile.timer += deltaTime;
                    if (tile.timer >= tile.maxTime) {
                        tile.active = false;
                    }
                }
                else {
                    tile.timer = glm::max(0.0f, tile.timer - deltaTime * 0.5f);
                }
            }
        }
    }

    bool CheckCollision(glm::vec3& playerPos, float deltaTime) {
        bool standing = false;
        float targetY = 0.0f;

        for (auto& tile : tiles) {
            if (!tile.active) continue;

            if (playerPos.x >= tile.hitbox.minX && playerPos.x <= tile.hitbox.maxX &&
                playerPos.z >= tile.hitbox.minZ && playerPos.z <= tile.hitbox.maxZ) {

                targetY = tile.hitbox.topY + 0.7f;
                if (playerPos.y <= targetY + 0.5f && playerPos.y >= targetY - 0.5f) {
                    playerPos.y = targetY;
                    standing = true;
                    break;
                }
            }
        }

        if (isPlayerOnBridge) {
            float windEffect = standing ? 0.4f : 1.0f;
            playerPos.x += windVelocity.x * deltaTime * windEffect;
            playerPos.z += windVelocity.y * deltaTime * windEffect;
        }

        return standing;
    }

    void Draw(Shader& shader) {
        if (!tileModel) return;

        shader.setInt("useTexture", 1);

        for (const auto& tile : tiles) {
            if (!tile.active) continue;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, tile.position);

            float visualSize = tileSize * 0.82f;
            model = glm::scale(model, glm::vec3(visualSize, 0.5f, visualSize));

            // Kolorowanie kafelków mety
            glm::vec4 color;
            if (tile.isEndTile) {
                // Fioletowe kafelki mety (zielony + czerwony = fioletowy)
                color = glm::vec4(0.5f, 0.0f, 1.0f, 1.0f);
            }
            else {
                // Normalny gradient czasowy
                float timeRatio = glm::clamp(tile.timer / tile.maxTime, 0.0f, 1.0f);
                if (timeRatio < 0.4f) {
                    color = glm::mix(glm::vec4(0.0f, 1.0f, 0.2f, 1.0f),
                        glm::vec4(1.0f, 1.0f, 0.2f, 1.0f),
                        timeRatio * 2.5f);
                }
                else {
                    color = glm::mix(glm::vec4(1.0f, 1.0f, 0.2f, 1.0f),
                        glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
                        (timeRatio - 0.4f) * 1.666f);
                }
            }

            // Miganie gdy blisko znikniêcia
            if (tile.timer > tile.maxTime * 0.8f) {
                float flash = sin(tile.timer * 15.0f) * 0.3f + 0.7f;
                color *= flash;
            }

            shader.setVec4("objectColor", color);
            shader.setMat4("model", model);
            tileModel->Draw(shader);
        }

        shader.setVec4("objectColor", glm::vec4(1.0f));
    }

    //Funkcja zwracaj¹ca aktualny wiatr
    glm::vec2 GetWindForce() const {
        return windVelocity;
    }

    //Lista pozycji do generowania cz¹steczek (ca³a platforma)
    std::vector<glm::vec3> GetParticleSpawnPoints() const {
        std::vector<glm::vec3> points;
        for (const auto& tile : tiles) {
            if (tile.active && !tile.isEndTile) { // Nie generuj na kafelkach mety
                points.push_back(tile.position);
            }
        }
        return points;
    }

    bool IsPlayerFalling(const glm::vec3& playerPos) {
        return (isPlayerOnBridge && playerPos.y < bridgeBounds.topY - 8.0f);
    }
};

#endif