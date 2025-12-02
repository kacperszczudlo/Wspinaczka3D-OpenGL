#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <random> 
#include <cmath> 

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include "Model.h"
#include "Shader.h"


// Prototypy
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void updateCrackGeometry(int currentCrackCount);

// Globalne ustawienia i stan
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

enum GameState {
    GAME_STATE_MENU,
    GAME_STATE_PLAYING,
    GAME_STATE_CRASHED
};
GameState currentState = GAME_STATE_MENU;

Shader* uiShader = nullptr;
// Zmiana: Inicjalizacja globalnych uchwytów GL na 0, aby unikn¹æ ostrze¿eñ "niezainicjowana zmienna"
unsigned int uiVAO = 0, uiVBO = 0;
unsigned int menuTexture = 0;

glm::vec3 eggPosition = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 previousEggPosition = eggPosition;
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraDistance = 7.0f;

float lastX;
float lastY;
bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Fizyka
float velocityY = 0.0f;
bool canJump = true;
const float GRAVITY = -9.8f;
// ZMIANA: Zmniejszona si³a skoku, aby zwyk³y skok nie powodowa³ od razu rozbicia.
const float JUMP_FORCE = 4.0f;

// Pêkniêcia i uszkodzenia
float maxFallHeight = 0.0f;
// ZMIANA: Zwiêkszone progi obra¿eñ, aby skok o sile 4.0f (dystans upadku ok. 0.82m) by³ bezpieczny.
const float CRASH_HEIGHT_THRESHOLD = 1.5f; // Rozbicie powy¿ej 1.5m
const float CRACK_HEIGHT_THRESHOLD = 0.9f;  // Pêkniêcie powy¿ej 0.9m
int crackCount = 0;
const int MAX_CRACKS = 3;
std::vector<float> crackVertices;
unsigned int crackVAO = 0, crackVBO = 0;

// Animacja rozbicia
float crashStartTime = 0.0f;
const float CRASH_ANIMATION_DURATION = 0.7f;
glm::vec3 fragments[5] = { glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(-0.5f, 0.5f, -0.5f) };
const float EGG_HALF_HEIGHT = 0.7f;

struct TableHitbox {
    float minX, maxX;
    float minZ, maxZ;
    float topY;
};

TableHitbox table1 = { -0.8f, 0.8f, -0.8f, 0.8f, -0.02f };
TableHitbox table2 = { 2.12f, 3.85f, -0.84f, 0.86f, 0.75f };

static bool isInsideXZ(const glm::vec3& pos, const TableHitbox& t) {
    return pos.x > t.minX && pos.x < t.maxX &&
        pos.z > t.minZ && pos.z < t.maxZ;
}

static void checkHorizontalCollisionAndRevert(const glm::vec3& newPos, const glm::vec3& oldPos, const TableHitbox& t) {
    // Obliczamy Y, na którym spoczywa³by œrodek jajka (0.7f to EGG_HALF_HEIGHT)
    float restingCenterY = t.topY + EGG_HALF_HEIGHT;

    // 1. Zezwalamy na ruch horyzontalny, jeœli jajko jest wystarczaj¹co wysoko, aby spoczywaæ na blacie.
    if (newPos.y > restingCenterY - 0.01f) {
        return;
    }

    // 2. Je¿eli jajko jest ni¿ej (spada, skacze, jest na pod³odze) i wesz³o w XZ granicê sto³u,
    // to dosz³o do próby penetracji bocznej lub tunelowania. Cofamy ruch X i Z.
    if (isInsideXZ(newPos, t)) {
        eggPosition.x = oldPos.x;
        eggPosition.z = oldPos.z;
    }
}

// Chmury
struct CloudComponent { glm::vec3 offset; float scale; };
struct Cloud { glm::vec3 position = glm::vec3(0.0f); glm::vec3 velocity = glm::vec3(0.0f); std::vector<CloudComponent> components; };
std::vector<Cloud> clouds;
float cloudSpawnTimer = 0.0f;

static float randomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

static float localRandomFloat(std::mt19937& generator, float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(generator);
}

// === ZMODYFIKOWANA FUNKCJA GENERUJ¥CA PÊKNIÊCIA ===
void updateCrackGeometry(int currentCrackCount) {
    crackVertices.clear();
    std::mt19937 generator;

    for (int i = 0; i < currentCrackCount; ++i) {
        unsigned int initialSeed = 100 + i * 10;
        generator.seed(initialSeed);

        float theta = localRandomFloat(generator, 0.0f, glm::pi<float>());
        float phi = localRandomFloat(generator, 0.0f, 2.0f * glm::pi<float>());

        glm::vec3 currentPoint;
        currentPoint.x = 0.5f * std::sin(theta) * std::cos(phi);
        currentPoint.y = 0.7f * std::cos(theta);
        currentPoint.z = 0.5f * std::sin(theta) * std::sin(phi);

        int numSegments = 5 + i * 3;

        for (int j = 0; j < numSegments; ++j) {
            crackVertices.push_back(currentPoint.x);
            crackVertices.push_back(currentPoint.y);
            crackVertices.push_back(currentPoint.z);

            // Zwiêkszony krok dla bardziej dramatycznego i rozleg³ego pêkniêcia
            float thetaStep = localRandomFloat(generator, -0.3f, 0.3f);
            float phiStep = localRandomFloat(generator, -0.3f, 0.3f);

            theta = glm::clamp(theta + thetaStep, 0.0f, glm::pi<float>());
            phi += phiStep;

            glm::vec3 nextPoint;
            nextPoint.x = 0.5f * std::sin(theta) * std::cos(phi);
            nextPoint.y = 0.7f * std::cos(theta);
            nextPoint.z = 0.5f * std::sin(theta) * std::sin(phi);

            crackVertices.push_back(nextPoint.x);
            crackVertices.push_back(nextPoint.y);
            crackVertices.push_back(nextPoint.z);

            currentPoint = nextPoint;
        }
    }

    const void* dataPtr = crackVertices.empty() ? nullptr : crackVertices.data();
    size_t dataSize = crackVertices.size() * sizeof(float);

    glBindBuffer(GL_ARRAY_BUFFER, crackVBO);
    glBufferData(GL_ARRAY_BUFFER, dataSize, dataPtr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
// ===============================================


static void spawnCloud() {
    Cloud newCloud;
    newCloud.position = glm::vec3(randomFloat(-50.0f, 50.0f), randomFloat(10.0f, 20.0f), -70.0f);
    newCloud.velocity = glm::vec3(0.0f, 0.0f, randomFloat(1.5f, 3.0f));

    int numComponents = rand() % 4 + 3;
    for (int i = 0; i < numComponents; ++i) {
        CloudComponent component;
        component.offset = glm::vec3(randomFloat(-2.5f, 2.5f), randomFloat(-1.0f, 1.0f), randomFloat(-1.5f, 1.5f));
        component.scale = randomFloat(1.5f, 2.5f);
        newCloud.components.push_back(component);
    }
    clouds.push_back(newCloud);
}


static unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = 0;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        if (format != 0) {
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        else {
            std::cout << "Blad ladowania tekstury: Nieznana liczba komponentow (" << nrComponents << ")" << std::endl;
        }
    }
    else {
        std::cout << "Blad ladowania tekstury: " << path << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}


int main() {
    // Inicjalizacja GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    SCR_WIDTH = mode->width;
    SCR_HEIGHT = mode->height;
    lastX = SCR_WIDTH / 2.0f;
    lastY = SCR_HEIGHT / 2.0f;

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Wspinaczka3D", NULL, NULL);
    if (window == NULL) {
        std::cout << "Blad tworzenia okna GLFW!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Blad inicjalizacji GLAD!" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    srand(static_cast<unsigned int>(time(0)));

    Shader ourShader("vertex_shader.glsl", "fragment_shader.glsl");

    // === INICJALIZACJA UI ===
    uiShader = new Shader("ui_vertex.glsl", "ui_fragment.glsl");
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
    uiShader->use();
    uiShader->setMat4("projection", projection);
    uiShader->setInt("uiTexture", 0);

    float width_ui = 500.0f;
    float height_ui = 500.0f;
    float x_pos = (SCR_WIDTH / 2.0f) - (width_ui / 2.0f);
    float y_pos = (SCR_HEIGHT / 2.0f) - (height_ui / 2.0f);

    float uiVertices[] = {
        x_pos,       y_pos + height_ui,   0.0f, 1.0f,
        x_pos,       y_pos,               0.0f, 0.0f,
        x_pos + width_ui, y_pos,             1.0f, 0.0f,
        x_pos,       y_pos + height_ui,   0.0f, 1.0f,
        x_pos + width_ui, y_pos,             1.0f, 0.0f,
        x_pos + width_ui, y_pos + height_ui,   1.0f, 1.0f
    };

    glGenVertexArrays(1, &uiVAO);
    glGenBuffers(1, &uiVBO);
    glBindVertexArray(uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uiVertices), uiVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    menuTexture = loadTexture("menu_prompt.png");

    // === Konfiguracja Geometrii (Jajko) ===
    std::vector<float> eggVertices;
    std::vector<unsigned int> eggIndices;
    int eggSegments = 40;
    for (int i = 0; i <= eggSegments; ++i) {
        float theta = glm::pi<float>() * i / eggSegments;
        for (int j = 0; j <= eggSegments; ++j) {
            float phi = 2 * glm::pi<float>() * j / eggSegments;
            eggVertices.push_back(0.5f * std::sin(theta) * std::cos(phi));
            eggVertices.push_back(0.7f * std::cos(theta));
            eggVertices.push_back(0.5f * std::sin(theta) * std::sin(phi));
        }
    }
    for (int i = 0; i < eggSegments; ++i) {
        for (int j = 0; j < eggSegments; ++j) {
            int first = (i * (eggSegments + 1)) + j;
            int second = first + eggSegments + 1;
            eggIndices.push_back(first);
            eggIndices.push_back(second);
            eggIndices.push_back(first + 1);
            eggIndices.push_back(second);
            eggIndices.push_back(second + 1);
            eggIndices.push_back(first + 1);
        }
    }
    unsigned int eggVAO, eggVBO, eggEBO;
    glGenVertexArrays(1, &eggVAO);
    glGenBuffers(1, &eggVBO);
    glGenBuffers(1, &eggEBO);
    glBindVertexArray(eggVAO);
    glBindBuffer(GL_ARRAY_BUFFER, eggVBO);
    glBufferData(GL_ARRAY_BUFFER, eggVertices.size() * sizeof(float), eggVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eggEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, eggIndices.size() * sizeof(unsigned int), eggIndices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // === Konfiguracja Geometrii (Pod³oga, Szeœcian, Pêkniêcia, Chmury) ===
    float floorVertices[] = { 50.0f, -0.7f, 50.0f, -50.0f, -0.7f, 50.0f, -50.0f, -0.7f, -50.0f, 50.0f, -0.7f, 50.0f, -50.0f, -0.7f, -50.0f, 50.0f, -0.7f, -50.0f };
    unsigned int floorVAO, floorVBO;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    float cubeVertices[] = { -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f };
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &crackVAO);
    glGenBuffers(1, &crackVBO);
    glBindVertexArray(crackVAO);
    glBindBuffer(GL_ARRAY_BUFFER, crackVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    int segments = 20;
    for (int i = 0; i <= segments; ++i) {
        float theta = glm::pi<float>() * i / segments;
        for (int j = 0; j <= segments; ++j) {
            float phi = 2 * glm::pi<float>() * j / segments;
            sphereVertices.push_back(std::cos(phi) * std::sin(theta));
            sphereVertices.push_back(std::cos(theta));
            sphereVertices.push_back(std::sin(phi) * std::sin(theta));
        }
    }
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < segments; ++j) {
            int first = (i * (segments + 1)) + j;
            int second = first + segments + 1;
            sphereIndices.push_back(first);
            sphereIndices.push_back(second);
            sphereIndices.push_back(first + 1);
            sphereIndices.push_back(second);
            sphereIndices.push_back(second + 1);
            sphereIndices.push_back(first + 1);
        }
    }
    unsigned int sphereVAO, sphereVBO, sphereEBO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);
    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // === £ADOWANIE MODELU 3D ===
    Model tableModel("models/table.obj");

    // Pêtla renderowania
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Zapis pozycji przed ruchem X/Z
        previousEggPosition = eggPosition;

        processInput(window);

        // === KOREKCJA KOLIZJI HORYZONTALNEJ ===
        if (currentState == GAME_STATE_PLAYING) {
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table1);
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table2);
        }

        // === FIZYKA I LOGIKA GRY ===
        if (currentState == GAME_STATE_PLAYING)
        {
            float oldY = eggPosition.y;

            if (velocityY >= 0.0f) {
                // maxFallHeight powinien œledziæ najwy¿szy Y œrodka osi¹gniêty od ostatniego l¹dowania
                maxFallHeight = eggPosition.y > maxFallHeight ? eggPosition.y : maxFallHeight;
            }

            // Grawitacja
            velocityY += GRAVITY * deltaTime;
            eggPosition.y += velocityY * deltaTime;

            bool standingOnSomething = false;

            auto checkLandingAndDamage = [&](float landingY) {

                // Dystans upadku to ró¿nica miêdzy najwy¿sz¹ osi¹gniêt¹ wysokoœci¹ (œrodek)
                // a wysokoœci¹ œrodka po wyl¹dowaniu (landingY). 
                float fallDistance = maxFallHeight - landingY;

                // Upewniamy siê, ¿e nie ma obra¿eñ, jeœli l¹dujemy na wy¿szej powierzchni
                if (fallDistance < 0.0f) {
                    fallDistance = 0.0f;
                }

                if (fallDistance >= CRASH_HEIGHT_THRESHOLD) {
                    currentState = GAME_STATE_CRASHED;
                    crashStartTime = currentFrame;
                    crackCount = MAX_CRACKS;
                }
                else if (fallDistance >= CRACK_HEIGHT_THRESHOLD) {
                    crackCount = glm::min(crackCount + 1, MAX_CRACKS);
                    if (crackCount >= MAX_CRACKS) {
                        currentState = GAME_STATE_CRASHED;
                        crashStartTime = currentFrame;
                    }
                    updateCrackGeometry(crackCount);
                }
                // Resetowanie wysokoœci maksymalnego upadku do aktualnej wysokoœci spoczynku
                maxFallHeight = landingY;
                };

            // Kolizje ze sto³ami (Wertykalne)
            if (isInsideXZ(eggPosition, table1)) {
                float desiredY1 = table1.topY + EGG_HALF_HEIGHT;
                if (oldY >= desiredY1 - 0.001f && eggPosition.y <= desiredY1 && velocityY <= 0.0f) {
                    checkLandingAndDamage(desiredY1);
                    eggPosition.y = desiredY1;
                    velocityY = 0.0f;
                    canJump = true;
                    standingOnSomething = true;
                }
            }

            if (!standingOnSomething && isInsideXZ(eggPosition, table2)) {
                float desiredY2 = table2.topY + EGG_HALF_HEIGHT;
                if (oldY >= desiredY2 - 0.001f && eggPosition.y <= desiredY2 && velocityY <= 0.0f) {
                    checkLandingAndDamage(desiredY2);
                    eggPosition.y = desiredY2;
                    velocityY = 0.0f;
                    canJump = true;
                    standingOnSomething = true;
                }
            }

            // Kolizja z ziemi¹
            if (!standingOnSomething) {
                if (eggPosition.y < 0.0f) {
                    checkLandingAndDamage(0.0f);
                    eggPosition.y = 0.0f;
                    velocityY = 0.0f;
                    canJump = true;
                }
            }

            // Chmury
            cloudSpawnTimer += deltaTime;
            if (cloudSpawnTimer > 2.0f && clouds.size() < 15) {
                spawnCloud();
                cloudSpawnTimer = 0.0f;
            }

            for (auto it = clouds.begin(); it != clouds.end(); ) {
                it->position += it->velocity * deltaTime;
                if (it->position.z > 50.0f) {
                    it = clouds.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
        // === KONIEC LOGIKI GRY ===


        // === RENDEROWANIE ===
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 150.0f);
        glm::mat4 view;

        if (currentState == GAME_STATE_PLAYING || currentState == GAME_STATE_CRASHED) {
            glm::vec3 cameraPos = eggPosition - cameraFront * cameraDistance;
            const float minCameraHeight = -0.5f;
            if (cameraPos.y < minCameraHeight) cameraPos.y = minCameraHeight;
            view = glm::lookAt(cameraPos, eggPosition, cameraUp);
        }
        else {
            glm::vec3 menuCameraPos = glm::vec3(0.0f, 5.0f, 15.0f);
            view = glm::lookAt(menuCameraPos, glm::vec3(0.0f, 2.0f, 0.0f), cameraUp);
        }

        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // Pod³oga
        glBindVertexArray(floorVAO);
        ourShader.setInt("useTexture", 0);
        ourShader.setMat4("model", glm::mat4(1.0f));
        ourShader.setVec3("objectColor", glm::vec3(0.2f, 0.6f, 0.1f));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Sto³y
        ourShader.setInt("useTexture", 1);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -0.75f, 0.0f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(3.0f, 0.0f, 0.0f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);


        // Jajko / Fragmenty
        if (currentState != GAME_STATE_CRASHED) {
            glBindVertexArray(eggVAO);
            ourShader.setInt("useTexture", 0);
            model = glm::translate(glm::mat4(1.0f), eggPosition);
            ourShader.setMat4("model", model);
            ourShader.setVec3("objectColor", glm::vec3(1.0f, 0.9f, 0.7f));
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(eggIndices.size()), GL_UNSIGNED_INT, 0);

            // Pêkniêcia
            if (crackCount > 0 && !crackVertices.empty()) {
                glBindVertexArray(crackVAO);
                ourShader.setMat4("model", model);
                ourShader.setVec3("objectColor", glm::vec3(0.0f, 0.0f, 0.0f));
                glLineWidth(3.0f);
                glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(crackVertices.size() / 3));
                glLineWidth(1.0f);
                glBindVertexArray(0);
            }
        }
        else { // Animacja rozbicia
            float timeElapsed = currentFrame - crashStartTime;
            float ratio = timeElapsed / CRASH_ANIMATION_DURATION;

            if (ratio < 1.0f) {
                glBindVertexArray(cubeVAO);
                ourShader.setInt("useTexture", 0);
                ourShader.setVec3("objectColor", glm::vec3(1.0f, 0.9f, 0.7f));

                for (int i = 0; i < 5; ++i) {
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, eggPosition);
                    float explosionForce = 3.0f * (1.0f - ratio);
                    glm::vec3 fragmentOffset = fragments[i] * explosionForce * timeElapsed;
                    fragmentOffset.y -= 5.0f * timeElapsed * timeElapsed;
                    model = glm::translate(model, fragmentOffset);
                    float scaleFactor = glm::mix(0.1f, 0.0f, ratio);
                    model = glm::scale(model, glm::vec3(scaleFactor));
                    ourShader.setMat4("model", model);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
                glBindVertexArray(0);
            }
        }

        // Chmury
        glBindVertexArray(sphereVAO);
        ourShader.setInt("useTexture", 0);
        ourShader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));
        for (const auto& cloud : clouds) {
            for (const auto& component : cloud.components) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, cloud.position);
                model = glm::translate(model, component.offset);
                model = glm::scale(model, glm::vec3(component.scale));
                ourShader.setMat4("model", model);
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sphereIndices.size()), GL_UNSIGNED_INT, 0);
            }
        }

        // Rysowanie UI
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        if (currentState == GAME_STATE_MENU || currentState == GAME_STATE_CRASHED) {
            uiShader->use();
            glBindVertexArray(uiVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, menuTexture);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);

            if (currentState == GAME_STATE_CRASHED) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Zwolnienie zasobów
    glDeleteVertexArrays(1, &eggVAO);
    glDeleteBuffers(1, &eggVBO);
    glDeleteBuffers(1, &eggEBO);
    glDeleteVertexArrays(1, &floorVAO);
    glDeleteBuffers(1, &floorVBO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &crackVAO);
    glDeleteBuffers(1, &crackVBO);
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
    glDeleteVertexArrays(1, &uiVAO);
    glDeleteBuffers(1, &uiVBO);
    glDeleteTextures(1, &menuTexture);
    delete uiShader;

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (currentState == GAME_STATE_MENU) {
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            currentState = GAME_STATE_PLAYING;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true;
        }
    }
    else if (currentState == GAME_STATE_CRASHED) {
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            currentState = GAME_STATE_MENU;
            eggPosition = glm::vec3(0.0f, 0.0f, 5.0f);
            velocityY = 0.0f;
            canJump = true;
            maxFallHeight = 0.0f;
            crackCount = 0;
            updateCrackGeometry(0);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    else if (currentState == GAME_STATE_PLAYING) {
        const float WALK_SPEED = 2.5f;
        const float SPRINT_SPEED = 5.0f;
        float currentSpeed = WALK_SPEED;

        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            currentSpeed = SPRINT_SPEED;
        }

        glm::vec3 forward = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
        glm::vec3 right = glm::normalize(glm::cross(forward, cameraUp));

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) eggPosition += forward * currentSpeed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) eggPosition -= forward * currentSpeed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) eggPosition -= right * currentSpeed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) eggPosition += right * currentSpeed * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && canJump) {
            velocityY = JUMP_FORCE;
            canJump = false;
        }
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
    if (uiShader) {
        glm::mat4 projection = glm::ortho(0.0f, (float)width, 0.0f, (float)height);
        uiShader->use();
        uiShader->setMat4("projection", projection);
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (currentState != GAME_STATE_PLAYING) return;

    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 direction;
    direction.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    direction.y = std::sin(glm::radians(pitch));
    direction.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}