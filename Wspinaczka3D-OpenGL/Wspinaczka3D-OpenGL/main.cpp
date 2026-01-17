#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <random> 
#include <cmath> 

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "GlassBridge.h" 
#include "Trampoline.h"
#include "Maze.h"
#include "Clouds.h" 
#include "WinZone.h" 
#include "Physics.h"
#include "Camera.h"
#include "UIManager.h" 
#include "Player.h"     
#include "Model.h"
#include "Shader.h"
#include "Ladder.h"
#include "FlyoverBridge.h" //

unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;
enum GameState { GAME_STATE_MENU, GAME_STATE_PLAYING, GAME_STATE_CRASHED };
GameState currentState = GAME_STATE_MENU;
bool needsReset = false;
bool enterKeyPressed = false;

Camera* gameCamera = nullptr;
Physics physics;
Player* player = nullptr;
UIManager* uiManager = nullptr;

glm::vec3 eggPosition = glm::vec3(0.0f, 0.7f, 5.0f);
glm::vec3 previousEggPosition = eggPosition;
float deltaTime = 0.0f, lastFrame = 0.0f;
float maxFallHeight = 0.7f;
int crackCount = 0;
float crashStartTime = 0.0f;
const float CRASH_ANIMATION_DURATION = 0.7f;

Ladder* myLadder = nullptr;
unsigned int ladderTexture = 0;
GlassBridge* glassBridge = nullptr;
Trampoline* bouncyTrampoline = nullptr;
Maze* myMaze = nullptr;

struct MovingPlatform { TableHitbox hitbox; glm::vec3 startPos, endPos; float speed, progress; int direction; glm::vec3 currentOffset; };
std::vector<MovingPlatform> platforms = {
    { {37.5f, 40.5f, -1.5f, 1.5f, 15.0f }, glm::vec3(39.0f, 15.0f, -4.5f), glm::vec3(39.0f, 15.0f, 4.5f), 2.5f, 0.0f, 1, glm::vec3(0.0f) },
    { {33.5f, 36.5f, -1.5f, 1.5f, 15.0f }, glm::vec3(35.0f, 15.0f, -5.0f), glm::vec3(35.0f, 15.0f, 5.0f), 4.0f, 0.2f, -1, glm::vec3(0.0f) },
    { {29.5f, 32.5f, -1.5f, 1.5f, 15.0f }, glm::vec3(31.0f, 15.0f, -6.0f), glm::vec3(31.0f, 15.0f, 6.0f), 3.0f, 0.5f, 1, glm::vec3(0.0f) },
    { {25.5f, 28.5f, -1.5f, 1.5f, 15.0f }, glm::vec3(27.0f, 15.0f, -4.0f), glm::vec3(27.0f, 15.0f, 4.0f), 5.5f, 0.8f, -1, glm::vec3(0.0f) }
};

TableHitbox tables[] = {
    {-2.8f, -1.2f, -0.8f, 0.8f, 0.68f}, {-0.8f, 0.8f, -0.8f, 0.8f, 1.45f}, {1.7f, 3.3f, -0.8f, 0.8f, 1.45f},
    {4.2f, 5.8f, -2.3f, -0.7f, 1.45f}, {6.7f, 8.3f, 0.7f, 2.3f, 1.45f}, {9.2f, 10.8f, -2.3f, -0.7f, 1.45f},
    {11.7f, 13.3f, 0.7f, 2.3f, 1.45f}, {14.2f, 15.8f, -2.3f, -0.7f, 1.45f}, {17.2f, 18.8f, -0.8f, 0.8f, 1.45f},
    {17.2f, 18.8f, 1.7f, 3.3f, 0.68f}
};
TableHitbox safeZone = { 43.0f, 47.0f, -2.0f, 2.0f, 15.0f };
TableHitbox mazeFloor = { 10.0f, 30.0f, 6.0f, 26.0f, 15.0f };

// <--- HITBOX PODUSZKI (BLIŻEJ): Z=27.0 do 30.0 (środek 28.5)
TableHitbox ladderPillow = { 21.5f, 24.5f, 27.0f, 30.0f, 24.0f };

void framebuffer_size_callback(GLFWwindow* w, int width, int height);
void mouse_callback(GLFWwindow* w, double xpos, double ypos);
void processInput(GLFWwindow* w);
static unsigned int loadTexture(const char* path);
void updateCrackWrapper(int count) { if (player) player->UpdateCracks(count); }

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    SCR_WIDTH = mode->width; SCR_HEIGHT = mode->height;
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Wspinaczka3D", monitor, NULL);

    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_DEPTH_TEST);
    Shader ourShader("vertex_shader.glsl", "fragment_shader.glsl");
    gameCamera = new Camera((float)SCR_WIDTH, (float)SCR_HEIGHT);
    uiManager = new UIManager((float)SCR_WIDTH, (float)SCR_HEIGHT, loadTexture("menu_prompt.png"));
    player = new Player();
    CloudManager cloudManager;
    WinZone winZone;

    Model tableModel("models/table.obj");
    Model ladderModel("models/Ladder.fbx");
    Model floorModel("models/floor.obj");
    Model tileModel("models/glass_tile.obj");
    Model trampolineModel("models/trampoline.obj");
    Model pillowModel("models/pillow.obj");
    Model rampModel("models/ramp.obj");
    Model flyoverModel("models/flyover.obj"); //

    ladderTexture = loadTexture("models/wood_ladder.jpg");

    // Drabina (23, 15, 27)
    myLadder = new Ladder(glm::vec3(23.0f, 15.0f, 27.0f), 10.0f, &ladderModel);

    myMaze = new Maze(glm::vec3(11.0f, 15.0f, 7.0f));
    glassBridge = new GlassBridge(glm::vec3(25.0f, 0.0f, 0.0f), 2.85f, &tileModel);
    bouncyTrampoline = new Trampoline(glm::vec3(41.0f, 0.0, 0.0f), 0.4f, 0.5f, 35.0f, &trampolineModel, glm::vec3(0.2f), glm::vec3(0.0f));

    FlyoverBridge* myFlyover = new FlyoverBridge(
        // Pozycja: Zmieniamy Z z 45.0f na 34.0f (zaraz za poduszką)
        // Y zostawiamy na 24.5f (wysokość poduszki to ok. 24.0f + jej grubość)
        glm::vec3(23.0f, 24.0f, 34.0f),

        // Obrót: 0 stopni
        glm::vec3(0.0f, 0.0f, 0.0f),

        // Skala: Zwiększamy z 0.1 na 0.4
        glm::vec3(0.4f, 0.4f, 0.4f),
        &flyoverModel
    );

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame; lastFrame = currentFrame;

        if (needsReset) {
            eggPosition = glm::vec3(0.0f, 0.7f, 5.0f);
            physics.Reset();
            maxFallHeight = 0.7f;
            crackCount = 0;
            player->UpdateCracks(0);
            if (glassBridge) glassBridge->Reset();
            needsReset = false;
        }

        if (currentState == GAME_STATE_PLAYING) previousEggPosition = eggPosition;
        processInput(window);

        if (currentState == GAME_STATE_PLAYING) {
            if (myLadder) physics.isClimbing = myLadder->CheckCollision(eggPosition);

            for (auto& t : tables) physics.CheckHorizontalCollision(eggPosition, previousEggPosition, t);
            physics.CheckHorizontalCollision(eggPosition, previousEggPosition, winZone.rampHorizontalBox);
            physics.CheckHorizontalCollision(eggPosition, previousEggPosition, safeZone);

            physics.CheckHorizontalCollision(eggPosition, previousEggPosition, ladderPillow);

            if (myMaze) myMaze->checkCollision(eggPosition, previousEggPosition);
            if (bouncyTrampoline && eggPosition.y - 0.7f <= 1.0f && glm::distance(glm::vec3(eggPosition.x, 0, eggPosition.z), bouncyTrampoline->position) < bouncyTrampoline->radius + 1.1f) {
                eggPosition.x = previousEggPosition.x; eggPosition.z = previousEggPosition.z;
            }

            float oldY = eggPosition.y;
            physics.ApplyGravity(deltaTime, eggPosition.y);
            if (!physics.isClimbing && physics.velocityY >= 0.0f) maxFallHeight = glm::max(eggPosition.y, maxFallHeight);

            bool standing = physics.isClimbing;
            if (standing) { maxFallHeight = eggPosition.y; physics.velocityY = 0.0f; }

            for (auto& plat : platforms) {
                float dist = glm::distance(plat.startPos, plat.endPos);
                if (dist > 0.001f) {
                    plat.progress += (plat.speed * deltaTime / dist) * plat.direction;
                    if (plat.progress >= 1.0f || plat.progress <= 0.0f) plat.direction *= -1;
                    glm::vec3 curr = glm::mix(plat.startPos, plat.endPos, plat.progress = glm::clamp(plat.progress, 0.0f, 1.0f));
                    plat.currentOffset = curr - glm::mix(plat.startPos, plat.endPos, glm::clamp(plat.progress - (plat.speed * deltaTime * plat.direction / dist), 0.0f, 1.0f));
                    plat.hitbox.minX = curr.x - 1.5f; plat.hitbox.maxX = curr.x + 1.5f;
                    plat.hitbox.minZ = curr.z - 1.5f; plat.hitbox.maxZ = curr.z + 1.5f; plat.hitbox.topY = curr.y;
                    if (!standing && Physics::IsInsideXZ(eggPosition, plat.hitbox) && oldY >= plat.hitbox.topY + 0.5f && eggPosition.y <= plat.hitbox.topY + 0.8f && physics.velocityY <= 0.0f) {
                        eggPosition.y = plat.hitbox.topY + 0.7f; physics.velocityY = 0.0f; physics.canJump = true; standing = true; maxFallHeight = eggPosition.y; eggPosition += plat.currentOffset;
                    }
                }
            }

            if (!standing) {
                int stateInt = (int)currentState;
                standing = winZone.CheckRampCollision(oldY, eggPosition.y, physics.velocityY, 0.7f, eggPosition, physics.canJump, maxFallHeight, 1.5f, 0.9f, crackCount, 3, stateInt, (int)GAME_STATE_CRASHED, currentFrame, crashStartTime, updateCrackWrapper);
                currentState = (GameState)stateInt;
            }
            if (!standing && glassBridge && glassBridge->checkCollision(eggPosition, eggPosition.y, physics.velocityY, 0.7f)) { standing = true; physics.canJump = true; maxFallHeight = eggPosition.y; }
            if (!standing && bouncyTrampoline && bouncyTrampoline->checkCollision(eggPosition, eggPosition.y, physics.velocityY, 0.7f)) { physics.canJump = false; maxFallHeight = eggPosition.y; }

            if (!standing && Physics::IsInsideXZ(eggPosition, safeZone) && oldY >= safeZone.topY + 0.6f && eggPosition.y <= safeZone.topY + 0.7f && physics.velocityY <= 0.0f) {
                maxFallHeight = safeZone.topY + 0.7f; eggPosition.y = maxFallHeight; physics.velocityY = 0.0f; physics.canJump = true; standing = true;
            }

            if (!standing && Physics::IsInsideXZ(eggPosition, ladderPillow) && oldY >= ladderPillow.topY + 0.6f && eggPosition.y <= ladderPillow.topY + 0.7f && physics.velocityY <= 0.0f) {
                maxFallHeight = ladderPillow.topY + 0.7f; eggPosition.y = maxFallHeight; physics.velocityY = 0.0f; physics.canJump = true; standing = true;
            }

            if (!standing && Physics::IsInsideXZ(eggPosition, mazeFloor) && oldY >= mazeFloor.topY + 0.5f && eggPosition.y <= mazeFloor.topY + 0.8f && physics.velocityY <= 0.0f) {
                maxFallHeight = mazeFloor.topY + 0.7f; eggPosition.y = maxFallHeight; physics.velocityY = 0.0f; physics.canJump = true; standing = true;
            }
            if (!standing) {
                for (auto& t : tables) {
                    if (Physics::IsInsideXZ(eggPosition, t) && oldY >= t.topY + 0.5f && eggPosition.y <= t.topY + 0.8f && physics.velocityY <= 0.0f) {
                        float fall = glm::max(maxFallHeight - (t.topY + 0.7f), 0.0f);
                        if (fall >= 1.5f) { currentState = GAME_STATE_CRASHED; crashStartTime = currentFrame; crackCount = 3; }
                        else if (fall >= 0.9f) { crackCount++; if (crackCount >= 3) { currentState = GAME_STATE_CRASHED; crashStartTime = currentFrame; } player->UpdateCracks(crackCount); }
                        maxFallHeight = t.topY + 0.7f; eggPosition.y = maxFallHeight; physics.velocityY = 0.0f; physics.canJump = true; standing = true; break;
                    }
                }
            }
            if (!standing && eggPosition.y < 0.7f && oldY >= 0.6f) {
                float fall = glm::max(maxFallHeight - 0.7f, 0.0f);
                if (fall >= 1.5f) { currentState = GAME_STATE_CRASHED; crashStartTime = currentFrame; crackCount = 3; }
                else if (fall >= 0.9f) { crackCount++; if (crackCount >= 3) { currentState = GAME_STATE_CRASHED; crashStartTime = currentFrame; } player->UpdateCracks(crackCount); }
                maxFallHeight = 0.7f; eggPosition.y = 0.7f; physics.velocityY = 0.0f; physics.canJump = true; standing = true;
            }
            cloudManager.Update(deltaTime);
        }

        glClearColor(0.53f, 0.81f, 0.92f, 1.0f); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ourShader.use();

        glm::mat4 view = (currentState == GAME_STATE_MENU) ?
            glm::lookAt(glm::vec3(0, 5, 15), glm::vec3(0, 2, 0), glm::vec3(0, 1, 0)) :
            gameCamera->GetViewMatrix(eggPosition);

        ourShader.setMat4("projection", glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 150.0f));
        ourShader.setMat4("view", view);

        ourShader.setInt("useTexture", 1);
        ourShader.setMat4("model", glm::translate(glm::mat4(1.0f), glm::vec3(0, -0.01f, 0))); floorModel.Draw(ourShader);
        for (auto& t : tables) { ourShader.setMat4("model", glm::translate(glm::mat4(1.0f), glm::vec3((t.minX + t.maxX) / 2, t.topY - 0.68f, (t.minZ + t.maxZ) / 2))); tableModel.Draw(ourShader); }
        winZone.Draw(ourShader, rampModel);

        if (myLadder) myLadder->Draw(ourShader);

        player->Draw(ourShader, eggPosition, currentState == GAME_STATE_CRASHED, currentFrame - crashStartTime, CRASH_ANIMATION_DURATION);
        if (myMaze) { ourShader.use(); ourShader.setInt("useTexture", 1); myMaze->DrawFloor(ourShader); myMaze->Draw(ourShader); }
        if (glassBridge) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glassBridge->Draw(ourShader); glDisable(GL_BLEND); }
        if (bouncyTrampoline) bouncyTrampoline->Draw(ourShader);

        ourShader.setInt("useTexture", 1);
        ourShader.setMat4("model", glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(45.0f, 14.85f, 0.0f)), glm::vec3(4.0f, 1.0f, 4.0f))); pillowModel.Draw(ourShader);

        // RYSOWANIE PODUSZKI (Z=28.5)
        ourShader.setMat4("model", glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(23.0f, 24.0f, 28.5f)), glm::vec3(3.0f, 1.0f, 3.0f)));
        pillowModel.Draw(ourShader);

        for (auto& p : platforms) { ourShader.setMat4("model", glm::scale(glm::translate(glm::mat4(1.0f), glm::mix(p.startPos, p.endPos, p.progress) - glm::vec3(0, 0.68f, 0)), glm::vec3(2, 1, 2))); tableModel.Draw(ourShader); }

        // --- POPRAWIONE: Rysowanie mostu wewnątrz pętli renderującej (PRZED UI i SwapBuffers) ---
        if (myFlyover) {
            myFlyover->Draw(ourShader);
        }

        if (currentState != GAME_STATE_PLAYING) uiManager->Draw();
        glfwSwapBuffers(window); glfwPollEvents();
    }

    // Czyszczenie pamięci
    delete myFlyover;

    glfwTerminate(); return 0;
}

void processInput(GLFWwindow* w) {
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(w, true);

    if (currentState == GAME_STATE_MENU || currentState == GAME_STATE_CRASHED) {
        if (glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS) {
            if (!enterKeyPressed) {
                currentState = GAME_STATE_PLAYING;
                needsReset = true;
                glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                gameCamera->FirstMouse = true;
                enterKeyPressed = true;
            }
        }
        else {
            enterKeyPressed = false;
        }
    }
    else {
        float speed = (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 5.0f : 2.5f;
        glm::vec3 f = gameCamera->GetFlatFront(), r = gameCamera->Right;
        if (physics.isClimbing) {
            if (glfwGetKey(w, GLFW_KEY_W)) eggPosition.y += speed * deltaTime;
            if (glfwGetKey(w, GLFW_KEY_S)) eggPosition.y -= speed * deltaTime;

            // Limit wspinaczki (sufit)
            eggPosition.y = glm::clamp(eggPosition.y, 0.7f, myLadder->position.y + 10.0f);

            if (glfwGetKey(w, GLFW_KEY_A)) eggPosition -= r * speed * deltaTime;
            if (glfwGetKey(w, GLFW_KEY_D)) eggPosition += r * speed * deltaTime;
        }
        else {
            if (glfwGetKey(w, GLFW_KEY_W)) eggPosition += f * speed * deltaTime;
            if (glfwGetKey(w, GLFW_KEY_S)) eggPosition -= f * speed * deltaTime;
            if (glfwGetKey(w, GLFW_KEY_A)) eggPosition -= r * speed * deltaTime;
            if (glfwGetKey(w, GLFW_KEY_D)) eggPosition += r * speed * deltaTime;
        }
        if (glfwGetKey(w, GLFW_KEY_SPACE)) physics.TryJump();
    }
}
void framebuffer_size_callback(GLFWwindow* w, int width, int height) { glViewport(0, 0, width, height); if (uiManager) uiManager->UpdateProjection((float)width, (float)height); }
void mouse_callback(GLFWwindow* w, double x, double y) { if (currentState == GAME_STATE_PLAYING) gameCamera->ProcessMouseMovement(x, y); }
static unsigned int loadTexture(const char* path) {
    unsigned int id; glGenTextures(1, &id); int w, h, c; stbi_set_flip_vertically_on_load(true);
    unsigned char* d = stbi_load(path, &w, &h, &c, 0);
    if (d) { GLenum f = (c == 4) ? GL_RGBA : GL_RGB; glBindTexture(GL_TEXTURE_2D, id); glTexImage2D(GL_TEXTURE_2D, 0, f, w, h, 0, f, GL_UNSIGNED_BYTE, d); glGenerateMipmap(GL_TEXTURE_2D); }
    stbi_image_free(d); return id;
}