#pragma once
#pragma once
#include <string>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Shader.h"

class Skybox {
public:
    // faces w kolejnoœci LearnOpenGL:
    // right, left, top, bottom, front, back
    Skybox(const std::vector<std::string>& faces);

    void Draw(const glm::mat4& view, const glm::mat4& projection);

private:
    unsigned int skyboxVAO = 0;
    unsigned int skyboxVBO = 0;
    unsigned int cubemapTexture = 0;

    Shader skyboxShader;

    void setupMesh();
    unsigned int loadCubemap(const std::vector<std::string>& faces);
};

