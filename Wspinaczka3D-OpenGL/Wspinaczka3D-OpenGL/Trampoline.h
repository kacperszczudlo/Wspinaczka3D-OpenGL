#ifndef TRAMPOLINE_H
#define TRAMPOLINE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Shader.h"
#include "Model.h"

class Trampoline {
public:
    // Fizyka (Hitbox)
    glm::vec3 position; // Gdzie stoi trampolina (logiczny œrodek)
    float radius;       // Promieñ obszaru, który wybija (hitbox)
    float height;       // Wysokoœæ, na której nastêpuje wybicie
    float bounceForce;  // Si³a wybicia

    // Wygl¹d
    Model* model;
    glm::vec3 visualScale;  // Jak bardzo zmniejszyæ/zwiêkszyæ model
    glm::vec3 visualOffset; // Przesuniêcie modelu wzglêdem logicznego œrodka (np. jeœli model jest krzywo wyœrodkowany)

    Trampoline(glm::vec3 pos, float r, float h, float force, Model* modelRef, glm::vec3 scale, glm::vec3 offset) {
        position = pos;
        radius = r;
        height = h;
        bounceForce = force;
        model = modelRef;
        visualScale = scale;
        visualOffset = offset;
    }

    void Draw(Shader& shader) {
        shader.setInt("useTexture", 1);
        shader.setVec4("objectColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        glm::mat4 modelMatrix = glm::mat4(1.0f);

        // 1. Przesuñ na pozycjê w œwiecie (tam gdzie jest hitbox)
        modelMatrix = glm::translate(modelMatrix, position);

        // 2. Zastosuj korektê przesuniêcia (jeœli model nie jest wyœrodkowany)
        modelMatrix = glm::translate(modelMatrix, visualOffset);

        // 3. Zastosuj skalowanie (zmniejsz model)
        modelMatrix = glm::scale(modelMatrix, visualScale);

        shader.setMat4("model", modelMatrix);
        model->Draw(shader);
    }

    bool checkCollision(glm::vec3 playerPos, float& playerY, float& velocityY, float playerHalfHeight) {
        // Hitbox dzia³a w oparciu o zmienn¹ 'position', a nie wygl¹d modelu
        float footLevel = playerPos.y - playerHalfHeight;

        // Tolerancja w pionie
        bool atLevel = (footLevel <= height + 0.5f) && (footLevel >= height - 0.5f);

        if (atLevel && velocityY <= 0.0f) {
            glm::vec3 flatPlayer = glm::vec3(playerPos.x, 0.0f, playerPos.z);
            glm::vec3 flatTrampoline = glm::vec3(position.x, 0.0f, position.z);

            float distance = glm::distance(flatPlayer, flatTrampoline);

            // Jeœli jesteœmy w promieniu 'radius'
            if (distance < radius) {
                playerY = height + playerHalfHeight;
                velocityY = bounceForce;
                return true;
            }
        }
        return false;
    }
};

#endif