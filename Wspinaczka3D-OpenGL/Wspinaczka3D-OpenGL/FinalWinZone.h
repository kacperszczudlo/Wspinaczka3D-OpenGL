#ifndef FINAL_WIN_ZONE_H
#define FINAL_WIN_ZONE_H

#include <glm/glm.hpp>
#include "Model.h"
#include "Shader.h"
#include "UIManager.h"

class FinalWinZone {
public:
    glm::vec3 position;
    float width;
    float height;
    float topY;

    // Hitbox
    TableHitbox hitbox;

    // Flag czy gracz ju¿ wygra³
    bool playerHasWon;

    FinalWinZone(glm::vec3 pos, float w = 8.0f, float h = 8.0f, float surfaceY = 24.3f) {
        position = pos;
        width = w;
        height = h;
        topY = surfaceY;
        playerHasWon = false;

        // Ustaw hitbox
        hitbox.minX = pos.x - w / 2.0f;
        hitbox.maxX = pos.x + w / 2.0f;
        hitbox.minZ = pos.z - h / 2.0f;
        hitbox.maxZ = pos.z + h / 2.0f;
        hitbox.topY = topY;
    }

    // SprawdŸ kolizjê z graczem
    bool CheckPlayerInZone(const glm::vec3& playerPos, float playerHeight = 0.7f) {
        if (playerHasWon) return false; // Tylko raz

        bool inXZ = (playerPos.x >= hitbox.minX && playerPos.x <= hitbox.maxX &&
            playerPos.z >= hitbox.minZ && playerPos.z <= hitbox.maxZ);
        bool atCorrectHeight = (playerPos.y >= topY + playerHeight - 0.5f &&
            playerPos.y <= topY + playerHeight + 0.5f);

        if (inXZ && atCorrectHeight) {
            playerHasWon = true;
            return true;
        }
        return false;
    }

    bool IsPositionInsideZone(const glm::vec3& pos) const {
        return pos.x >= hitbox.minX && pos.x <= hitbox.maxX &&
            pos.z >= hitbox.minZ && pos.z <= hitbox.maxZ &&
            pos.y >= topY - 2.0f; // Tolerancja 2 jednostki w dó³
    }

    // Rysuj strefê (zielona platforma)
    void Draw(Shader& shader, Model& platformModel) {
        shader.setInt("useTexture", 1);
        shader.setVec4("objectColor", glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)); // Zielona

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position.x, topY - 0.2f, position.z));
        model = glm::scale(model, glm::vec3(width, 0.4f, height));

        shader.setMat4("model", model);
        platformModel.Draw(shader);

        // Reset koloru
        shader.setVec4("objectColor", glm::vec4(1.0f));
    }

    // Wyœwietl komunikat (wywo³aj w UI)
    void DisplayWinMessage(UIManager* ui) {
        // U¿yj istniej¹cego UIManager do pokazania komunikatu
        // UIManager musi mieæ metodê DrawCustomMessage(std::string)
        // Jeœli nie ma, trzeba dodaæ - albo rysujemy bezpoœrednio tutaj
    }
};

#endif#pragma once
