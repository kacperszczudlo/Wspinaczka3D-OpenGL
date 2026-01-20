#include "BallManager.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

BallManager::BallManager(Model* modelRef) {
    ballModel = modelRef;
    spawnTimer = 0.0f;

    spawnInterval = 3.0f;
    ballRadius = 0.5f;
    floorLevel = 23.4f + ballRadius;
    startX = -28.0f;
    bridgeEdgeX = 28.0f;
    deleteX = 40.0f;
    minZ = 29.0f;
    maxZ = 31.0f;

    std::srand(static_cast<unsigned int>(std::time(0)));
}

void BallManager::Update(float deltaTime) {
    // Spawnowanie
    spawnTimer += deltaTime;
    if (spawnTimer >= spawnInterval) {
        SpawnBall();
        spawnTimer = 0.0f;
    }

    //Fizyka kul
    for (auto& ball : balls) {
        if (!ball.active) continue;

        // Ruch
        ball.position += ball.velocity * deltaTime;

        // Odbijanie od barierek
        if (!ball.isFalling) {
            float wallLeft = 28.5f + ballRadius;
            float wallRight = 31.6f - ballRadius;

            if (ball.position.z < wallLeft) {
                ball.position.z = wallLeft;
                ball.velocity.z *= -1.0f;
            }
            else if (ball.position.z > wallRight) {
                ball.position.z = wallRight;
                ball.velocity.z *= -1.0f;
            }
        }

        // Obrót (toczenie)
        if (!ball.isFalling) {
            float moveStep = ball.velocity.x * deltaTime;
            ball.rotationAngle -= (moveStep / ballRadius) * (180.0f / 3.14159f);
        }

        // Spadanie (Grawitacja)
        if (ball.position.x > bridgeEdgeX) {
            ball.isFalling = true;
            ball.velocity.y -= 15.0f * deltaTime;
        }
        else {
            ball.position.y = floorLevel;
            ball.velocity.y = 0.0f;
        }

        // Usuwanie
        if (ball.position.y < -10.0f || ball.position.x > deleteX) {
            ball.active = false;
        }
    }
}

void BallManager::Draw(Shader& shader) {
    shader.setInt("useTexture", 1);
    shader.setVec4("objectColor", glm::vec4(1.0f));

    for (const auto& ball : balls) {
        if (!ball.active) continue;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, ball.position);
        model = glm::rotate(model, glm::radians(ball.rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(ballRadius));

        shader.setMat4("model", model);
        ballModel->Draw(shader);
    }
}

bool BallManager::CheckCollision(glm::vec3 playerPos) {
    float playerRadius = 0.5f;

    for (auto& ball : balls) {
        if (!ball.active) continue;

        // Optymalizacja: pomijanie jeœli daleko w osi X
        if (std::abs(playerPos.x - ball.position.x) > 2.0f) continue;

        float dist = glm::distance(playerPos, ball.position);

        if (dist < (ballRadius + playerRadius - 0.1f)) {
            ball.active = false;
            return true;
        }
    }
    return false;
}

float BallManager::randomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

void BallManager::SpawnBall() {
    bool spawned = false;
    for (auto& ball : balls) {
        if (!ball.active) {
            ResetBall(ball);
            spawned = true;
            break;
        }
    }
    if (!spawned) {
        RollingBall b;
        ResetBall(b);
        balls.push_back(b);
    }
}

void BallManager::ResetBall(RollingBall& b) {
    float randZ = randomFloat(minZ, maxZ);
    float speed = randomFloat(4.0f, 7.0f);
    float driftZ = randomFloat(-1.2f, 1.2f);

    b.position = glm::vec3(startX, floorLevel, randZ);
    b.velocity = glm::vec3(speed, 0.0f, driftZ);
    b.rotationAngle = 0.0f;
    b.active = true;
    b.isFalling = false;
}