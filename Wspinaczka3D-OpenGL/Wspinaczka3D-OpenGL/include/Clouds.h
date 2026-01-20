#ifndef CLOUDS_H
#define CLOUDS_H

#include <vector>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"

// Struktury wewnetrzne
struct CloudComponent { glm::vec3 offset; float scale; };
struct Cloud {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    std::vector<CloudComponent> components;
};

class CloudManager {
private:
    std::vector<Cloud> clouds;
    float spawnTimer = 0.0f;
    unsigned int vao, vbo, ebo;
    std::vector<unsigned int> indices; 

    // Funkcja pomocnicza do losowania
    float randomFloat(float min, float max) {
        return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
    }

    void SpawnCloud() {
        Cloud newCloud;
        newCloud.position = glm::vec3(randomFloat(-50.0f, 50.0f), randomFloat(10.0f, 20.0f), -70.0f);
        newCloud.velocity = glm::vec3(0.0f, 0.0f, randomFloat(1.5f, 3.0f));

        int numComponents = rand() % 4 + 3;
        for (int i = 0; i < numComponents; ++i) {
            CloudComponent component;
            component.offset = glm::vec3(randomFloat(-2.5f, 2.5f), randomFloat(-1.0f, 1.0f), randomFloat(-1.5f, 1.5f));
            component.scale = randomFloat(1.5f, 2.5f);
            newCloud.components.push_back(component);
        }
        clouds.push_back(newCloud);
    }

public:
    CloudManager() {
        std::vector<float> vertices;
        int segments = 20;
        for (int i = 0; i <= segments; ++i) {
            float theta = glm::pi<float>() * i / segments;
            for (int j = 0; j <= segments; ++j) {
                float phi = 2 * glm::pi<float>() * j / segments;
                vertices.push_back(std::cos(phi) * std::sin(theta));
                vertices.push_back(std::cos(theta));
                vertices.push_back(std::sin(phi) * std::sin(theta));
            }
        }
        for (int i = 0; i < segments; ++i) {
            for (int j = 0; j < segments; ++j) {
                int first = (i * (segments + 1)) + j;
                int second = first + segments + 1;
                indices.push_back(first); indices.push_back(second); indices.push_back(first + 1);
                indices.push_back(second); indices.push_back(second + 1); indices.push_back(first + 1);
            }
        }

        glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); glGenBuffers(1, &ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo); glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo); glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    ~CloudManager() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
    }

    void Update(float deltaTime) {
        spawnTimer += deltaTime;
        if (spawnTimer > 2.0f && clouds.size() < 15) {
            SpawnCloud();
            spawnTimer = 0.0f;
        }

        for (auto it = clouds.begin(); it != clouds.end(); ) {
            it->position += it->velocity * deltaTime;
            if (it->position.z > 50.0f) it = clouds.erase(it);
            else ++it;
        }
    }

    void Draw(Shader& shader) {
        glBindVertexArray(vao);
        shader.setInt("useTexture", 0);
        shader.setVec4("objectColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        for (const auto& cloud : clouds) {
            for (const auto& component : cloud.components) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, cloud.position);
                model = glm::translate(model, component.offset);
                model = glm::scale(model, glm::vec3(component.scale));
                shader.setMat4("model", model);
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
            }
        }
        glBindVertexArray(0);
    }
};

#endif