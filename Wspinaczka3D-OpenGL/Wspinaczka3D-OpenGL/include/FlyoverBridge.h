#ifndef FLYOVER_BRIDGE_H
#define FLYOVER_BRIDGE_H

#include <glm/glm.hpp>
#include "Model.h"
#include "Shader.h"

class FlyoverBridge {
public:
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    Model* model;

    // Konstruktor
    FlyoverBridge(glm::vec3 pos, glm::vec3 rot, glm::vec3 sc, Model* modelRef);

    // Metoda
    void Draw(Shader& shader);
};

#endif