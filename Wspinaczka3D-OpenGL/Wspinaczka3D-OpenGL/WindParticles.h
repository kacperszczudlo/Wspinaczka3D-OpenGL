#pragma once
#pragma once
#ifndef WINDPARTICLES_H
#define WINDPARTICLES_H

#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"
#include "Mesh.h"

class WindParticles {
public:
    std::vector<glm::vec3> particles;
    Mesh* particleMesh = nullptr;
    float spawnTimer = 0.0f;

    // KONSTRUKTOR (inline)
    inline WindParticles() {
        std::vector<Vertex> vertices = {
            {{-0.1f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            {{ 0.1f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
            {{ 0.1f,  0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{-0.1f,  0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
        };
        std::vector<unsigned int> indices = { 0, 1, 2, 2, 3, 0 };
        particleMesh = new Mesh(vertices, indices, {});
    }

    // DESTRUKTOR (inline)
    inline ~WindParticles() {
        delete particleMesh;
    }

    // UPDATE (inline)
    inline void Update(float deltaTime, glm::vec3 playerPos, glm::vec3 windForce) {
        if (windForce == glm::vec3(0.0f)) {
            particles.clear();
            return;
        }

        spawnTimer += deltaTime;
        if (spawnTimer > 0.05f) { // Co 50ms
            spawnTimer = 0.0f;

            glm::vec3 spawnPos = playerPos - glm::normalize(windForce) * 3.0f;
            spawnPos.y += 1.0f + (rand() % 100 / 100.0f);

            particles.push_back(spawnPos);
        }

        for (auto& p : particles) {
            p += windForce * deltaTime * 0.6f;
        }

        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [&](glm::vec3& p) { return glm::distance(p, playerPos) > 20.0f; }),
            particles.end()
        );
    }

    // DRAW (inline)
    inline void Draw(Shader& shader) {
        if (particles.empty()) return;

        shader.setInt("useTexture", 0);
        shader.setVec4("objectColor", glm::vec4(1.0f, 1.0f, 1.0f, 0.7f));

        for (auto& p : particles) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), p);
            shader.setMat4("model", model);
            particleMesh->Draw(shader);
        }

        shader.setInt("useTexture", 1);
    }
};

#endif // WINDPARTICLES_H