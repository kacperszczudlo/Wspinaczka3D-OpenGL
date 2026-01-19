#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

#include "Shader.h"

class Ground {
public:
    // size = po³owa rozmiaru w œwiecie (np. 100 => plane od -100 do +100)
    // tiling = ile razy tekstura ma siê powtórzyæ na ca³ym plane
    Ground(const std::string& texturePath, float size = 120.0f, float tiling = 40.0f, float y = -0.01f);

    void Draw(Shader& shader);

private:
    unsigned int VAO = 0, VBO = 0;
    unsigned int textureID = 0;
    float size = 120.0f;
    float tiling = 40.0f;
    float y = -0.01f;

    unsigned int loadTexture2D(const std::string& path);
};
