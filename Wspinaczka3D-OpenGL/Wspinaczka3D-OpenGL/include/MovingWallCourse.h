#pragma once
#ifndef MOVING_WALL_COURSE_H
#define MOVING_WALL_COURSE_H

#include <glm/glm.hpp>
#include <vector>
#include "Mesh.h"
#include "Shader.h"
#include "Physics.h"

class MovingWallCourse {
public:
    struct Wall {
        float zPos;
        float phase;
    };

    glm::vec3 startPos;
    float roadLength;
    float roadWidth;

    float wallSpeed = 1.5f;
    float wallAmplitude = 2.0f;

    float wallWidth = 8.0f;
    float wallHeight = 5.0f;
    float holeWidth = 1.2f;
    float holeHeight = 2.5f;

    float wallMoveSpeed = 6.0f;
    float resetZOffset = 35.0f;

    float wallThickness = 0.6f;
    float midPillarWidth = 1.2f;


    unsigned int wallTexture;
    unsigned int floorTexture;

    Mesh* cubeMesh;
    Mesh* floorMesh;

    std::vector<Wall> walls;

    MovingWallCourse(glm::vec3 pos,
        unsigned int wallTex,
        unsigned int floorTex,
        Mesh* cube);

    void Update(float time);
    void Draw(Shader& shader);

    // Zwraca true jeœli gracz trafi³ w Z£Y otwór
    bool CheckPlayer(const glm::vec3& playerPos, float radius);

    TableHitbox roadHitbox;
    TableHitbox GetRoadHitbox() const { return roadHitbox; }

};



#endif
