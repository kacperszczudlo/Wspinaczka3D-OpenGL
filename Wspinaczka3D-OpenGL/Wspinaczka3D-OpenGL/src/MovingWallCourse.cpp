#include "MovingWallCourse.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

MovingWallCourse::MovingWallCourse(glm::vec3 pos,
    unsigned int wallTex,
    unsigned int floorTex,
    Mesh* cube)
{
    startPos = pos;
    roadLength = 30.0f;
    roadWidth = 10.0f;

    wallWidth = roadWidth + 0.2f;  // lekki zapas, żeby nie było szpar


    // droga jest rysowana jako box: center w (startPos.x, startPos.y-0.1, startPos.z + roadLength/2)
    // skala: (roadWidth, 0.2, roadLength)
    // topY = (startPos.y - 0.1) + 0.1 = startPos.y

    float halfW = roadWidth * 0.5f;
    float zMin = startPos.z;
    float zMax = startPos.z + roadLength;

    roadHitbox.minX = startPos.x - halfW;
    roadHitbox.maxX = startPos.x + halfW;
    roadHitbox.minZ = zMin;
    roadHitbox.maxZ = zMax;
    roadHitbox.topY = startPos.y; // górna powierzchnia drogi


    wallTexture = wallTex;
    floorTexture = floorTex;
    cubeMesh = cube;

    // 3 ściany
    for (int i = 0; i < 3; ++i) {
        Wall w;
        w.zPos = startPos.z + 6.0f + i * 8.0f;
        w.phase = i * 1.5f;
        walls.push_back(w);
    }

    // prosta płaska droga (użyjemy cubeMesh jako cienkiego boxa)
    floorMesh = cubeMesh;
}

void MovingWallCourse::Update(float dt)
{
    for (auto& w : walls) {
        w.zPos -= wallMoveSpeed * dt;

        float despawnZ = startPos.z + 2.0f;
        if (w.zPos < despawnZ) {
            w.zPos = startPos.z + roadLength + 2.0f;
        }
    }
}



void MovingWallCourse::Draw(Shader& shader)
{
    // --- ROAD ---
    shader.setInt("useTexture", 1);
    shader.setInt("useWorldUV", 0);

    glBindTexture(GL_TEXTURE_2D, floorTexture);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model,
        glm::vec3(startPos.x, startPos.y - 0.1f, startPos.z + roadLength * 0.5f));
    model = glm::scale(model, glm::vec3(roadWidth, 0.2f, roadLength));
    shader.setMat4("model", model);
    floorMesh->Draw(shader);

    // --- WALLS ---
    glBindTexture(GL_TEXTURE_2D, wallTexture);
    shader.setInt("useWorldUV", 1);
    shader.setFloat("texWorldSize", 2.0f);

    float sidePillarWidth =
        (wallWidth - 2.0f * holeWidth - midPillarWidth) * 0.5f;

    for (const auto& w : walls)
    {
        // funkcja pomocnicza do rysowania jednego bloku
        auto drawBlock = [&](float centerX, float widthX)
            {
                glm::mat4 m(1.0f);
                m = glm::translate(m,
                    glm::vec3(centerX,
                        startPos.y + wallHeight * 0.5f,
                        w.zPos));
                m = glm::scale(m,
                    glm::vec3(widthX, wallHeight, wallThickness));
                shader.setMat4("model", m);
                cubeMesh->Draw(shader);
            };

        float cx = startPos.x;

        // LEWY FILAR
        drawBlock(cx - (holeWidth + midPillarWidth * 0.5f + sidePillarWidth * 0.5f),
            sidePillarWidth);

        // ŚRODKOWY FILAR
        drawBlock(cx, midPillarWidth);

        // PRAWY FILAR
        drawBlock(cx + (holeWidth + midPillarWidth * 0.5f + sidePillarWidth * 0.5f),
            sidePillarWidth);
    }

    shader.setInt("useWorldUV", 0);
}


bool MovingWallCourse::CheckPlayer(const glm::vec3& p, float r)
{
    // 1) działamy tylko na drodze
    float halfW = roadWidth * 0.5f;
    bool onRoad =
        (p.x > startPos.x - halfW && p.x < startPos.x + halfW) &&
        (p.z > startPos.z && p.z < startPos.z + roadLength);

    if (!onRoad) return false;

    // 2) testujemy każdą ścianę
    for (const auto& w : walls)
    {
        // bliskość w osi Z 
        if (fabs(p.z - w.zPos) > (r + 0.5f)) 
            continue;

        // wysokość ściany
        float bottomY = startPos.y;
        float topY    = startPos.y + wallHeight;

        if (p.y + r < bottomY || p.y - r > topY)
            continue;

        float localX = p.x - startPos.x;
        float holeHalf = holeWidth * 0.5f;

        float goodCenter = -(midPillarWidth * 0.5f + holeHalf);
        float badCenter  = +(midPillarWidth * 0.5f + holeHalf);

        bool inGood = (localX > goodCenter - holeHalf && localX < goodCenter + holeHalf);
        bool inBad  = (localX > badCenter  - holeHalf && localX < badCenter  + holeHalf);

        if (inGood) return false; // przechodzisz
        if (inBad)  return true;  // zły otwór -> kill

        // poza otworami = uderzenie w ramę -> kill
        return true;
    }

    return false;
}


