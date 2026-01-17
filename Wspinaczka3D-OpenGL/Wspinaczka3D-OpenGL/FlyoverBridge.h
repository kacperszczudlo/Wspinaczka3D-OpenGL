#ifndef FLYOVER_BRIDGE_H
#define FLYOVER_BRIDGE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Model.h"
#include "Shader.h"

class FlyoverBridge {
public:
    glm::vec3 position;
    glm::vec3 rotation; // Obrót w stopniach (X, Y, Z)
    glm::vec3 scale;    // Skala obiektu
    Model* model;

    // Konstruktor
    FlyoverBridge(glm::vec3 pos, glm::vec3 rot, glm::vec3 sc, Model* modelRef) {
        position = pos;
        rotation = rot;
        scale = sc;
        model = modelRef;
    }

    void Draw(Shader& shader) {
        // Ustawiamy podstawowe parametry shadera
        shader.setInt("useTexture", 1); // Zak³adamy, ¿e .mtl za³aduje kolory/tekstury
        shader.setVec4("objectColor", glm::vec4(1.0f)); // Domyœlny bia³y (nie zmienia koloru tekstury)

        glm::mat4 modelMatrix = glm::mat4(1.0f);

        // 1. Pozycja
        modelMatrix = glm::translate(modelMatrix, position);

        // 2. Obrót (Najpierw Y, potem X i Z - zale¿nie od orientacji modelu)
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        // 3. Skala
        modelMatrix = glm::scale(modelMatrix, scale);

        shader.setMat4("model", modelMatrix);

        // Rysujemy model
        model->Draw(shader);
    }
};

#endif