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
#include "FlyoverBridge.h"
#include "Skybox.h"
#include "Ground.h"
#include "BallManager.h" 
#include "MovingWallCourse.h"
#include "WindyTileBridge.h"
#include "FinalWinZone.h"
#include "WindParticles.h"

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

//OG SCIEZKA NIE USUWAC
glm::vec3 eggPosition = glm::vec3(0.0f, 0.7f, 5.0f);
//glm::vec3 eggPosition = glm::vec3(23.0f, 15.8f, 25.0f);
//glm::vec3 eggPosition = glm::vec3(-46.0f, 24.3f, 57.0f);

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
BallManager* ballManager = nullptr;
MovingWallCourse* wallCourse = nullptr;
WindyTileBridge* windyBridge = nullptr;
WindParticles* windParticles = nullptr;
FinalWinZone* finalWinZone = nullptr;
bool gameWon = false;
bool showWinMessage = false;
std::string winMessage = "";
float winMessageDisplayTime = 0.0f;
const float WIN_MESSAGE_DURATION = 5.0f;
bool restrictMovementToWinZone = false; // Ograniczenie ruchu po wygranej

// --- SHADOWS ---
const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
unsigned int depthMapFBO = 0;
unsigned int depthMap = 0;

struct MovingPlatform {
    TableHitbox hitbox;
    glm::vec3 startPos, endPos;
    float speed, progress;
    int direction;
    glm::vec3 currentOffset;
};

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

TableHitbox midSafeZone = { 43.0f, 47.0f, -2.0f, 2.0f, 15.0f };
TableHitbox mazeFloor = { 10.0f, 30.0f, 6.0f, 26.0f, 15.0f };

// HITBOX PODUSZKI
TableHitbox ladderPillow = { -40.5f, 28.0f, 27.8f, 33.0f, 22.9f };

// Barierka TYLNA
TableHitbox barrierBack = { -40.5f, 28.0f, 28.1f, 28.5f, 23.6f };
// Barierka PRZEDNIA
TableHitbox barrierFront = { -40.5f, 28.0f, 31.6f, 32.0f, 23.6f };

void framebuffer_size_callback(GLFWwindow* w, int width, int height);
void mouse_callback(GLFWwindow* w, double xpos, double ypos);
void processInput(GLFWwindow* w);
static unsigned int loadTexture(const char* path);
void updateCrackWrapper(int count) { if (player) player->UpdateCracks(count); }


void RenderScene(Shader& shader,
    Ground& ground, Model& tableModel, Model& rampModel,
    WinZone& winZone,
    Ladder* myLadder,
    Player* player, glm::vec3 eggPosition, GameState currentState,
    float currentFrame, float crashStartTime, float CRASH_ANIMATION_DURATION,
    Maze* myMaze, GlassBridge* glassBridge, Trampoline* bouncyTrampoline,
    std::vector<MovingPlatform>& platforms,
    FlyoverBridge* myFlyover,
    BallManager* ballManager,
    Model& pillowModel,
    WindyTileBridge* windyBridge,
    FinalWinZone* finalWinZone);

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //// Pobierz monitor
    //GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    //const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    //SCR_WIDTH = mode->width;
    //SCR_HEIGHT = mode->height;

    //GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Wspinaczka3D", monitor, NULL);

    SCR_WIDTH = 1280;
    SCR_HEIGHT = 720;

    // Czwarty argument NULL oznacza tryb okienkowy (zamiast monitora)
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Wspinaczka3D", NULL, NULL);
    if (!window) return -1;

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_DEPTH_TEST);

    Skybox skybox({
        "assets/skybox/Daylight Box_Right.bmp",
        "assets/skybox/Daylight Box_Left.bmp",
        "assets/skybox/Daylight Box_Top.bmp",
        "assets/skybox/Daylight Box_Bottom.bmp",
        "assets/skybox/Daylight Box_Front.bmp",
        "assets/skybox/Daylight Box_Back.bmp"
        });

    Shader ourShader("assets/shaders/vertex_shader.glsl", "assets/shaders/fragment_shader.glsl");
    Shader shadowShader("assets/shaders/shadow_depth.vs.glsl", "assets/shaders/shadow_depth.fs.glsl");

   
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    gameCamera = new Camera((float)SCR_WIDTH, (float)SCR_HEIGHT);
    uiManager = new UIManager((float)SCR_WIDTH, (float)SCR_HEIGHT, loadTexture("assets/models/menu_prompt.png"));
    player = new Player();
    CloudManager cloudManager;
    WinZone winZone;

    Ground ground("assets/textures/grass_albedo.png", 120.0f, 100.0f, -0.01f);
    Model tableModel("assets/models/table.obj");
    Model ladderModel("assets/models/Ladder.fbx");
    Model tileModel("assets/models/glass_tile.obj");
    Model trampolineModel("assets/models/trampoline.obj");
    Model pillowModel("assets/models/pillow.obj");
    Model rampModel("assets/models/ramp.obj");
    Model flyoverModel("assets/models/flyover.obj");
    Model ballModel("assets/models/ball.obj");

    ladderTexture = loadTexture("assets/models/wood_ladder.jpg");

    // Drabina (23, 15, 27)
    myLadder = new Ladder(glm::vec3(23.0f, 15.0f, 27.0f), 10.0f, &ladderModel);

    myMaze = new Maze(glm::vec3(11.0f, 15.0f, 7.0f));
    glassBridge = new GlassBridge(glm::vec3(25.0f, 0.0f, 0.0f), 2.85f, &tileModel);
    bouncyTrampoline = new Trampoline(glm::vec3(41.0f, 0.0, 0.0f), 0.4f, 0.5f, 35.0f, &trampolineModel, glm::vec3(0.2f), glm::vec3(0.0f));

    
    wallCourse = new MovingWallCourse(
        glm::vec3(-45.0f, 23.6f, 28.0f),
        myMaze->wallTextureID,
        myMaze->floorTextureID,
        myMaze->cubeMesh
    );

    windyBridge = new WindyTileBridge(
        glm::vec3(-46.0f, 24.3f, 58.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        &tileModel,
        10,
        30,
        2.0f
    );

    windParticles = new WindParticles();
    windParticles->bridge = windyBridge;

    finalWinZone = new FinalWinZone(
        glm::vec3(0.0f, 24.3f, 125.0f),
        12.0f,
        12.0f,
        24.3f
    );

    ballManager = new BallManager(&ballModel);

    FlyoverBridge* myFlyover = new FlyoverBridge(
        
        glm::vec3(-9.0, 21.8f, 30.0f),

        // Obrót
        glm::vec3(0.0f, 0.0f, 0.0f),

        glm::vec3(5.0f, 0.4f, 0.8f),
        &flyoverModel
    );

    float winQuadVertices[] = {
        -1.0f,  1.0f, 0.0f,    0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,    0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,    1.0f, 0.0f,

         1.0f, -1.0f, 0.0f,    1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,    1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f,    0.0f, 1.0f
    };

    unsigned int winTexture = loadTexture("assets/models/win.png");

    unsigned int winVAO, winVBO;
    glGenVertexArrays(1, &winVAO);
    glGenBuffers(1, &winVBO);
    glBindVertexArray(winVAO);
    glBindBuffer(GL_ARRAY_BUFFER, winVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(winQuadVertices), &winQuadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);

    float titleTimer = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (wallCourse) wallCourse->Update(deltaTime);
        if (windyBridge) windyBridge->Update(deltaTime, eggPosition);

        if (windParticles) {
            windParticles->Update(deltaTime, glm::vec3(windyBridge->GetWindForce().x, 0, windyBridge->GetWindForce().y));
        }

        titleTimer += deltaTime;
        if (titleTimer >= 0.1f) { // Aktualizuj co 100ms
            std::string title;
            if (showWinMessage) {
                title = "=== GRATULACJE! UKONCZYLES PARKOUR! ===";
            }
            else {
                title = "Wspinaczka3D | X: " + std::to_string(eggPosition.x) +
                    " | Y: " + std::to_string(eggPosition.y) +
                    " | Z: " + std::to_string(eggPosition.z);
            }
            glfwSetWindowTitle(window, title.c_str());
            titleTimer = 0.0f;
        }

        if (needsReset) {
            //og nie usuwac
            eggPosition = glm::vec3(0.0f, 0.7f, 5.0f);
            //eggPosition = glm::vec3(-46.0f, 24.3f, 57.0f);
            //physics.Reset();
            maxFallHeight = 0.7f;
            maxFallHeight = eggPosition.y;
            crackCount = 0;
            player->UpdateCracks(0);
            if (glassBridge) glassBridge->Reset();
            if (finalWinZone) finalWinZone->playerHasWon = false;
            gameWon = false;
            restrictMovementToWinZone = false;
            needsReset = false;
        }

        if (currentState == GAME_STATE_PLAYING) previousEggPosition = eggPosition;
        processInput(window);

        if (currentState == GAME_STATE_PLAYING) {
            // Logika kulek
            if (ballManager) {
                ballManager->Update(deltaTime);
                if (ballManager->CheckCollision(eggPosition)) {
                    currentState = GAME_STATE_CRASHED;
                    crashStartTime = currentFrame;
                    crackCount = 3;
                    if (player) player->UpdateCracks(3);
                }
            }

            if (myLadder) physics.isClimbing = myLadder->CheckCollision(eggPosition);

            for (auto& t : tables) physics.CheckHorizontalCollision(eggPosition, previousEggPosition, t);
            physics.CheckHorizontalCollision(eggPosition, previousEggPosition, winZone.rampHorizontalBox);
            physics.CheckHorizontalCollision(eggPosition, previousEggPosition, midSafeZone);

            if (eggPosition.y > 15.0f) {
                // Kolizja z podłogą mostu (żeby nie przenikać przez bok podłogi)
                physics.CheckHorizontalCollision(eggPosition, previousEggPosition, ladderPillow);

                // Kolizja z BARIERKAMI (poręcze wzdłuż mostu)
                physics.CheckHorizontalCollision(eggPosition, previousEggPosition, barrierBack);
                physics.CheckHorizontalCollision(eggPosition, previousEggPosition, barrierFront);
            }

            if (myMaze) myMaze->checkCollision(eggPosition, previousEggPosition);
            if (bouncyTrampoline && eggPosition.y - 0.7f <= 1.0f &&
                glm::distance(glm::vec3(eggPosition.x, 0, eggPosition.z), bouncyTrampoline->position) < bouncyTrampoline->radius + 1.1f) {
                eggPosition.x = previousEggPosition.x;
                eggPosition.z = previousEggPosition.z;
            }

            if (wallCourse && wallCourse->CheckPlayer(eggPosition, 0.4f)) {
                crackCount++;
                player->UpdateCracks(crackCount);
                if (crackCount >= 3) {
                    currentState = GAME_STATE_CRASHED;
                    crashStartTime = (float)glfwGetTime();
                }
            }

            float oldY = eggPosition.y;
            physics.ApplyGravity(deltaTime, eggPosition.y);
            if (!physics.isClimbing && physics.velocityY >= 0.0f) {
                maxFallHeight = glm::max(eggPosition.y, maxFallHeight);
            }

            bool standing = physics.isClimbing;
            if (standing) {
                maxFallHeight = eggPosition.y;
                physics.velocityY = 0.0f;
            }

            for (auto& plat : platforms) {
                float dist = glm::distance(plat.startPos, plat.endPos);
                if (dist > 0.001f) {
                    plat.progress += (plat.speed * deltaTime / dist) * plat.direction;
                    if (plat.progress >= 1.0f || plat.progress <= 0.0f) plat.direction *= -1;
                    glm::vec3 curr = glm::mix(plat.startPos, plat.endPos, plat.progress = glm::clamp(plat.progress, 0.0f, 1.0f));
                    plat.currentOffset = curr - glm::mix(plat.startPos, plat.endPos, glm::clamp(plat.progress - (plat.speed * deltaTime * plat.direction / dist), 0.0f, 1.0f));
                    plat.hitbox.minX = curr.x - 1.5f;
                    plat.hitbox.maxX = curr.x + 1.5f;
                    plat.hitbox.minZ = curr.z - 1.5f;
                    plat.hitbox.maxZ = curr.z + 1.5f;
                    plat.hitbox.topY = curr.y;
                    if (!standing && Physics::IsInsideXZ(eggPosition, plat.hitbox) &&
                        oldY >= plat.hitbox.topY + 0.5f && eggPosition.y <= plat.hitbox.topY + 0.8f && physics.velocityY <= 0.0f) {
                        eggPosition.y = plat.hitbox.topY + 0.7f;
                        physics.velocityY = 0.0f;
                        physics.canJump = true;
                        standing = true;
                        maxFallHeight = eggPosition.y;
                        eggPosition += plat.currentOffset;
                    }
                }
            }

            if (!standing) {
                int stateInt = (int)currentState;
                standing = winZone.CheckRampCollision(oldY, eggPosition.y, physics.velocityY, 0.7f,
                    eggPosition, physics.canJump, maxFallHeight, 1.5f, 0.9f, crackCount, 3,
                    stateInt, (int)GAME_STATE_CRASHED, currentFrame, crashStartTime, updateCrackWrapper);
                currentState = (GameState)stateInt;
            }

            if (!standing && glassBridge && glassBridge->checkCollision(eggPosition, eggPosition.y, physics.velocityY, 0.7f)) {
                standing = true;
                physics.canJump = true;
                maxFallHeight = eggPosition.y;
            }

            if (!standing && bouncyTrampoline && bouncyTrampoline->checkCollision(eggPosition, eggPosition.y, physics.velocityY, 0.7f)) {
                physics.canJump = false;
                maxFallHeight = eggPosition.y;
            }

            if (!standing && Physics::IsInsideXZ(eggPosition, midSafeZone) &&
                oldY >= midSafeZone.topY + 0.6f && eggPosition.y <= midSafeZone.topY + 0.7f && physics.velocityY <= 0.0f) {
                maxFallHeight = midSafeZone.topY + 0.7f;
                eggPosition.y = maxFallHeight;
                physics.velocityY = 0.0f;
                physics.canJump = true;
                standing = true;
            }

            if (!standing && Physics::IsInsideXZ(eggPosition, ladderPillow) &&
                oldY >= ladderPillow.topY + 0.6f && eggPosition.y <= ladderPillow.topY + 0.7f && physics.velocityY <= 0.0f) {
                maxFallHeight = ladderPillow.topY + 0.7f;
                eggPosition.y = maxFallHeight;
                physics.velocityY = 0.0f;
                physics.canJump = true;
                standing = true;
            }

            // Obsługa stania na barierkach (jeśli ktoś na nie wskoczy)
            if (!standing && Physics::IsInsideXZ(eggPosition, barrierBack) &&
                oldY >= barrierBack.topY + 0.6f && eggPosition.y <= barrierBack.topY + 0.7f && physics.velocityY <= 0.0f) {
                maxFallHeight = barrierBack.topY + 0.7f;
                eggPosition.y = maxFallHeight;
                physics.velocityY = 0.0f;
                physics.canJump = true;
                standing = true;
            }

            if (!standing && Physics::IsInsideXZ(eggPosition, barrierFront) &&
                oldY >= barrierFront.topY + 0.6f && eggPosition.y <= barrierFront.topY + 0.7f && physics.velocityY <= 0.0f) {
                maxFallHeight = barrierFront.topY + 0.7f;
                eggPosition.y = maxFallHeight;
                physics.velocityY = 0.0f;
                physics.canJump = true;
                standing = true;
            }

            if (!standing && Physics::IsInsideXZ(eggPosition, mazeFloor) &&
                oldY >= mazeFloor.topY + 0.5f && eggPosition.y <= mazeFloor.topY + 0.8f && physics.velocityY <= 0.0f) {
                maxFallHeight = mazeFloor.topY + 0.7f;
                eggPosition.y = maxFallHeight;
                physics.velocityY = 0.0f;
                physics.canJump = true;
                standing = true;
            }

            if (!standing) {
                for (auto& t : tables) {
                    if (Physics::IsInsideXZ(eggPosition, t) && oldY >= t.topY + 0.5f &&
                        eggPosition.y <= t.topY + 0.8f && physics.velocityY <= 0.0f) {
                        float fall = glm::max(maxFallHeight - (t.topY + 0.7f), 0.0f);
                        if (fall >= 1.5f) {
                            currentState = GAME_STATE_CRASHED;
                            crashStartTime = currentFrame;
                            crackCount = 3;
                        }
                        else if (fall >= 0.9f) {
                            crackCount++;
                            if (crackCount >= 3) {
                                currentState = GAME_STATE_CRASHED;
                                crashStartTime = currentFrame;
                            }
                            player->UpdateCracks(crackCount);
                        }
                        maxFallHeight = t.topY + 0.7f;
                        eggPosition.y = maxFallHeight;
                        physics.velocityY = 0.0f;
                        physics.canJump = true;
                        standing = true;
                        break;
                    }
                }
            }

            if (!standing && eggPosition.y < 0.7f && oldY >= 0.6f) {
                float fall = glm::max(maxFallHeight - 0.7f, 0.0f);
                if (fall >= 1.5f) {
                    currentState = GAME_STATE_CRASHED;
                    crashStartTime = currentFrame;
                    crackCount = 3;
                }
                else if (fall >= 0.9f) {
                    crackCount++;
                    if (crackCount >= 3) {
                        currentState = GAME_STATE_CRASHED;
                        crashStartTime = currentFrame;
                    }
                    player->UpdateCracks(crackCount);
                }
                maxFallHeight = 0.7f;
                eggPosition.y = 0.7f;
                physics.velocityY = 0.0f;
                physics.canJump = true;
                standing = true;
            }

            cloudManager.Update(deltaTime);

            if (windyBridge) {
                bool onTiles = windyBridge->CheckCollision(eggPosition, deltaTime);
                if (onTiles && !standing) {
                    standing = true;
                    physics.canJump = true;
                    maxFallHeight = eggPosition.y;
                }

                if (windyBridge->IsPlayerFalling(eggPosition)) {
                    currentState = GAME_STATE_CRASHED;
                    crashStartTime = currentFrame;
                    crackCount = 3;
                    if (player) player->UpdateCracks(3);
                }

                if (eggPosition.z >= (58.0f + (30 - 3) * 2.0f)) {
                    eggPosition = glm::vec3(0.0f, 24.3f + 1.0f, 125.0f);
                }

                // Wygrana - aktywuj ograniczenie ruchu
                if (finalWinZone && finalWinZone->CheckPlayerInZone(eggPosition) && !gameWon) {
                    gameWon = true;
                    restrictMovementToWinZone = true;
                    showWinMessage = true;  // NOWA: aktywuj komunikat
                    winMessageDisplayTime = (float)glfwGetTime();
                    restrictMovementToWinZone = true;
                    // USTAW KOMUNIKAT
                    winMessage = "=== GRATULACJE! UKONCZYLES PARKOUR! ===";
                    winMessageDisplayTime = (float)glfwGetTime();
                }
            }

            if (!standing && wallCourse) {
                TableHitbox roadHB = wallCourse->GetRoadHitbox();
                if (Physics::IsInsideXZ(eggPosition, roadHB) &&
                    oldY >= roadHB.topY + 0.5f &&
                    eggPosition.y <= roadHB.topY + 0.8f &&
                    physics.velocityY <= 0.0f) {
                    maxFallHeight = roadHB.topY + 0.7f;
                    eggPosition.y = maxFallHeight;
                    physics.velocityY = 0.0f;
                    physics.canJump = true;
                    standing = true;
                }
            }
        }

        // Sprawdź czy gracz spadł z platformy wygranej
        if (restrictMovementToWinZone && finalWinZone && !finalWinZone->IsPositionInsideZone(eggPosition)) {
            currentState = GAME_STATE_CRASHED;
            crashStartTime = currentFrame;
            crackCount = 3;
            if (player) player->UpdateCracks(3);
        }

        // --- LIGHT SETUP ---
        glm::vec3 lightDir = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f));
        glm::vec3 lightColor = glm::vec3(1.0f);

        // --- LIGHT SPACE MATRIX (orthographic for directional light) ---
        glm::mat4 lightProjection = glm::ortho(-60.0f, 60.0f, -60.0f, 60.0f, 1.0f, 80.0f);
        glm::mat4 lightView = glm::lookAt(-lightDir * 30.0f, glm::vec3(0.0f), glm::vec3(0, 1, 0));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        // =========================
        // 1) SHADOW DEPTH PASS
        // =========================
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        shadowShader.use();
        shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        RenderScene(shadowShader,
            ground, tableModel, rampModel,
            winZone,
            myLadder,
            player, eggPosition, currentState,
            currentFrame, crashStartTime, CRASH_ANIMATION_DURATION,
            myMaze, glassBridge, bouncyTrampoline,
            platforms,
            myFlyover,
            ballManager,
            pillowModel,
            windyBridge,
            finalWinZone
        );

        if (bouncyTrampoline) bouncyTrampoline->Draw(shadowShader);
        if (glassBridge) glassBridge->Draw(shadowShader);

        // RYSUJ CIENIE DLA NOWYCH OBIEKTÓW:
        if (windyBridge) windyBridge->Draw(shadowShader);
        if (finalWinZone) finalWinZone->Draw(shadowShader, tableModel);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // =========================
        // 2) NORMAL RENDER PASS
        // =========================
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();
        ourShader.setInt("forceUpNormal", 0);
        ourShader.setInt("twoSided", 0);

        glm::mat4 view = (currentState == GAME_STATE_MENU) ?
            glm::lookAt(glm::vec3(0, 5, 15), glm::vec3(0, 2, 0), glm::vec3(0, 1, 0)) :
            gameCamera->GetViewMatrix(eggPosition);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
            (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 150.0f);

        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        // żeby nie było czarno jeśli shader mnoży przez objectColor
        ourShader.setVec4("objectColor", glm::vec4(1.0f));

        // --- LIGHT ---
        glm::vec3 viewPos;
        if (currentState == GAME_STATE_MENU) {
            viewPos = glm::vec3(0.0f, 5.0f, 15.0f);
        }
        else {
            viewPos = eggPosition - gameCamera->Front * gameCamera->Distance;
            viewPos.y += 1.5f;
            if (viewPos.y < 0.5f) viewPos.y = 0.5f;
        }

        ourShader.setVec3("viewPos", viewPos);
        ourShader.setVec3("lightDir", lightDir);
        ourShader.setVec3("lightColor", lightColor);
        ourShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        ourShader.setInt("shadowMap", 3);

        RenderScene(ourShader,
            ground, tableModel, rampModel,
            winZone,
            myLadder,
            player, eggPosition, currentState,
            currentFrame, crashStartTime, CRASH_ANIMATION_DURATION,
            myMaze, glassBridge, bouncyTrampoline,
            platforms,
            myFlyover,
            ballManager,
            pillowModel,
            windyBridge,
            finalWinZone
        );

        if (bouncyTrampoline) {
            ourShader.setInt("twoSided", 1);
            ourShader.setInt("forceUpNormal", 1);
            bouncyTrampoline->Draw(ourShader);
            ourShader.setInt("forceUpNormal", 0);
            ourShader.setInt("twoSided", 0);
        }

        if (glassBridge) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glassBridge->Draw(ourShader);
            glDisable(GL_BLEND);
        }
        skybox.Draw(view, projection);

        if (currentState != GAME_STATE_PLAYING && !gameWon) {
            uiManager->Draw();
        }

        if (gameWon) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_DEPTH_TEST);

            ourShader.use();
            ourShader.setMat4("projection", glm::mat4(1.0f));
            ourShader.setMat4("view", glm::mat4(1.0f));
            ourShader.setMat4("model", glm::mat4(1.0f));
            ourShader.setInt("useTexture", 1);

            glVertexAttrib3f(1, 0.0f, 0.0f, 1.0f);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, winTexture);
            ourShader.setInt("texture1", 0);

            glBindVertexArray(winVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Czyszczenie pamięci
    delete myFlyover;
    delete ballManager;
    delete wallCourse;
    delete windyBridge;
    delete finalWinZone;
    delete windParticles;

    glfwTerminate();
    return 0;
}
void RenderScene(Shader& shader,
    Ground& ground, Model& tableModel, Model& rampModel,
    WinZone& winZone,
    Ladder* myLadder,
    Player* player, glm::vec3 eggPosition, GameState currentState,
    float currentFrame, float crashStartTime, float CRASH_ANIMATION_DURATION,
    Maze* myMaze, GlassBridge* glassBridge, Trampoline* bouncyTrampoline,
    std::vector<MovingPlatform>& platforms,
    FlyoverBridge* myFlyover,
    BallManager* ballManager,
    Model& pillowModel,
    WindyTileBridge* windyBridge,
    FinalWinZone* finalWinZone) {

    shader.setInt("useTexture", 1);

    // floor 
    ground.Draw(shader);

    // tables
    for (auto& t : tables) {
        shader.setMat4("model", glm::translate(glm::mat4(1.0f),
            glm::vec3((t.minX + t.maxX) / 2, t.topY - 0.68f, (t.minZ + t.maxZ) / 2)));
        tableModel.Draw(shader);
    }

    // ramp/winzone
    winZone.Draw(shader, rampModel);

    // poduszka
    shader.setInt("useTexture", 1);
    shader.setMat4("model", glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(45.0f, 14.85f, 0.0f)), glm::vec3(4.0f, 1.0f, 4.0f)));
    pillowModel.Draw(shader);

    // ladder
    if (myLadder) myLadder->Draw(shader);

    // player
    player->Draw(shader, eggPosition, currentState == GAME_STATE_CRASHED,
        currentFrame - crashStartTime, CRASH_ANIMATION_DURATION);

    // maze
    if (myMaze) {
        shader.use();
        shader.setInt("useTexture", 1);
        myMaze->DrawFloor(shader);
        myMaze->Draw(shader);
    }

    if (wallCourse) wallCourse->Draw(shader);



    
    //if (glassBridge) {
    //    // UWAGA: w depth pass nie potrzebujesz blendingu; tu nie robimy enable/disable żeby nie mieszać
    //    glassBridge->Draw(shader);
    //}



    // moving platforms
    shader.setInt("useTexture", 1);
    for (auto& p : platforms) {
        shader.setMat4("model",
            glm::scale(glm::translate(glm::mat4(1.0f),
                glm::mix(p.startPos, p.endPos, p.progress) - glm::vec3(0, 0.68f, 0)),
                glm::vec3(2, 1, 2)));
        tableModel.Draw(shader);
    }

    // flyover
    if (myFlyover) myFlyover->Draw(shader);

    // kulki
    if (ballManager) ballManager->Draw(shader);

    if (windyBridge) windyBridge->Draw(shader);

    if (finalWinZone) finalWinZone->Draw(shader, tableModel);

    if (windParticles) windParticles->Draw(shader);
}

void processInput(GLFWwindow* w) {
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(w, true);

    if (gameWon && restrictMovementToWinZone) {
        if (glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS) {
            gameWon = false;
            restrictMovementToWinZone = false;
            needsReset = true;
            showWinMessage = false;
            return;
        }
    }

    if (gameWon && currentState == GAME_STATE_MENU) {
        if (glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS) {
            gameWon = false;
            restrictMovementToWinZone = false;
            needsReset = true;
            showWinMessage = false;
            return;
        }
    }

    if (currentState == GAME_STATE_MENU || currentState == GAME_STATE_CRASHED) {
        if (glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS) {
            if (!enterKeyPressed) {
                currentState = GAME_STATE_PLAYING;
                needsReset = true;
                showWinMessage = false;
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
        glm::vec3 lastSafePosition = eggPosition;

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

        // Ograniczenie ruchu do strefy wygranej
        if (restrictMovementToWinZone && finalWinZone) {
            if (!finalWinZone->IsPositionInsideZone(eggPosition)) {
                eggPosition = lastSafePosition;
            }
        }
    }
}

void framebuffer_size_callback(GLFWwindow* w, int width, int height) {
    glViewport(0, 0, width, height);
    if (uiManager) uiManager->UpdateProjection((float)width, (float)height);
}

void mouse_callback(GLFWwindow* w, double xpos, double ypos) {
    if (currentState == GAME_STATE_PLAYING) gameCamera->ProcessMouseMovement(xpos, ypos);
}

static unsigned int loadTexture(const char* path) {
    unsigned int id;
    glGenTextures(1, &id);
    int w, h, c;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* d = stbi_load(path, &w, &h, &c, 0);
    if (d) {
        GLenum f = (c == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, f, w, h, 0, f, GL_UNSIGNED_BYTE, d);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(d);
    return id;
}
