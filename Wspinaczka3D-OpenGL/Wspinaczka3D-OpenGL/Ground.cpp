#include "Ground.h"
#include <iostream>

#include "stb_image.h"

Ground::Ground(const std::string& texturePath, float size, float tiling, float y)
    : size(size), tiling(tiling), y(y)
{
    // plane 2 trójk¹ty, atrybuty: pos(3), normal(3), uv(2)
    float s = size;
    float t = tiling;

    float vertices[] = {
        // pos                  // normal         // uv
        -s, y, -s,              0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         s, y, -s,              0.0f, 1.0f, 0.0f,  t,    0.0f,
         s, y,  s,              0.0f, 1.0f, 0.0f,  t,    t,

         s, y,  s,              0.0f, 1.0f, 0.0f,  t,    t,
        -s, y,  s,              0.0f, 1.0f, 0.0f,  0.0f, t,
        -s, y, -s,              0.0f, 1.0f, 0.0f,  0.0f, 0.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // location 0: position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    // location 1: normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    // location 2: texcoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);

    textureID = loadTexture2D(texturePath);
}

unsigned int Ground::loadTexture2D(const std::string& path)
{
    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // tiling
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // filtrowanie
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int w, h, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 0);

    if (!data) {
        std::cout << "Failed to load ground texture: " << path << "\n";
        return tex;
    }

    GLenum format = GL_RGB;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    return tex;
}

void Ground::Draw(Shader& shader)
{
    // Shader ju¿ jest .use() w mainie przed RenderScene, ale to ustawiamy tu:
    shader.setInt("useTexture", 1);

    // Twoje shadery u¿ywaj¹ texture_diffuse1
    shader.setInt("texture_diffuse1", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // model = identity, bo y ju¿ jest w wierzcho³kach
    shader.setMat4("model", glm::mat4(1.0f));

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
