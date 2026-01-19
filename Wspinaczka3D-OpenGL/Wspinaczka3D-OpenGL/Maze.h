#ifndef MAZE_H
#define MAZE_H

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
#include "Mesh.h"
#include <iostream>

class Maze {
public:
    struct Wall {
        float x, z;
        float width;
    };
    unsigned int wallTextureID = 0;
    unsigned int floorTextureID = 0;
    Mesh* floorMesh = nullptr;

    float floorThickness = 0.4f; // np. 0.2–0.6 zale¿nie od skali
    Mesh* floorSlabMesh = nullptr;


    unsigned int loadTexture2D(const std::string& path);

    std::vector<Wall> walls;
    glm::vec3 startPosition;
    Mesh* cubeMesh;
    float blockSize;
    float mazeWidth;  // Szerokoœæ ca³ego labiryntu
    float mazeDepth;  // G³êbokoœæ ca³ego labiryntu

    Maze(glm::vec3 pos) {
        startPosition = pos;
        blockSize = 2.0f; // Rozmiar bloku 2x2

        // Labirynt ma 10x10 pól
        mazeWidth = 10 * blockSize;
        mazeDepth = 10 * blockSize;

        // 1. TWORZENIE GEOMETRII SZEŒCIANU
        std::vector<Vertex> vertices = {
            // Ty³
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
            // Przód
            {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            // Lewo
            {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            // Prawo
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            // Dó³
            {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
            // Góra
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}
        };

        std::vector<unsigned int> indices;
        for (unsigned int i = 0; i < vertices.size(); i++) indices.push_back(i);
        std::vector<Texture> textures;
        cubeMesh = new Mesh(vertices, indices, textures);

        // --- TEXTURES (zmieñ œcie¿ki jak chcesz) ---
        wallTextureID = loadTexture2D("models/textures/maze_wall.png");   // np. ceg³a/kamieñ
        floorTextureID = loadTexture2D("models/textures/maze_floor.png");  // np. p³ytki/kamieñ

        // --- FLOOR MESH (plane z tilingiem) ---
        float halfW = mazeWidth * 0.5f;
        float halfD = mazeDepth * 0.5f;

        float topY = 0.0f;
        float botY = -floorThickness;

        float tileTop = 10.0f;   // tiling na górze
        float tileSide = 2.0f;   // tiling na bokach (mo¿esz dopasowaæ)

        std::vector<Vertex> v;

        // --- TOP (normal up) ---
        v.push_back({ {-halfW, topY, -halfD}, {0,1,0}, {0,0} });
        v.push_back({ { halfW, topY, -halfD}, {0,1,0}, {tileTop,0} });
        v.push_back({ { halfW, topY,  halfD}, {0,1,0}, {tileTop,tileTop} });
        v.push_back({ { halfW, topY,  halfD}, {0,1,0}, {tileTop,tileTop} });
        v.push_back({ {-halfW, topY,  halfD}, {0,1,0}, {0,tileTop} });
        v.push_back({ {-halfW, topY, -halfD}, {0,1,0}, {0,0} });

        // --- BOTTOM (normal down) ---
        v.push_back({ {-halfW, botY,  halfD}, {0,-1,0}, {0,0} });
        v.push_back({ { halfW, botY,  halfD}, {0,-1,0}, {tileTop,0} });
        v.push_back({ { halfW, botY, -halfD}, {0,-1,0}, {tileTop,tileTop} });
        v.push_back({ { halfW, botY, -halfD}, {0,-1,0}, {tileTop,tileTop} });
        v.push_back({ {-halfW, botY, -halfD}, {0,-1,0}, {0,tileTop} });
        v.push_back({ {-halfW, botY,  halfD}, {0,-1,0}, {0,0} });

        // --- SIDE +Z (front) normal (0,0,1) ---
        v.push_back({ {-halfW, botY,  halfD}, {0,0,1}, {0,0} });
        v.push_back({ { halfW, botY,  halfD}, {0,0,1}, {tileSide,0} });
        v.push_back({ { halfW, topY,  halfD}, {0,0,1}, {tileSide,1} });
        v.push_back({ { halfW, topY,  halfD}, {0,0,1}, {tileSide,1} });
        v.push_back({ {-halfW, topY,  halfD}, {0,0,1}, {0,1} });
        v.push_back({ {-halfW, botY,  halfD}, {0,0,1}, {0,0} });

        // --- SIDE -Z (back) normal (0,0,-1) ---
        v.push_back({ { halfW, botY, -halfD}, {0,0,-1}, {0,0} });
        v.push_back({ {-halfW, botY, -halfD}, {0,0,-1}, {tileSide,0} });
        v.push_back({ {-halfW, topY, -halfD}, {0,0,-1}, {tileSide,1} });
        v.push_back({ {-halfW, topY, -halfD}, {0,0,-1}, {tileSide,1} });
        v.push_back({ { halfW, topY, -halfD}, {0,0,-1}, {0,1} });
        v.push_back({ { halfW, botY, -halfD}, {0,0,-1}, {0,0} });

        // --- SIDE +X (right) normal (1,0,0) ---
        v.push_back({ { halfW, botY,  halfD}, {1,0,0}, {0,0} });
        v.push_back({ { halfW, botY, -halfD}, {1,0,0}, {tileSide,0} });
        v.push_back({ { halfW, topY, -halfD}, {1,0,0}, {tileSide,1} });
        v.push_back({ { halfW, topY, -halfD}, {1,0,0}, {tileSide,1} });
        v.push_back({ { halfW, topY,  halfD}, {1,0,0}, {0,1} });
        v.push_back({ { halfW, botY,  halfD}, {1,0,0}, {0,0} });

        // --- SIDE -X (left) normal (-1,0,0) ---
        v.push_back({ {-halfW, botY, -halfD}, {-1,0,0}, {0,0} });
        v.push_back({ {-halfW, botY,  halfD}, {-1,0,0}, {tileSide,0} });
        v.push_back({ {-halfW, topY,  halfD}, {-1,0,0}, {tileSide,1} });
        v.push_back({ {-halfW, topY,  halfD}, {-1,0,0}, {tileSide,1} });
        v.push_back({ {-halfW, topY, -halfD}, {-1,0,0}, {0,1} });
        v.push_back({ {-halfW, botY, -halfD}, {-1,0,0}, {0,0} });

        std::vector<unsigned int> idx;
        idx.reserve(v.size());
        for (unsigned int i = 0; i < v.size(); i++) idx.push_back(i);

        std::vector<Texture> noTex;
        floorSlabMesh = new Mesh(v, idx, noTex);



        // 2. MAPA LABIRYNTU
        const char* mapLayout[10] = {
            "bbbbbbbb b",
            "b   rrrr b",
            "b     rr b",
            "b wwwwww b",
            "b     rr b",
            "bwwrr    b",
            "b      rrb",
            "br rrrrrrb",
            "br       b",
            "bbbbbb bbb"
        };

        for (int row = 0; row < 10; ++row) {
            for (int col = 0; col < 10; ++col) {
                if (mapLayout[row][col] != ' ') {
                    Wall w;
                    w.x = startPosition.x + (col * blockSize);
                    w.z = startPosition.z + (row * blockSize);
                    w.width = blockSize;
                    walls.push_back(w);
                }
            }
        }
    }

    // Nowa funkcja do rysowania pod³ogi pod labiryntem
    void DrawFloor(Shader& shader) {
        shader.setInt("useTexture", 1);
        shader.setInt("texture_diffuse1", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorTextureID);

        float centerX = startPosition.x + (mazeWidth / 2.0f) - (blockSize / 2.0f);
        float centerZ = startPosition.z + (mazeDepth / 2.0f) - (blockSize / 2.0f);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(centerX, startPosition.y, centerZ));
        shader.setMat4("model", model);
        floorSlabMesh->Draw(shader);
    }


    void Draw(Shader& shader) {
        shader.setInt("useTexture", 1);
        shader.setInt("texture_diffuse1", 0);
        shader.setInt("useWorldUV", 1);
        shader.setFloat("texWorldSize", 2.0f); // testuj: 1.5 / 2 / 3


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, wallTextureID);


        for (const auto& w : walls) {
            glm::mat4 model = glm::mat4(1.0f);

            // Œciana ma wysokoœæ 5.0f. Jej œrodek jest w 2.5f.
            // Chcemy, ¿eby spód œciany by³ na startPosition.y
            // Wiêc œrodek = startPosition.y + 2.5f
            float wallHeight = 5.0f;
            float yPos = startPosition.y + (wallHeight / 2.0f);

            model = glm::translate(model, glm::vec3(w.x, yPos, w.z));
            model = glm::scale(model, glm::vec3(blockSize, wallHeight, blockSize));

            shader.setMat4("model", model);
            cubeMesh->Draw(shader);
        }
        shader.setInt("useWorldUV", 0);

    }

    void checkCollision(glm::vec3& playerPos, const glm::vec3& oldPos) {
        float playerRadius = 0.4f;
        float wallHalf = blockSize / 2.0f;

        // Sprawdzamy wysokoœæ (czy jesteœmy na poziomie labiryntu)
        if (playerPos.y < startPosition.y - 1.0f || playerPos.y > startPosition.y + 6.0f) {
            return;
        }

        for (const auto& w : walls) {
            if (playerPos.x + playerRadius > w.x - wallHalf &&
                playerPos.x - playerRadius < w.x + wallHalf &&
                playerPos.z + playerRadius > w.z - wallHalf &&
                playerPos.z - playerRadius < w.z + wallHalf) {

                playerPos.x = oldPos.x;
                playerPos.z = oldPos.z;
                return;
            }
        }
    }
};
#endif