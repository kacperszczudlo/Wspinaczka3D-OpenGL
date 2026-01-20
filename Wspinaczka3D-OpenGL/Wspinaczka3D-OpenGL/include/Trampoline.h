#ifndef TRAMPOLINE_H
#define TRAMPOLINE_H

#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"
#include "Model.h"

class Trampoline {
public:
    // Fizyka (Hitbox)
    glm::vec3 position; // Gdzie stoi trampolina 
    float radius;       // Promieñ obszaru, który wybija 
    float height;       // Wysokoœæ, na której nastêpuje wybicie
    float bounceForce;  // Si³a wybicia

    // Wygl¹d
    Model* model;
    glm::vec3 visualScale;  // zmniejszanie/zwiêkszanie modelu
    glm::vec3 visualOffset; // Przesuniêcie modelu 

    // Konstruktor
    Trampoline(glm::vec3 pos, float r, float h, float force, Model* modelRef, glm::vec3 scale, glm::vec3 offset);

    // Metody
    void Draw(Shader& shader);
    bool checkCollision(glm::vec3 playerPos, float& playerY, float& velocityY, float playerHalfHeight);
};

#endif