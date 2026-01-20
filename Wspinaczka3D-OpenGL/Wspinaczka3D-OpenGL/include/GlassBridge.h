#ifndef GLASS_BRIDGE_H
#define GLASS_BRIDGE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cstdlib> // Potrzebne do rand()
#include "Shader.h"
#include "Model.h"

struct GlassTile {
    glm::vec3 position;
    bool isSafe;
    bool isBroken;
    float minX, maxX;
    float minZ, maxZ;

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

        // USTAWIENIA ODLEG£OŒCI
        float stepX = 2.0f;      // Odleg³oœæ miêdzy rzêdami (skok w dal)
        // P³ytka ma 1.0m, wiêc 2.0f daje 1.0m dziury.

        float spreadZ = 0.9f;    // Odsuniêcie od œrodka (szerokoœæ mostu)
        // Prawa bêdzie na +0.9, Lewa na -0.9.

// Generujemy 8 par wzd³u¿ osi X
        for (int i = 0; i < 8; ++i) {
            float xOffset = i * stepX;

            // --- LOGIKA "PECHOWYCH" P£YTEK ---
            bool rightSideIsSafe;

            // WARIANT 1: LOSOWY (¿eby by³o 50/50)
            
            /*rightSideIsSafe = (rand() % 2 == 0);*/
            

            // WARIANT 2: TESTOWY 
            // Pierwsze 4 rzêdy (0,1,2,3) -> Prawa bezpieczna
            // Kolejne 4 rzêdy (4,5,6,7) -> Lewa bezpieczna (czyli prawa niebezpieczna)
            if (i < 4) {
                rightSideIsSafe = true;
            }
            else {
                rightSideIsSafe = false;
            }
            // ---------------------------------

            // LEWA P£YTKA (Z minus)
            GlassTile leftTile = {};
            leftTile.position = startPos + glm::vec3(xOffset, 0.0f, -spreadZ);
            leftTile.isSafe = !rightSideIsSafe; // Jeœli prawa bezpieczna, to lewa pêka
            leftTile.isBroken = false;
            updateHitbox(leftTile);
            tiles.push_back(leftTile);

            // PRAWA P£YTKA (Z plus)
            GlassTile rightTile = {};
            rightTile.position = startPos + glm::vec3(xOffset, 0.0f, spreadZ);
            rightTile.isSafe = rightSideIsSafe;
            rightTile.isBroken = false;
            updateHitbox(rightTile);
            tiles.push_back(rightTile);
        }
    }

    void updateHitbox(GlassTile& tile) {
        // Hitbox dopasowany do rozmiaru p³ytki (zak³adamy 1x1 metr)
        tile.minX = tile.position.x - 0.5f;
        tile.maxX = tile.position.x + 0.5f;
        tile.minZ = tile.position.z - 0.5f;
        tile.maxZ = tile.position.z + 0.5f;
    }

    void Draw(Shader& shader) {
        // W³¹czamy tekstury
        shader.setInt("useTexture", 1);

        // Ustawiamy kolor na Bia³y z Alpha = 0.5 (50% widocznoœci dla efektu szk³a)
        shader.setVec4("objectColor", glm::vec4(1.0f, 1.0f, 1.0f, 0.5f));

        for (const auto& tile : tiles) {
            if (!tile.isBroken) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(tile.position.x, yLevel, tile.position.z));
                shader.setMat4("model", model);
                tileModel->Draw(shader);
            }
        }

        //  pe³n¹ widocznoœæ dla reszty obiektów
        shader.setVec4("objectColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    bool checkCollision(glm::vec3 playerPos, float& playerY, float& velocityY, float playerHalfHeight) {
        float footLevel = playerPos.y - playerHalfHeight;

        // margines b³êdu w pionie (+0.2f), ¿eby ³atwiej by³o "z³apaæ" krawêdŸ przy skoku
        bool atBridgeLevel = (playerPos.y >= yLevel + playerHalfHeight - 0.2f) &&
            (footLevel <= yLevel + 0.2f) &&
            (velocityY <= 0.0f);

        if (!atBridgeLevel) return false;

        for (auto& tile : tiles) {
            if (tile.isBroken) continue; // Ignoruj pêkniête

            // Sprawdzanie czy jesteœmy w obrysie p³ytki
            if (playerPos.x > tile.minX && playerPos.x < tile.maxX &&
                playerPos.z > tile.minZ && playerPos.z < tile.maxZ) {

                if (tile.isSafe) {
                    playerY = yLevel + playerHalfHeight;
                    velocityY = 0.0f;
                    return true;
                }
                else {
                    tile.isBroken = true; // Pêkniêcie!
                    return false; // Spadamy
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