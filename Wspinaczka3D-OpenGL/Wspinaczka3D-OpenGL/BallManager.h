#ifndef BALL_MANAGER_H
#define BALL_MANAGER_H

#include <vector>
#include <cstdlib> // Do losowania (rand)
#include <ctime>   // Do ziarna losowoœci
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Model.h"
#include "Shader.h"

struct RollingBall {
    glm::vec3 position;
    glm::vec3 velocity; // Teraz przechowujemy wektor prêdkoœci (X, Y, Z)
    float rotationAngle;
    bool active;
    bool isFalling;     // Czy kula ju¿ spada z mostu?
};

class BallManager {
public:
    std::vector<RollingBall> balls;
    Model* ballModel;

    // Ustawienia
    float spawnTimer;
    float spawnInterval;
    float ballRadius;

    // Sta³e poziomu
    float floorLevel;
    float startX;
    float bridgeEdgeX; // KrawêdŸ mostu, gdzie kule zaczynaj¹ spadaæ
    float deleteX;     // Punkt ca³kowitego usuniêcia kuli

    // Zakres szerokoœci mostu (Z)
    float minZ;
    float maxZ;

    BallManager(Model* modelRef) {
        ballModel = modelRef;
        spawnTimer = 0.0f;

        // --- KONFIGURACJA TRUDNOŒCI ---
        spawnInterval = 2.0f; // Szybciej! Co 2 sekundy nowa kula
        ballRadius = 0.5f;

        // Most FlyoverBridge
        floorLevel = 23.4f + ballRadius;

        // Startuje daleko na pocz¹tku mostu
        startX = -42.0f;

        // Koniec mostu (tam gdzie s¹ barierki w main.cpp)
        // Kule zaczn¹ spadaæ jak min¹ X = 28.0f
        bridgeEdgeX = 28.0f;

        // Usuwamy je dopiero jak spadn¹ g³êboko w dó³ lub polec¹ daleko
        deleteX = 40.0f;

        // Zakres chodzenia po moœcie (pomiêdzy barierkami)
        // Barierki s¹ na ok. 28.5 i 31.6. Zostawiamy margines dla kul.
        minZ = 29.0f;
        maxZ = 31.0f;

        // Inicjalizacja losowoœci
        std::srand(static_cast<unsigned int>(std::time(0)));
    }

    void Update(float deltaTime) {
        // 1. Spawnowanie
        spawnTimer += deltaTime;
        if (spawnTimer >= spawnInterval) {
            SpawnBall();
            spawnTimer = 0.0f; // Reset licznika
        }

        // 2. Fizyka kul
        for (auto& ball : balls) {
            if (!ball.active) continue;

            // -- RUCH --
            // Kula porusza siê zgodnie ze swoj¹ prêdkoœci¹
            ball.position += ball.velocity * deltaTime;

            // --- NOWOŒÆ: ODBIJANIE OD BARIEREK ---
            // ¯eby kule nie spada³y z boków mostu przed koñcem trasy
            if (!ball.isFalling) {
                // Lewa barierka (ok. 28.5 + promieñ kulki)
                float wallLeft = 28.5f + ballRadius;
                // Prawa barierka (ok. 31.6 - promieñ kulki)
                float wallRight = 31.6f - ballRadius;

                // Sprawdzenie lewej œciany
                if (ball.position.z < wallLeft) {
                    ball.position.z = wallLeft;       // Wypchnij ze œciany
                    ball.velocity.z *= -1.0f;         // Odbij wektor (zmieñ znak na przeciwny)
                }
                // Sprawdzenie prawej œciany
                else if (ball.position.z > wallRight) {
                    ball.position.z = wallRight;      // Wypchnij ze œciany
                    ball.velocity.z *= -1.0f;         // Odbij wektor
                }
            }
            // -------------------------------------

            // -- OBRÓT --
            // Krêci siê tylko w osi X (toczenie), dopóki nie spada
            if (!ball.isFalling) {
                // Obliczamy ile siê obróciæ na podstawie przebytej drogi w X
                float moveStep = ball.velocity.x * deltaTime;
                ball.rotationAngle -= (moveStep / ballRadius) * (180.0f / 3.14159f);
            }

            // -- SPADANIE (GRAWITACJA) --
            // Jeœli kula minê³a krawêdŸ mostu (bridgeEdgeX)
            if (ball.position.x > bridgeEdgeX) {
                ball.isFalling = true;
                // Dodajemy grawitacjê do prêdkoœci Y
                ball.velocity.y -= 15.0f * deltaTime; // Grawitacja

                // Zachowujemy pêd do przodu (X), wiêc leci ³ukiem w dó³
            }
            else {
                // Jeœli jest na moœcie, trzymamy j¹ na pod³odze
                ball.position.y = floorLevel;
                ball.velocity.y = 0.0f;
            }

            // -- USUWANIE --
            // Jeœli kula spad³a bardzo nisko ALBO polecia³a za daleko
            if (ball.position.y < -10.0f || ball.position.x > deleteX) {
                ball.active = false;
            }
        }
    }

    void Draw(Shader& shader) {
        shader.setInt("useTexture", 1);
        shader.setVec4("objectColor", glm::vec4(1.0f));

        for (const auto& ball : balls) {
            if (!ball.active) continue;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, ball.position);
            model = glm::rotate(model, glm::radians(ball.rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(ballRadius));

            shader.setMat4("model", model);
            ballModel->Draw(shader);
        }
    }

    bool CheckCollision(glm::vec3 playerPos) {
        float playerRadius = 0.5f;

        for (auto& ball : balls) {
            if (!ball.active) continue;

            // Jeœli kula jest daleko w X (optymalizacja)
            if (abs(playerPos.x - ball.position.x) > 2.0f) continue;

            // Dok³adny dystans 3D (dzia³a te¿ jak kula spada na g³owê!)
            float dist = glm::distance(playerPos, ball.position);

            if (dist < (ballRadius + playerRadius - 0.1f)) {
                ball.active = false; // Kula znika po uderzeniu
                return true;
            }
        }
        return false;
    }

private:
    // Funkcja pomocnicza do losowania floatów
    float randomFloat(float min, float max) {
        return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
    }

    void SpawnBall() {
        // Szukamy wolnego slotu w wektorze (recykling)
        bool spawned = false;
        for (auto& ball : balls) {
            if (!ball.active) {
                ResetBall(ball);
                spawned = true;
                break;
            }
        }
        // Jeœli nie ma wolnego slotu, dodajemy now¹ kulê
        if (!spawned) {
            RollingBall b;
            ResetBall(b);
            balls.push_back(b);
        }
    }

    void ResetBall(RollingBall& b) {
        // LOSOWANIE POZYCJI Z (Lewo / Œrodek / Prawo)
        float randZ = randomFloat(minZ, maxZ);

        // LOSOWANIE PRÊDKOŒCI (¯eby nie wszystkie toczy³y siê tak samo)
        float speed = randomFloat(5.0f, 9.0f); // Miêdzy 5 a 9 jednostek

        // Opcjonalnie: Lekki k¹t (kula mo¿e lekko zbaczaæ z kursu)
        // ZWIÊKSZY£EM MINIMALNIE DRIFT, bo teraz mamy odbijanie, wiêc bêdzie ciekawiej
        float driftZ = randomFloat(-1.5f, 1.5f);

        b.position = glm::vec3(startX, floorLevel, randZ);
        b.velocity = glm::vec3(speed, 0.0f, driftZ); // Ustawiamy wektor prêdkoœci
        b.rotationAngle = 0.0f;
        b.active = true;
        b.isFalling = false;
    }
};

#endif