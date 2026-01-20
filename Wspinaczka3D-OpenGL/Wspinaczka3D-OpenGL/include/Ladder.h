#ifndef LADDER_H
#define LADDER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Model.h"
#include "Shader.h"

class Ladder {
public:
    glm::vec3 position;
    Model* model;
    float height;

    // Hitbox wspinania
    float hitboxWidth = 2.0f;
    float hitboxDepth = 1.5f;

    // Wysokoœæ wizualna jednego segmentu
    const float SEGMENT_HEIGHT = 4.8f;

    Ladder(glm::vec3 pos, float h, Model* m) : position(pos), height(h), model(m) {}

    void Draw(Shader& shader) {
        shader.setInt("useTexture", 0);
        shader.setVec4("objectColor", glm::vec4(0.45f, 0.25f, 0.05f, 1.0f));

        // Segment 1 (Dó³)
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, position);
        modelMat = glm::rotate(modelMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(1.5f));
        shader.setMat4("model", modelMat);
        model->Draw(shader);

        // Segment 2 (Góra)
        glm::mat4 modelMat2 = glm::mat4(1.0f);
        modelMat2 = glm::translate(modelMat2, position + glm::vec3(0.0f, SEGMENT_HEIGHT, 0.0f));
        modelMat2 = glm::rotate(modelMat2, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMat2 = glm::scale(modelMat2, glm::vec3(1.5f));
        shader.setMat4("model", modelMat2);
        model->Draw(shader);

        shader.setInt("useTexture", 1);
        shader.setVec4("objectColor", glm::vec4(1.0f));
    }

    bool CheckCollision(const glm::vec3& playerPos) {
        float minX = position.x - hitboxWidth / 2.0f;
        float maxX = position.x + hitboxWidth / 2.0f;
        float minZ = position.z - hitboxDepth / 2.0f;
        float maxZ = position.z + hitboxDepth / 2.0f;

        bool inXZ = (playerPos.x >= minX && playerPos.x <= maxX &&
            playerPos.z >= minZ && playerPos.z <= maxZ);

        // POPRAWKA: Koniec wspinania równo z wysokoœci¹ wizualn¹ (ok. 9.6m od podstawy)
        bool inY = (playerPos.y >= position.y - 0.5f && playerPos.y <= position.y + 9.8f);

        return inXZ && inY;
    }
};

#endif