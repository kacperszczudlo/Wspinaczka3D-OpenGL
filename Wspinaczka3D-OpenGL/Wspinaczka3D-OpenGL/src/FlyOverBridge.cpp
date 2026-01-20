#include "FlyoverBridge.h"
#include <glm/gtc/matrix_transform.hpp>

FlyoverBridge::FlyoverBridge(glm::vec3 pos, glm::vec3 rot, glm::vec3 sc, Model* modelRef) {
    position = pos;
    rotation = rot;
    scale = sc;
    model = modelRef;
}

void FlyoverBridge::Draw(Shader& shader) {
    shader.setInt("useTexture", 1);
    shader.setVec4("objectColor", glm::vec4(1.0f));

    glm::mat4 modelMatrix = glm::mat4(1.0f);

    // 1. Pozycja
    modelMatrix = glm::translate(modelMatrix, position);

    // 2. Obrót
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    // 3. Skala
    modelMatrix = glm::scale(modelMatrix, scale);

    shader.setMat4("model", modelMatrix);
    model->Draw(shader);
}