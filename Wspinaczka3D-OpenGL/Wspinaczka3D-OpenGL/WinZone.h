#ifndef WINZONE_H
#define WINZONE_H

#include <glm/glm.hpp>
#include "Model.h"
#include "Shader.h"

// Przenosimy struktury hitbox�w tutaj, bo s� u�ywane globalnie
struct TableHitbox {
    float minX, maxX;
    float minZ, maxZ;
    float topY;
};

struct RampHitbox {
    float minX, maxX;
    float minZ, maxZ;
    float startY, endY;
    float lengthZ;
};

class WinZone {
public:
    RampHitbox ramp;
    TableHitbox rampHorizontalBox;

    WinZone() {
        // Konfiguracja Rampy ( X=20.0f. D�ugo�� 4m do X=24.0f)
        ramp = { 20.0f, 24.0f, -1.2f, 1.2f, 2.05f, 2.85f, 4.0f };
        // Hitbox boczny rampy
        rampHorizontalBox = { 20.0f, 24.0f, -0.9f, 0.9f, 2.05f };
    }

    void Draw(Shader& shader, Model& rampModel) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(20.0f, 2.05f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        shader.setMat4("model", model);
        rampModel.Draw(shader);
    }

    // Ta wielka funkcja sprawdzaj�ca kolizj� z ramp�
    // Zwraca true, je�li stoimy na rampie
    bool CheckRampCollision(float oldY, float newY, float& velocityY,
        float eggHalfHeight, glm::vec3& eggPosition,
        bool& canJump, float& maxFallHeight,
        float crashThreshold, float crackThreshold,
        int& crackCount, const int maxCracks,
        int& gameStateRef, int crashedStateValue,
        float currentFrame, float& crashStartTime,
        void (*updateCrackGeom)(int))
    {
        // Czy jeste�my w og�le w obszarze XZ rampy?
        if (eggPosition.x <= ramp.minX || eggPosition.x >= ramp.maxX ||
            eggPosition.z <= ramp.minZ || eggPosition.z >= ramp.maxZ) {
            return false;
        }

        float distanceX = ramp.maxX - ramp.minX;
        float heightChange = ramp.endY - ramp.startY;
        float rampRatio = glm::clamp((eggPosition.x - ramp.minX) / distanceX, 0.0f, 1.0f);

        float surfaceY = ramp.startY + heightChange * rampRatio;
        float desiredCenterY = surfaceY + eggHalfHeight;

        // Sprawdzenie wysoko�ci
        if (oldY >= desiredCenterY - 0.5f && newY <= desiredCenterY && velocityY <= 0.0f) {

            // Obra�enia
            float fallDistance = maxFallHeight - desiredCenterY;
            if (fallDistance < 0.0f) fallDistance = 0.0f;

            if (fallDistance >= crashThreshold) {
                gameStateRef = crashedStateValue;
                crashStartTime = currentFrame;
                crackCount = maxCracks;
            }
            else if (fallDistance >= crackThreshold) {
                crackCount = glm::min(crackCount + 1, maxCracks);
                if (crackCount >= maxCracks) {
                    gameStateRef = crashedStateValue;
                    crashStartTime = currentFrame;
                }
                updateCrackGeom(crackCount);
            }

            // Fizyka
            maxFallHeight = desiredCenterY;
            eggPosition.y = desiredCenterY;
            velocityY = 0.0f;
            canJump = true;
            return true;
        }
        return false;
    }
};

#endif