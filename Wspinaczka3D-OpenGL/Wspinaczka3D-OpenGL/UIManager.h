#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"

class UIManager {
public:
    Shader* uiShader;
    unsigned int VAO, VBO;
    unsigned int textureID;

    UIManager(float scrWidth, float scrHeight, unsigned int texID) {
        textureID = texID;
        uiShader = new Shader("ui_vertex.glsl", "ui_fragment.glsl");

        UpdateProjection(scrWidth, scrHeight);

        // Ustawienie prostok�ta na �rodku ekranu
        float width_ui = 500.0f;
        float height_ui = 500.0f;
        float x_pos = (scrWidth - width_ui) / 2.0f;
        float y_pos = (scrHeight - height_ui) / 2.0f;

        float vertices[] = {
            x_pos,            y_pos + height_ui,   0.0f, 1.0f,
            x_pos,            y_pos,               0.0f, 0.0f,
            x_pos + width_ui, y_pos,               1.0f, 0.0f,

            x_pos,            y_pos + height_ui,   0.0f, 1.0f,
            x_pos + width_ui, y_pos,               1.0f, 0.0f,
            x_pos + width_ui, y_pos + height_ui,   1.0f, 1.0f
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }

    ~UIManager() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        delete uiShader;
    }

    void UpdateProjection(float width, float height) {
        uiShader->use();
        glm::mat4 projection = glm::ortho(0.0f, width, 0.0f, height);
        uiShader->setMat4("projection", projection);
    }

    void Draw() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST); // UI zawsze na wierzchu

        uiShader->use();
        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }
};

#endif