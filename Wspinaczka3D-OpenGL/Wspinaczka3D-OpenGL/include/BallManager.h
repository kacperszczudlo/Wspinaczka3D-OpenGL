#ifndef BALL_MANAGER_H
#define BALL_MANAGER_H

#include <vector>
#include <glm/glm.hpp>
#include "Model.h"
#include "Shader.h"

struct RollingBall {
    glm::vec3 position;
    glm::vec3 velocity;
    float rotationAngle;
    bool active;
    bool isFalling;
};

class BallManager {
public:
    std::vector<RollingBall> balls;
    Model* ballModel;

    // Ustawienia
    float spawnTimer;
    float spawnInterval;
    float ballRadius;

    // Sta³e poziomu
    float floorLevel;
    float startX;
    float bridgeEdgeX;
    float deleteX;

    // Zakres szerokoœci mostu (Z)
    float minZ;
    float maxZ;

    // Konstruktor
    BallManager(Model* modelRef);

    // Metody g³ówne
    void Update(float deltaTime);
    void Draw(Shader& shader);
    bool CheckCollision(glm::vec3 playerPos);

private:
    // Metody pomocnicze
    float randomFloat(float min, float max);
    void SpawnBall();
    void ResetBall(RollingBall& b);
};

#endif