#pragma once
#ifndef WINDPARTICLES_H
#define WINDPARTICLES_H

#include <glm/glm.hpp>
#include <vector>
#include <algorithm>
#include "Shader.h"
#include "Mesh.h"
#include "WindyTileBridge.h"

class WindParticles {
public:
    std::vector<glm::vec3> particles;
    std::vector<float> particleLifetimes;
    Mesh* particleMesh = nullptr;
    float spawnTimer = 0.0f;

    WindyTileBridge* bridge = nullptr;

    WindParticles() {
        // Mniejsze cz�steczki (0.05)
        std::vector<Vertex> vertices = {
            {{-0.05f, -0.05f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            {{ 0.05f, -0.05f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
            {{ 0.05f,  0.05f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{-0.05f,  0.05f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
        };
        std::vector<unsigned int> indices = { 0, 1, 2, 2, 3, 0 };
        particleMesh = new Mesh(vertices, indices, {});
    }

    ~WindParticles() {
        delete particleMesh;
    }

    void Update(float deltaTime, glm::vec3 windForce) {
        // Spawn nowych cz�steczek tylko gdy wiatr wieje
        if (glm::length(windForce) > 0.5f) { // Minimalna si�a wiatru
            spawnTimer += deltaTime;
            if (spawnTimer > 0.03f) { // Co 30ms
                spawnTimer = 0.0f;

                if (bridge && !bridge->tiles.empty()) {
                    auto spawnPoints = bridge->GetParticleSpawnPoints();
                    for (const auto& spawnPos : spawnPoints) {
                        if (particles.size() > 200) break; // Maksymalnie 200 cz�steczek

                        glm::vec3 pos = spawnPos;
                        pos.y += 1.5f; // Nad kafelkami
                        pos.x += (rand() % 100 / 100.0f - 0.5f) * bridge->tileSize * 0.8f;
                        pos.z += (rand() % 100 / 100.0f - 0.5f) * bridge->tileSize * 0.8f;

                        particles.push_back(pos);
                        particleLifetimes.push_back(0.0f);
                    }
                }
            }
        }

        // Updatuj istniej�ce cz�steczki
        for (size_t i = 0; i < particles.size(); ++i) {
            if (i < particleLifetimes.size()) {
                particles[i] += windForce * deltaTime * 0.8f;
                particleLifetimes[i] += deltaTime;
            }
        }

        // Usuwanie starych cz�steczek (�ycie 3 sekundy)
        float maxLifetime = 3.0f;
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [&](glm::vec3& p) {
                    size_t idx = &p - &particles[0];
                    if (idx >= particleLifetimes.size()) return true;
                    return particleLifetimes[idx] > maxLifetime;
                }),
            particles.end()
        );

        // Czy�� lifetimes osobno
        particleLifetimes.erase(
            std::remove_if(particleLifetimes.begin(), particleLifetimes.end(),
                [&](float& lt) { return lt > maxLifetime; }),
            particleLifetimes.end()
        );
    }

    void Draw(Shader& shader) {
        if (particles.empty()) return;

        // ZAPISZ STAN OPENGL
        GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);

        // ZAPISZ AKTUALNE UNIFORMY
        GLint prevUseTexture;
        glGetUniformiv(shader.ID, glGetUniformLocation(shader.ID, "useTexture"), &prevUseTexture);

        glm::vec4 prevObjectColor;
        glGetUniformfv(shader.ID, glGetUniformLocation(shader.ID, "objectColor"), &prevObjectColor[0]);

        // W��CZ BLENDING I WY��CZ CULLING DLA CZ�STECZEK
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);

        // USTAWIENIA DLA CZ�STECZEK
        shader.setInt("useTexture", 0);
        shader.setVec4("objectColor", glm::vec4(1.0f, 1.0f, 1.0f, 0.7f));

        // RYSUJ CZ�STECZKI
        for (size_t i = 0; i < particles.size(); ++i) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, particles[i]);

            // Rotacja w kierunku wiatru
            if (bridge) {
                glm::vec2 wind = bridge->GetWindForce();
                if (glm::length(wind) > 0.1f) {
                    float angle = atan2(wind.y, wind.x);
                    model = glm::rotate(model, angle, glm::vec3(0, 0, 1));
                }
            }

            shader.setMat4("model", model);
            particleMesh->Draw(shader);
        }

        // PRZYWR�� POPRZEDNI STAN OPENGL
        if (!blendEnabled) glDisable(GL_BLEND);
        if (cullEnabled) glEnable(GL_CULL_FACE);

        // PRZYWR�� POPRZEDNIE UNIFORMY
        shader.setInt("useTexture", prevUseTexture);
        shader.setVec4("objectColor", prevObjectColor);
    }
};

#endif // WINDPARTICLES_H