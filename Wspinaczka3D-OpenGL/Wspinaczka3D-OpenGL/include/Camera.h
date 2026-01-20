#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    // Wektory kamery
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // K�ty Eulera
    float Yaw;
    float Pitch;

    // Ustawienia kamery
    float Distance;
    float MouseSensitivity;

    // Stan myszy
    float LastX;
    float LastY;
    bool FirstMouse;

    Camera(float width, float height) {
        Front = glm::vec3(0.0f, 0.0f, -1.0f);
        WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        Yaw = -90.0f;
        Pitch = 0.0f;
        Distance = 7.0f;
        MouseSensitivity = 0.1f;

        LastX = width / 2.0f;
        LastY = height / 2.0f;
        FirstMouse = true;

        UpdateCameraVectors();
    }

    // Zwraca macierz widoku (LookAt)
    glm::mat4 GetViewMatrix(glm::vec3 targetPos) {
        // Pozycja kamery = cel - wektor_kierunku * dystans
        glm::vec3 cameraPos = targetPos - Front * Distance;

        // Podniesienie kamery lekko do g�ry
        cameraPos.y += 1.5f;

        // Zabezpieczenie przed wchodzeniem pod pod�og�
        if (cameraPos.y < 0.5f) cameraPos.y = 0.5f;

        // Kamera patrzy na cel + lekkie przesuni�cie w g�r� (na �rodek jajka)
        return glm::lookAt(cameraPos, targetPos + glm::vec3(0.0f, 0.5f, 0.0f), Up);
    }

    // Obs�uga ruchu myszk�
    void ProcessMouseMovement(double xpos, double ypos) {
        if (FirstMouse) {
            LastX = static_cast<float>(xpos);
            LastY = static_cast<float>(ypos);
            FirstMouse = false;
        }

        float xoffset = static_cast<float>(xpos) - LastX;
        float yoffset = LastY - static_cast<float>(ypos); // Odwr�cone Y

        LastX = static_cast<float>(xpos);
        LastY = static_cast<float>(ypos);

        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        // Blokada gimbala
        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;

        UpdateCameraVectors();
    }

    // Pobranie wektora przodu "na p�asko" (do chodzenia, �eby nie lata� w g�r�/d�)
    glm::vec3 GetFlatFront() {
        return glm::normalize(glm::vec3(Front.x, 0.0f, Front.z));
    }

private:
    void UpdateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);

        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};

#endif