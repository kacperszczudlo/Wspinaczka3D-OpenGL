#include "Trampoline.h"
#include <glm/gtc/matrix_transform.hpp>

// Konstruktor
Trampoline::Trampoline(glm::vec3 pos, float r, float h, float force, Model* modelRef, glm::vec3 scale, glm::vec3 offset) {
    position = pos;
    radius = r;
    height = h;
    bounceForce = force;
    model = modelRef;
    visualScale = scale;
    visualOffset = offset;
}

// Rysowanie
void Trampoline::Draw(Shader& shader) {
    shader.setInt("useTexture", 1);
    shader.setVec4("objectColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    glm::mat4 modelMatrix = glm::mat4(1.0f);

    // Przesuñ na pozycjê w œwiecie
    modelMatrix = glm::translate(modelMatrix, position);

    // 2. Zastosuj korektê przesuniêcia
    modelMatrix = glm::translate(modelMatrix, visualOffset);

    // 3. Zastosuj skalowanie
    modelMatrix = glm::scale(modelMatrix, visualScale);

    shader.setMat4("model", modelMatrix);
    model->Draw(shader);
}

// Sprawdzanie kolizji
bool Trampoline::checkCollision(glm::vec3 playerPos, float& playerY, float& velocityY, float playerHalfHeight) {
    float footLevel = playerPos.y - playerHalfHeight;

    // Tolerancja w pionie
    bool atLevel = (footLevel <= height + 0.5f) && (footLevel >= height - 0.5f);

    // Sprawdzamy tylko, jeœli gracz jest na odpowiedniej wysokoœci i spada 
    if (atLevel && velocityY <= 0.0f) {
        // Ignorujemy ró¿nicê wysokoœci przy liczeniu dystansu 
        glm::vec3 flatPlayer = glm::vec3(playerPos.x, 0.0f, playerPos.z);
        glm::vec3 flatTrampoline = glm::vec3(position.x, 0.0f, position.z);

        float distance = glm::distance(flatPlayer, flatTrampoline);

        // Jeœli jesteœmy w promieniu 'radius'
        if (distance < radius) {
            playerY = height + playerHalfHeight; // Wypchnij gracza na wierzch
            velocityY = bounceForce;             // Nadaj prêdkoœæ w górê
            return true;
        }
    }
    return false;
}