#ifndef PLAYER_H
#define PLAYER_H

#include <vector>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"

class Player {
private:
    // Geometria Jajka
    unsigned int eggVAO, eggVBO, eggEBO;
    std::vector<unsigned int> eggIndices;

    // Geometria P�kni��
    unsigned int crackVAO, crackVBO;
    std::vector<float> crackVertices;

    // Geometria Od�amk�w (Sze�cian)
    unsigned int cubeVAO, cubeVBO;
    glm::vec3 fragments[5];

    // Generator losowy
    float localRandom(std::mt19937& gen, float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen);
    }

public:
    Player() {
        // 1. Generowanie Jajka (Elipsoidy)
        std::vector<float> vertices;
        int segments = 40;
        for (int i = 0; i <= segments; ++i) {
            float theta = glm::pi<float>() * i / segments;
            for (int j = 0; j <= segments; ++j) {
                float phi = 2 * glm::pi<float>() * j / segments;
                vertices.push_back(0.5f * std::sin(theta) * std::cos(phi));
                vertices.push_back(0.7f * std::cos(theta));
                vertices.push_back(0.5f * std::sin(theta) * std::sin(phi));
            }
        }
        for (int i = 0; i < segments; ++i) {
            for (int j = 0; j < segments; ++j) {
                int first = (i * (segments + 1)) + j;
                int second = first + segments + 1;
                eggIndices.push_back(first); eggIndices.push_back(second); eggIndices.push_back(first + 1);
                eggIndices.push_back(second); eggIndices.push_back(second + 1); eggIndices.push_back(first + 1);
            }
        }
        glGenVertexArrays(1, &eggVAO); glGenBuffers(1, &eggVBO); glGenBuffers(1, &eggEBO);
        glBindVertexArray(eggVAO);
        glBindBuffer(GL_ARRAY_BUFFER, eggVBO); glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eggEBO); glBufferData(GL_ELEMENT_ARRAY_BUFFER, eggIndices.size() * sizeof(unsigned int), eggIndices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
        //tymczasowe
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);

        // 2. Inicjalizacja P�kni��
        glGenVertexArrays(1, &crackVAO); glGenBuffers(1, &crackVBO);
        glBindVertexArray(crackVAO);
        glBindBuffer(GL_ARRAY_BUFFER, crackVBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
        //tymczasowe
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);

        // 3. Inicjalizacja Od�amk�w (Cube)
        float cubeData[] = { -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f };
        glGenVertexArrays(1, &cubeVAO); glGenBuffers(1, &cubeVBO);
        glBindVertexArray(cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO); glBufferData(GL_ARRAY_BUFFER, sizeof(cubeData), cubeData, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
        //tymczasowe
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);

        // Kierunki od�amk�w
        fragments[0] = glm::vec3(0.5f, 0.5f, 0.5f); fragments[1] = glm::vec3(-0.5f, 0.5f, 0.5f);
        fragments[2] = glm::vec3(0.0f, -0.5f, 0.0f); fragments[3] = glm::vec3(0.5f, 0.5f, -0.5f);
        fragments[4] = glm::vec3(-0.5f, 0.5f, -0.5f);
    }

    ~Player() {
        glDeleteVertexArrays(1, &eggVAO); glDeleteBuffers(1, &eggVBO); glDeleteBuffers(1, &eggEBO);
        glDeleteVertexArrays(1, &crackVAO); glDeleteBuffers(1, &crackVBO);
        glDeleteVertexArrays(1, &cubeVAO); glDeleteBuffers(1, &cubeVBO);
    }

    // Proceduralne generowanie p�kni��
    void UpdateCracks(int count) {
        crackVertices.clear();
        std::mt19937 generator;
        for (int i = 0; i < count; ++i) {
            unsigned int seed = 100 + i * 10; generator.seed(seed);
            float theta = localRandom(generator, 0.0f, glm::pi<float>());
            float phi = localRandom(generator, 0.0f, 2.0f * glm::pi<float>());
            glm::vec3 curr(0.5f * std::sin(theta) * std::cos(phi), 0.7f * std::cos(theta), 0.5f * std::sin(theta) * std::sin(phi));
            for (int j = 0; j < 5 + i * 3; ++j) {
                crackVertices.push_back(curr.x); crackVertices.push_back(curr.y); crackVertices.push_back(curr.z);
                theta = glm::clamp(theta + localRandom(generator, -0.3f, 0.3f), 0.0f, glm::pi<float>());
                phi += localRandom(generator, -0.3f, 0.3f);
                glm::vec3 next(0.5f * std::sin(theta) * std::cos(phi), 0.7f * std::cos(theta), 0.5f * std::sin(theta) * std::sin(phi));
                crackVertices.push_back(next.x); crackVertices.push_back(next.y); crackVertices.push_back(next.z);
                curr = next;
            }
        }
        glBindBuffer(GL_ARRAY_BUFFER, crackVBO);
        glBufferData(GL_ARRAY_BUFFER, crackVertices.size() * sizeof(float), crackVertices.empty() ? nullptr : crackVertices.data(), GL_DYNAMIC_DRAW);
    }

    void Draw(Shader& shader, glm::vec3 pos, bool crashed, float timeElapsed, float animationDuration) {
        if (!crashed) {
            // Rysuj ca�e jajko
            shader.setInt("useTexture", 0);
            shader.setMat4("model", glm::translate(glm::mat4(1.0f), pos));
            shader.setVec4("objectColor", glm::vec4(1.0f, 0.9f, 0.7f, 1.0f));
            glBindVertexArray(eggVAO);
            glDrawElements(GL_TRIANGLES, (GLsizei)eggIndices.size(), GL_UNSIGNED_INT, 0);

            // Rysuj p�kni�cia
            if (!crackVertices.empty()) {
                shader.setVec4("objectColor", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
                glBindVertexArray(crackVAO);
                glLineWidth(3.0f);
                glDrawArrays(GL_LINES, 0, (GLsizei)crackVertices.size() / 3);
                glLineWidth(1.0f);
            }
        }
        else {
            // Rysuj wybuch (fragmenty)
            float ratio = timeElapsed / animationDuration;
            if (ratio < 1.0f) {
                glBindVertexArray(cubeVAO);
                shader.setInt("useTexture", 0);
                shader.setVec4("objectColor", glm::vec4(1.0f, 0.9f, 0.7f, 1.0f));
                for (int i = 0; i < 5; ++i) {
                    glm::mat4 m = glm::translate(glm::mat4(1.0f), pos + fragments[i] * 3.0f * (1.0f - ratio) * timeElapsed);
                    m = glm::translate(m, glm::vec3(0.0f, -5.0f * pow(timeElapsed, 2), 0.0f)); // grawitacja
                    shader.setMat4("model", glm::scale(m, glm::vec3(glm::mix(0.1f, 0.0f, ratio))));
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
        }
        glBindVertexArray(0);
    }
};

#endif