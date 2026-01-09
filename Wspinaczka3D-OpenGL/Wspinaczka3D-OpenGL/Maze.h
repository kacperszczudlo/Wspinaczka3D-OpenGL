#ifndef MAZE_H
#define MAZE_H

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
#include "Mesh.h"

class Maze {
public:
    struct Wall {
        float x, z;
        float width;
    };

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
        shader.setInt("useTexture", 0);
        shader.setVec4("objectColor", glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)); // Czarny

        // Obliczamy œrodek labiryntu
        // StartPosition to lewy górny róg pierwszego bloku.
        // Mapa ma szerokoœæ 10 bloków * 2.0 = 20.0
        // Œrodek w X = startX + (20 / 2) - (pó³ bloku korekty)
        // Dla uproszczenia: Geometryczny œrodek planszy 10x10
        float centerX = startPosition.x + (mazeWidth / 2.0f) - (blockSize / 2.0f);
        float centerZ = startPosition.z + (mazeDepth / 2.0f) - (blockSize / 2.0f);

        glm::mat4 model = glm::mat4(1.0f);

        // Ustawiamy pod³ogê:
        // Y = startPosition.y - 0.1f (¿eby by³a minimalnie pod œcianami, ale styka³a siê)
        model = glm::translate(model, glm::vec3(centerX, startPosition.y - 0.1f, centerZ));

        // Skalujemy: Szerokoœæ labiryntu, gruboœæ 0.2, g³êbokoœæ labiryntu
        model = glm::scale(model, glm::vec3(mazeWidth, 0.2f, mazeDepth));

        shader.setMat4("model", model);
        cubeMesh->Draw(shader);
    }

    void Draw(Shader& shader) {
        shader.setInt("useTexture", 0);
        shader.setVec4("objectColor", glm::vec4(0.6f, 0.6f, 0.6f, 1.0f)); // Szare œciany

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