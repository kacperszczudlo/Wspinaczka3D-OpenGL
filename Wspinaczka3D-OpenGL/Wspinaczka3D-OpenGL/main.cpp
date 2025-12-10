#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <random> 
#include <cmath> 

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Biblioteki matematyczne GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

// Nasze klasy pomocnicze
#include "Model.h"
#include "Shader.h"

// ==========================================
// 1. STRUKTURY DANYCH
// ==========================================

// Struktura opisuj¹ca p³aski stó³ (prostopad³oœcian)
struct TableHitbox {
    float minX, maxX; // Zakres w osi X
    float minZ, maxZ; // Zakres w osi Z
    float topY;       // Wysokoœæ blatu (powierzchnia, na której siê stoi)
};

// Struktura opisuj¹ca rampê (pochy³¹ powierzchniê)
struct RampHitbox {
    float minX, maxX;
    float minZ, maxZ;
    float startY, endY; // Wysokoœæ pocz¹tkowa i koñcowa rampy
    float lengthZ;
};

// Struktury dla systemu chmur (ozdoba t³a)
struct CloudComponent { glm::vec3 offset; float scale; };
struct Cloud {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    std::vector<CloudComponent> components;
};

// ==========================================
// 2. PROTOTYPY FUNKCJI (Deklaracje)
// ==========================================
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void updateCrackGeometry(int currentCrackCount);

// G³ówna funkcja kolizji rampy - oblicza wysokoœæ gracza na pochy³oœci
bool checkRampCollision(float oldY, float newY, float& velocityY, float EGG_HALF_HEIGHT, float currentEggX, float& eggPositionY, bool& canJump, float& maxFallHeight, const float CRASH_HEIGHT_THRESHOLD, const float CRACK_HEIGHT_THRESHOLD, int& crackCount, const int MAX_CRACKS, float currentFrame, float& crashStartTime, const RampHitbox& ramp, void (*updateCrackGeom)(int));

// Funkcje pomocnicze do kolizji prostok¹tnych
static bool isInsideXZ(const glm::vec3& pos, const TableHitbox& t);
static void checkHorizontalCollisionAndRevert(const glm::vec3& newPos, const glm::vec3& oldPos, const TableHitbox& t);

// ==========================================
// 3. ZMIENNE GLOBALNE
// ==========================================
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

// Stany gry - zarz¹dzanie menu i rozgrywk¹
enum GameState {
    GAME_STATE_MENU,
    GAME_STATE_PLAYING,
    GAME_STATE_CRASHED
};
GameState currentState = GAME_STATE_MENU;
bool needsReset = false; // Flaga informuj¹ca, ¿e trzeba zresetowaæ pozycjê gracza

// Zmienne do obs³ugi UI (Menu)
Shader* uiShader = nullptr;
unsigned int uiVAO = 0, uiVBO = 0;
unsigned int menuTexture = 0;

// === KAMERA I GRACZ ===
glm::vec3 eggPosition = glm::vec3(0.0f, 0.7f, 5.0f); // Startowa pozycja gracza
glm::vec3 previousEggPosition = eggPosition;         // Pozycja z poprzedniej klatki (do kolizji)
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraDistance = 7.0f;

// Obs³uga myszy
float lastX;
float lastY;
bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;

// Czas klatki (dla p³ynnego ruchu niezale¿nie od FPS)
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// === FIZYKA ===
float velocityY = 0.0f; // Prêdkoœæ wertykalna (skok/upadek)
bool canJump = true;    // Czy gracz stoi na ziemi?
const float GRAVITY = -9.8f;
const float JUMP_FORCE = 4.0f;

// === SYSTEM USZKODZEÑ ===
float maxFallHeight = 0.0f;       // Najwy¿szy punkt osi¹gniêty podczas skoku
const float CRASH_HEIGHT_THRESHOLD = 1.5f; // Wysokoœæ upadku powoduj¹ca œmieræ
const float CRACK_HEIGHT_THRESHOLD = 0.9f; // Wysokoœæ upadku powoduj¹ca pêkniêcie
int crackCount = 0;
const int MAX_CRACKS = 3;
std::vector<float> crackVertices; // Wierzcho³ki rysowanych pêkniêæ
unsigned int crackVAO = 0, crackVBO = 0;

// Animacja rozbicia jajka
float crashStartTime = 0.0f;
const float CRASH_ANIMATION_DURATION = 0.7f;
// Fragmenty skorupki do animacji wybuchu
glm::vec3 fragments[5] = { glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(-0.5f, 0.5f, -0.5f) };
const float EGG_HALF_HEIGHT = 0.7f; // Po³owa wysokoœci jajka (wa¿ne do kolizji)

// === DEFINICJE OBIEKTÓW (HITBOXY) ===
// Stó³ 1 (Wymiary 1.6x1.6)
TableHitbox table1 = { -2.8f, -1.2f, -0.8f, 0.8f, 0.68f }; // Œrodek X=-2.0f

// Stó³ 2 (Wymiary 1.6x1.6)
TableHitbox table2 = { -0.8f, 0.8f, -0.8f, 0.8f, 1.45f }; // Œrodek X=0.0f

// Stó³ 3 (Wymiary 1.6x1.6)
TableHitbox table3 = { 1.7f, 3.3f, -0.8f, 0.8f, 1.45f }; // Œrodek X=2.5f

// Stó³ 4 (Lewo, Wymiary 1.6x1.6)
TableHitbox table4 = { 4.2f, 5.8f, -2.3f, -0.7f, 1.45f }; // Œrodek X=5.0f, Z=-1.5f

// Stó³ 5 (Prawo, Wymiary 1.6x1.6)
TableHitbox table5 = { 6.7f, 8.3f, 0.7f, 2.3f, 1.45f }; // Œrodek X=7.5f, Z=1.5f

// Stó³ 6 (Lewo, Wymiary 1.6x1.6)
TableHitbox table6 = { 9.2f, 10.8f, -2.3f, -0.7f, 1.45f }; // Œrodek X=10.0f, Z=-1.5f

// Stó³ 7 (Prawo, Wymiary 1.6x1.6)
TableHitbox table7 = { 11.7f, 13.3f, 0.7f, 2.3f, 1.45f }; // Œrodek X=12.5f, Z=1.5f

// Stó³ 8 (Lewo, Wymiary 1.6x1.6)
TableHitbox table8 = { 14.2f, 15.8f, -2.3f, -0.7f, 1.45f }; // Œrodek X=15.0f, Z=-1.5f

// Stó³ 9 (Podejœcie, Wymiary 1.6x1.6)
TableHitbox table9 = { 17.2f, 18.8f, -0.8f, 0.8f, 1.45f }; // Œrodek X=18.0f, Z=0.0f


// Rampa (Przesuniêta na X=20.0f. D³ugoœæ 4m do X=24.0f)
RampHitbox ramp = { 20.0f, 24.0f, -1.2f, 1.2f, 2.05f, 2.85f, 4.0f };
// Hitbox boczny rampy (¿eby nie przenikaæ przez jej boki na dole)
TableHitbox ramp_horizontal_box = { 20.0f, 24.0f, -0.9f, 0.9f, 2.05f };

// Chmury
std::vector<Cloud> clouds;
float cloudSpawnTimer = 0.0f;

// ==========================================
// 4. IMPLEMENTACJA FUNKCJI POMOCNICZYCH
// ==========================================

// Sprawdza, czy punkt jest wewn¹trz prostok¹ta (tylko osie X i Z)
static bool isInsideXZ(const glm::vec3& pos, const TableHitbox& t) {
    return pos.x > t.minX && pos.x < t.maxX &&
        pos.z > t.minZ && pos.z < t.maxZ;
}

// Sprawdza kolizjê "boczn¹" (chodzenie w œciany) i cofa ruch, jeœli nast¹pi³a
static void checkHorizontalCollisionAndRevert(const glm::vec3& newPos, const glm::vec3& oldPos, const TableHitbox& t) {
    // Obliczamy poziom, na którym spoczywa³by œrodek jajka stoj¹c na blacie
    float restingCenterY = t.topY + EGG_HALF_HEIGHT;

    // Jeœli jajko jest WY¯EJ ni¿ blat (minus margines tolerancji), to znaczy, ¿e wchodzimy NA stó³, a nie W stó³.
    // 0.10f to tolerancja u³atwiaj¹ca wskakiwanie.
    if (newPos.y > restingCenterY - 0.10f) {
        return; // Brak kolizji bocznej, jesteœmy nad obiektem
    }

    // Jeœli jesteœmy ni¿ej i weszliœmy w obrys XZ sto³u -> cofamy ruch X i Z
    if (isInsideXZ(newPos, t)) {
        eggPosition.x = oldPos.x;
        eggPosition.z = oldPos.z;
    }
}

// Generatory liczb losowych
static float randomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

static float localRandomFloat(std::mt19937& generator, float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(generator);
}

// £adowanie tekstur z pliku za pomoc¹ biblioteki stb_image
static unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true); // Odwracamy teksturê (OpenGL ma Y=0 na dole)
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
            // Ustawienia powtarzania i filtrowania tekstury
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
    }
    else {
        std::cout << "Blad ladowania tekstury: " << path << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}

// Tworzenie nowej chmury w losowym miejscu
static void spawnCloud() {
    Cloud newCloud;
    newCloud.position = glm::vec3(randomFloat(-50.0f, 50.0f), randomFloat(10.0f, 20.0f), -70.0f);
    newCloud.velocity = glm::vec3(0.0f, 0.0f, randomFloat(1.5f, 3.0f));

    // Ka¿da chmura sk³ada siê z kilku sfer (komponentów)
    int numComponents = rand() % 4 + 3;
    for (int i = 0; i < numComponents; ++i) {
        CloudComponent component;
        component.offset = glm::vec3(randomFloat(-2.5f, 2.5f), randomFloat(-1.0f, 1.0f), randomFloat(-1.5f, 1.5f));
        component.scale = randomFloat(1.5f, 2.5f);
        newCloud.components.push_back(component);
    }
    clouds.push_back(newCloud);
}

// Proceduralne generowanie pêkniêæ na skorupce jajka
void updateCrackGeometry(int currentCrackCount) {
    crackVertices.clear();
    std::mt19937 generator;

    for (int i = 0; i < currentCrackCount; ++i) {
        unsigned int initialSeed = 100 + i * 10;
        generator.seed(initialSeed);

        // Losowy punkt startowy na elipsoidzie
        float theta = localRandomFloat(generator, 0.0f, glm::pi<float>());
        float phi = localRandomFloat(generator, 0.0f, 2.0f * glm::pi<float>());

        glm::vec3 currentPoint;
        currentPoint.x = 0.5f * std::sin(theta) * std::cos(phi);
        currentPoint.y = 0.7f * std::cos(theta);
        currentPoint.z = 0.5f * std::sin(theta) * std::sin(phi);

        int numSegments = 5 + i * 3; // Im wiêcej pêkniêæ, tym d³u¿sze

        // Generowanie linii pêkniêcia (b³¹dzenie losowe)
        for (int j = 0; j < numSegments; ++j) {
            crackVertices.push_back(currentPoint.x);
            crackVertices.push_back(currentPoint.y);
            crackVertices.push_back(currentPoint.z);

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

    // Przes³anie danych do bufora graficznego (VBO)
    const void* dataPtr = crackVertices.empty() ? nullptr : crackVertices.data();
    size_t dataSize = crackVertices.size() * sizeof(float);

    glBindBuffer(GL_ARRAY_BUFFER, crackVBO);
    glBufferData(GL_ARRAY_BUFFER, dataSize, dataPtr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// === LOGIKA KOLIZJI Z RAMP¥ ===
// Sprawdza, czy gracz stoi na pochy³ej powierzchni.
// Oblicza wysokoœæ pod³o¿a na podstawie pozycji X gracza.
bool checkRampCollision(float oldY, float newY, float& velocityY, float EGG_HALF_HEIGHT, float currentEggX, float& eggPositionY, bool& canJump, float& maxFallHeight, const float CRASH_HEIGHT_THRESHOLD, const float CRACK_HEIGHT_THRESHOLD, int& crackCount, const int MAX_CRACKS, float currentFrame, float& crashStartTime, const RampHitbox& ramp, void (*updateCrackGeom)(int)) {
    float distanceX = ramp.maxX - ramp.minX; // D³ugoœæ rampy
    float heightChange = ramp.endY - ramp.startY; // Ró¿nica wysokoœci

    // Obliczamy procent, jak daleko gracz jest na rampie (0.0 = pocz¹tek, 1.0 = koniec)
    float rampRatio = glm::clamp((currentEggX - ramp.minX) / distanceX, 0.0f, 1.0f);

    // Wyliczamy wysokoœæ pod³ogi w tym konkretnym punkcie X
    float surfaceY = ramp.startY + heightChange * rampRatio;
    float desiredCenterY = surfaceY + EGG_HALF_HEIGHT; // Tu powinien byæ œrodek jajka

    // Sprawdzamy, czy gracz dotyka powierzchni rampy (z tolerancj¹ 0.5f na wejœcie)
    if (oldY >= desiredCenterY - 0.5f && newY <= desiredCenterY && velocityY <= 0.0f) {

        // Obliczamy obra¿enia od upadku
        float fallDistance = maxFallHeight - desiredCenterY;
        if (fallDistance < 0.0f) { fallDistance = 0.0f; }

        if (fallDistance >= CRASH_HEIGHT_THRESHOLD) {
            currentState = GAME_STATE_CRASHED; // Œmieræ
            crashStartTime = currentFrame;
            crackCount = MAX_CRACKS;
        }
        else if (fallDistance >= CRACK_HEIGHT_THRESHOLD) {
            crackCount = glm::min(crackCount + 1, MAX_CRACKS); // Pêkniêcie
            if (crackCount >= MAX_CRACKS) {
                currentState = GAME_STATE_CRASHED;
            }
            updateCrackGeom(crackCount);
        }

        // Reset fizyki (stajemy na ziemi)
        maxFallHeight = desiredCenterY;
        eggPositionY = desiredCenterY; // "Przyklej" gracza do rampy
        velocityY = 0.0f;
        canJump = true;

        return true; // Kolizja wykryta
    }
    return false; // Brak kolizji
}

// ==========================================
// 5. FUNKCJA MAIN
// ==========================================
int main() {
    // Inicjalizacja GLFW (systemu okienkowego)
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Pobranie rozdzielczoœci monitora
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    SCR_WIDTH = mode->width;
    SCR_HEIGHT = mode->height;
    lastX = SCR_WIDTH / 2.0f;
    lastY = SCR_HEIGHT / 2.0f;

    // Tworzenie okna
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Wspinaczka3D", NULL, NULL);
    if (window == NULL) {
        std::cout << "Blad tworzenia okna GLFW!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Kursor widoczny w menu

    // Inicjalizacja GLAD (wskaŸniki do funkcji OpenGL)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Blad inicjalizacji GLAD!" << std::endl;
        return -1;
    }

    // Ustawienia globalne OpenGL
    glEnable(GL_DEPTH_TEST); // W³¹czenie bufora g³êbokoœci (Z-buffer)
    glDisable(GL_CULL_FACE); // Wy³¹czenie ukrywania tylnych œcianek (widocznoœæ rampy od spodu)
    srand(static_cast<unsigned int>(time(0)));

    // Kompilacja Shaderów (z plików .glsl)
    Shader ourShader("vertex_shader.glsl", "fragment_shader.glsl");

    // === KONFIGURACJA UI (MENU) ===
    uiShader = new Shader("ui_vertex.glsl", "ui_fragment.glsl");
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
    uiShader->use();
    uiShader->setMat4("projection", projection);
    uiShader->setInt("uiTexture", 0);

    // Wspó³rzêdne prostok¹ta menu (wycentrowanego)
    float width_ui = 500.0f;
    float height_ui = 500.0f;
    float x_pos = (SCR_WIDTH / 2.0f) - (width_ui / 2.0f);
    float y_pos = (SCR_HEIGHT / 2.0f) - (height_ui / 2.0f);
    float uiVertices[] = {
        // Pozycja (x, y)      // TexCoord (u, v)
        x_pos,       y_pos + height_ui,   0.0f, 1.0f,
        x_pos,       y_pos,               0.0f, 0.0f,
        x_pos + width_ui, y_pos,             1.0f, 0.0f,
        x_pos,       y_pos + height_ui,   0.0f, 1.0f,
        x_pos + width_ui, y_pos,             1.0f, 0.0f,
        x_pos + width_ui, y_pos + height_ui,   1.0f, 1.0f
    };

    // Bufory dla UI
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

    // === GEOMETRIA JAJKA (Gracza) ===
    // Generowanie sfery/elipsoidy
    std::vector<float> eggVertices;
    std::vector<unsigned int> eggIndices;
    int eggSegments = 40;
    for (int i = 0; i <= eggSegments; ++i) {
        float theta = glm::pi<float>() * i / eggSegments;
        for (int j = 0; j <= eggSegments; ++j) {
            float phi = 2 * glm::pi<float>() * j / eggSegments;
            // Równania parametryczne elipsoidy
            eggVertices.push_back(0.5f * std::sin(theta) * std::cos(phi)); // X (szerokoœæ)
            eggVertices.push_back(0.7f * std::cos(theta));               // Y (wysokoœæ)
            eggVertices.push_back(0.5f * std::sin(theta) * std::sin(phi)); // Z (g³êbokoœæ)
        }
    }
    // Generowanie indeksów (trójk¹tów) dla siatki jajka
    for (int i = 0; i < eggSegments; ++i) {
        for (int j = 0; j < eggSegments; ++j) {
            int first = (i * (eggSegments + 1)) + j;
            int second = first + eggSegments + 1;
            eggIndices.push_back(first); eggIndices.push_back(second); eggIndices.push_back(first + 1);
            eggIndices.push_back(second); eggIndices.push_back(second + 1); eggIndices.push_back(first + 1);
        }
    }
    unsigned int eggVAO, eggVBO, eggEBO;
    glGenVertexArrays(1, &eggVAO); glGenBuffers(1, &eggVBO); glGenBuffers(1, &eggEBO);
    glBindVertexArray(eggVAO);
    glBindBuffer(GL_ARRAY_BUFFER, eggVBO); glBufferData(GL_ARRAY_BUFFER, eggVertices.size() * sizeof(float), eggVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eggEBO); glBufferData(GL_ELEMENT_ARRAY_BUFFER, eggIndices.size() * sizeof(unsigned int), eggIndices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);

    // === GEOMETRIA POD£OGI ===
    float floorVertices[] = { 50.0f, 0.0f, 50.0f, -50.0f, 0.0f, 50.0f, -50.0f, 0.0f, -50.0f, 50.0f, 0.0f, 50.0f, -50.0f, 0.0f, -50.0f, 50.0f, 0.0f, -50.0f };
    unsigned int floorVAO, floorVBO;
    glGenVertexArrays(1, &floorVAO); glGenBuffers(1, &floorVBO); glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO); glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);

    // === GEOMETRIA SZEŒCIANU (Fragmenty rozbicia) ===
    float cubeVertices[] = { -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f };
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO); glGenBuffers(1, &cubeVBO); glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO); glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);

    // === GEOMETRIA PÊKNIÊÆ (Linie) ===
    glGenVertexArrays(1, &crackVAO); glGenBuffers(1, &crackVBO); glBindVertexArray(crackVAO);
    glBindBuffer(GL_ARRAY_BUFFER, crackVBO); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); glEnableVertexAttribArray(0); glBindVertexArray(0);

    // === GEOMETRIA CHMUR (Sfery) ===
    std::vector<float> sphereVertices; std::vector<unsigned int> sphereIndices;
    int segments = 20;
    // (Podobnie jak jajko, ale sfera)
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
            sphereIndices.push_back(first); sphereIndices.push_back(second); sphereIndices.push_back(first + 1);
            sphereIndices.push_back(second); sphereIndices.push_back(second + 1); sphereIndices.push_back(first + 1);
        }
    }
    unsigned int sphereVAO, sphereVBO, sphereEBO;
    glGenVertexArrays(1, &sphereVAO); glGenBuffers(1, &sphereVBO); glGenBuffers(1, &sphereEBO);
    glBindVertexArray(sphereVAO); glBindBuffer(GL_ARRAY_BUFFER, sphereVBO); glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO); glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);

    // £adowanie modeli 3D z plików .obj
    Model tableModel("models/table.obj");
    Model rampModel("models/ramp.obj");

    // ==========================================
    // 6. G£ÓWNA PÊTLA GRY
    // ==========================================
    while (!glfwWindowShouldClose(window))
    {
        // Obliczanie czasu klatki (delta time)
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Zapamiêtanie pozycji przed ruchem (do kolizji bocznych)
        previousEggPosition = eggPosition;

        // Obs³uga wejœcia (klawiatura)
        processInput(window);

        // === FIZYKA: KOREKCJA KOLIZJI HORYZONTALNEJ ===
        // Najpierw ruszamy gracza (w processInput), a teraz sprawdzamy, czy wszed³ w œcianê
        if (currentState == GAME_STATE_PLAYING) {
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table1);
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table2);
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table3);
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table4); // NOWY STÓ£
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table5); // NOWY STÓ£
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table6); // NOWY STÓ£
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table7); // NOWY STÓ£
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table8); // NOWY STÓ£
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table9); // NOWY STÓ£
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, ramp_horizontal_box);
        }

        // === OBS£UGA RESETU GRY ===
        if (needsReset) {
            eggPosition = glm::vec3(0.0f, 0.7f, 5.0f); // Powrót na start
            velocityY = 0.0f;
            canJump = true;
            maxFallHeight = 0.7f;
            crackCount = 0;
            updateCrackGeometry(0);
            needsReset = false;
        }

        // === FIZYKA I LOGIKA GRY (Ruch w pionie) ===
        if (currentState == GAME_STATE_PLAYING)
        {
            float oldY = eggPosition.y; // Zapamiêtujemy wysokoœæ przed grawitacj¹

            // Aktualizacja maksymalnej wysokoœci (do obliczania obra¿eñ)
            if (velocityY >= 0.0f) {
                maxFallHeight = eggPosition.y > maxFallHeight ? eggPosition.y : maxFallHeight;
            }

            // Grawitacja
            velocityY += GRAVITY * deltaTime;
            eggPosition.y += velocityY * deltaTime;

            bool standingOnSomething = false;

            // === KOLIZJA Z RAMP¥ ===
            // Sprawdzamy, czy jesteœmy w obszarze rampy XZ
            if (!standingOnSomething && eggPosition.x > ramp.minX && eggPosition.x < ramp.maxX && eggPosition.z > ramp.minZ && eggPosition.z < ramp.maxZ) {
                standingOnSomething = checkRampCollision(
                    oldY,
                    eggPosition.y,
                    velocityY,
                    EGG_HALF_HEIGHT,
                    eggPosition.x,
                    eggPosition.y,
                    canJump,
                    maxFallHeight,
                    CRASH_HEIGHT_THRESHOLD,
                    CRACK_HEIGHT_THRESHOLD,
                    crackCount,
                    MAX_CRACKS,
                    currentFrame,
                    crashStartTime,
                    ramp,
                    updateCrackGeometry
                );
            }

            // === KOLIZJE ZE STO£AMI (Wertykalne - l¹dowanie) ===
            // Sprawdzamy stó³ 1
            if (isInsideXZ(eggPosition, table1)) {
                float desiredY1 = table1.topY + EGG_HALF_HEIGHT;
                // Jeœli opadaliœmy i przeciêliœmy poziom blatu
                if (oldY >= desiredY1 - 0.001f && eggPosition.y <= desiredY1 && velocityY <= 0.0f) {
                    float fallDistance = maxFallHeight - desiredY1;
                    if (fallDistance < 0.0f) { fallDistance = 0.0f; }
                    // Logika uszkodzeñ
                    if (fallDistance >= CRASH_HEIGHT_THRESHOLD) { currentState = GAME_STATE_CRASHED; crashStartTime = currentFrame; crackCount = MAX_CRACKS; }
                    else if (fallDistance >= CRACK_HEIGHT_THRESHOLD) { crackCount = glm::min(crackCount + 1, MAX_CRACKS); if (crackCount >= MAX_CRACKS) { currentState = GAME_STATE_CRASHED; crashStartTime = currentFrame; } updateCrackGeometry(crackCount); }

                    // L¹dowanie
                    maxFallHeight = desiredY1;
                    eggPosition.y = desiredY1;
                    velocityY = 0.0f;
                    canJump = true;
                    standingOnSomething = true;
                }
            }

            // Pozosta³e sto³y (2, 3, 4, 5, 6, 7, 8, 9) maj¹ blat 1.45f
            float desiredY_high_tables = 1.45f + EGG_HALF_HEIGHT;

            // --- FUNKCJA WSPÓLNA DLA WYSOKICH STO£ÓW ---
            auto checkHighTableCollision = [&](const TableHitbox& t) {
                if (!standingOnSomething && isInsideXZ(eggPosition, t)) {
                    if (oldY >= desiredY_high_tables - 0.001f && eggPosition.y <= desiredY_high_tables && velocityY <= 0.0f) {
                        float fallDistance = maxFallHeight - desiredY_high_tables;
                        if (fallDistance < 0.0f) { fallDistance = 0.0f; }

                        // Logika uszkodzeñ
                        if (fallDistance >= CRASH_HEIGHT_THRESHOLD) { currentState = GAME_STATE_CRASHED; crashStartTime = currentFrame; crackCount = MAX_CRACKS; }
                        else if (fallDistance >= CRACK_HEIGHT_THRESHOLD) { crackCount = glm::min(crackCount + 1, MAX_CRACKS); if (crackCount >= MAX_CRACKS) { currentState = GAME_STATE_CRASHED; crashStartTime = currentFrame; } updateCrackGeometry(crackCount); }

                        // L¹dowanie
                        maxFallHeight = desiredY_high_tables;
                        eggPosition.y = desiredY_high_tables;
                        velocityY = 0.0f;
                        canJump = true;
                        standingOnSomething = true;
                    }
                }
                };

            // Sprawdzamy wszystkie sto³y o wysokoœci 1.45f
            checkHighTableCollision(table2);
            checkHighTableCollision(table3);
            checkHighTableCollision(table4); 
            checkHighTableCollision(table5); 
            checkHighTableCollision(table6); 
            checkHighTableCollision(table7); 
            checkHighTableCollision(table8); 
            checkHighTableCollision(table9); 

            // === KOLIZJA Z ZIEMI¥ (Pod³oga na 0.0f) ===
            if (!standingOnSomething) {
                if (eggPosition.y < 0.7f) { // 0.7f to œrodek jajka stoj¹cego na Y=0
                    if (oldY >= 0.7f - 0.001f && eggPosition.y < 0.7f && velocityY <= 0.0f) {
                        float desiredY = 0.7f;
                        float fallDistance = maxFallHeight - desiredY;
                        if (fallDistance < 0.0f) { fallDistance = 0.0f; }

                        if (fallDistance >= CRASH_HEIGHT_THRESHOLD) {
                            currentState = GAME_STATE_CRASHED;
                            crashStartTime = currentFrame;
                            crackCount = MAX_CRACKS;
                        }
                        else if (fallDistance >= CRACK_HEIGHT_THRESHOLD) {
                            crackCount = glm::min(crackCount + 1, MAX_CRACKS);
                            if (crackCount >= MAX_CRACKS) { currentState = GAME_STATE_CRASHED; crashStartTime = currentFrame; }
                            updateCrackGeometry(crackCount);
                        }

                        maxFallHeight = desiredY;
                        eggPosition.y = desiredY;
                        velocityY = 0.0f;
                        canJump = true;
                        standingOnSomething = true;
                    }
                }
            }

            // === AKTUALIZACJA CHMUR ===
            cloudSpawnTimer += deltaTime;
            if (cloudSpawnTimer > 2.0f && clouds.size() < 15) {
                spawnCloud();
                cloudSpawnTimer = 0.0f;
            }

            // Przesuwanie i usuwanie starych chmur
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

        // ==========================================
        // 7. RENDEROWANIE (RYSOWANIE)
        // ==========================================

        // Czyszczenie ekranu (kolor t³a i bufor g³êbokoœci)
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // Jasny b³êkit
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        // Macierz projekcji (perspektywa)
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 150.0f);
        glm::mat4 view;

        // Ustawianie kamery
        if (currentState == GAME_STATE_PLAYING || currentState == GAME_STATE_CRASHED) {
            glm::vec3 cameraPos = eggPosition - cameraFront * cameraDistance;

            // Podniesienie kamery nad gracza
            cameraPos += glm::vec3(0.0f, 1.5f, 0.0f);

            // Zabezpieczenie przed wchodzeniem kamery pod pod³ogê
            const float minCameraHeight = 0.5f;
            if (cameraPos.y < minCameraHeight) cameraPos.y = minCameraHeight;

            // Kamera patrzy nieco nad gracza
            view = glm::lookAt(cameraPos, eggPosition + glm::vec3(0.0f, 0.5f, 0.0f), cameraUp);
        }
        else {
            // Statyczna kamera w Menu
            glm::vec3 menuCameraPos = glm::vec3(0.0f, 5.0f, 15.0f);
            view = glm::lookAt(menuCameraPos, glm::vec3(0.0f, 2.0f, 0.0f), cameraUp);
        }

        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // --- RYSOWANIE POD£OGI ---
        glBindVertexArray(floorVAO);
        ourShader.setInt("useTexture", 0);
        ourShader.setMat4("model", glm::mat4(1.0f));
        ourShader.setVec3("objectColor", glm::vec3(0.2f, 0.6f, 0.1f)); // Zielony
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // --- RYSOWANIE STO£ÓW ---
        ourShader.setInt("useTexture", 1);

        // Stó³ 1 (Blat 0.68f, Œrodek X: -2.0f)
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-2.0f, -0.05f, 0.0f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // Stó³ 2 (Blat 1.45f, Œrodek X: 0.0f)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.77f, 0.0f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // Stó³ 3 (Blat 1.45f, Œrodek X: 2.5f)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.5f, 0.77f, 0.0f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // Stó³ 4 (Lewo, Œrodek X: 5.0f, Z: -1.5f)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(5.0f, 0.77f, -1.5f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // Stó³ 5 (Prawo, Œrodek X: 7.5f, Z: 1.5f)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(7.5f, 0.77f, 1.5f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // Stó³ 6 (Lewo, Œrodek X: 10.0f, Z: -1.5f)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(10.0f, 0.77f, -1.5f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // Stó³ 7 (Prawo, Œrodek X: 12.5f, Z: 1.5f)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(12.5f, 0.77f, 1.5f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // Stó³ 8 (Lewo, Œrodek X: 15.0f, Z: -1.5f)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(15.0f, 0.77f, -1.5f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // Stó³ 9 (Podejœcie, Œrodek X: 18.0f, Z: 0.0f)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(18.0f, 0.77f, 0.0f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // --- RYSOWANIE RAMPY (Nowa pozycja startowa X=20.0f) ---
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(20.0f, 2.05f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", model);
        rampModel.Draw(ourShader);

        // --- RYSOWANIE GRACZA (JAJKA) ---
        if (currentState != GAME_STATE_CRASHED) {
            glBindVertexArray(eggVAO);
            ourShader.setInt("useTexture", 0);
            model = glm::translate(glm::mat4(1.0f), eggPosition);
            ourShader.setMat4("model", model);
            ourShader.setVec3("objectColor", glm::vec3(1.0f, 0.9f, 0.7f)); // Kolor jajka
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(eggIndices.size()), GL_UNSIGNED_INT, 0);

            // Rysowanie pêkniêæ (czarne linie)
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
        else {
            // --- ANIMACJA ROZBICIA ---
            float timeElapsed = currentFrame - crashStartTime;
            float ratio = timeElapsed / CRASH_ANIMATION_DURATION;

            if (ratio < 1.0f) {
                glBindVertexArray(cubeVAO);
                ourShader.setInt("useTexture", 0);
                ourShader.setVec3("objectColor", glm::vec3(1.0f, 0.9f, 0.7f));

                for (int i = 0; i < 5; ++i) {
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, eggPosition);
                    // Fizyka wybuchu fragmentów
                    float explosionForce = 3.0f * (1.0f - ratio);
                    glm::vec3 fragmentOffset = fragments[i] * explosionForce * timeElapsed;
                    fragmentOffset.y -= 5.0f * timeElapsed * timeElapsed; // Grawitacja fragmentów
                    model = glm::translate(model, fragmentOffset);
                    float scaleFactor = glm::mix(0.1f, 0.0f, ratio); // Fragmenty znikaj¹
                    model = glm::scale(model, glm::vec3(scaleFactor));
                    ourShader.setMat4("model", model);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
                glBindVertexArray(0);
            }
        }

        // --- RYSOWANIE CHMUR ---
        glBindVertexArray(sphereVAO);
        ourShader.setInt("useTexture", 0);
        ourShader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f)); // Bia³e chmury
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

        // --- RYSOWANIE UI (MENU) ---
        // W³¹czamy przezroczystoœæ dla tekstury menu (kana³ Alpha)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST); // UI rysowane zawsze na wierzchu

        if (currentState == GAME_STATE_MENU || currentState == GAME_STATE_CRASHED) {
            uiShader->use();
            glBindVertexArray(uiVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, menuTexture);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);

            if (currentState == GAME_STATE_CRASHED) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Poka¿ kursor
            }
        }

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST); // Przywracamy sprawdzanie g³êbokoœci dla 3D

        glfwSwapBuffers(window); // Podmiana buforów ekranu
        glfwPollEvents();        // Sprawdzenie zdarzeñ systemowych (klawiatura, mysz)
    }

    // === ZWOLNIENIE ZASOBÓW (CLEANUP) ===
    glDeleteVertexArrays(1, &eggVAO); glDeleteBuffers(1, &eggVBO); glDeleteBuffers(1, &eggEBO);
    glDeleteVertexArrays(1, &floorVAO); glDeleteBuffers(1, &floorVBO);
    glDeleteVertexArrays(1, &cubeVAO); glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &crackVAO); glDeleteBuffers(1, &crackVBO);
    glDeleteVertexArrays(1, &sphereVAO); glDeleteBuffers(1, &sphereVBO); glDeleteBuffers(1, &sphereEBO);
    glDeleteVertexArrays(1, &uiVAO); glDeleteBuffers(1, &uiVBO);
    glDeleteTextures(1, &menuTexture);
    delete uiShader;

    glfwTerminate(); // Zamkniêcie GLFW
    return 0;
}

// Funkcja obs³uguj¹ca wejœcie z klawiatury
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // MENU -> GRA
    if (currentState == GAME_STATE_MENU) {
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            currentState = GAME_STATE_PLAYING;
            needsReset = true; // Zresetuj pozycjê przy starcie
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Ukryj kursor
            firstMouse = true;
        }
    }
    // ŒMIERÆ -> MENU
    else if (currentState == GAME_STATE_CRASHED) {
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            currentState = GAME_STATE_MENU;
            eggPosition = glm::vec3(0.0f, 0.7f, 5.0f);
            velocityY = 0.0f;
            canJump = true;
            maxFallHeight = 0.0f;
            crackCount = 0;
            updateCrackGeometry(0);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    // STEROWANIE W TRAKCIE GRY
    else if (currentState == GAME_STATE_PLAYING) {
        const float WALK_SPEED = 2.5f;
        const float SPRINT_SPEED = 5.0f;
        float currentSpeed = WALK_SPEED;

        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            currentSpeed = SPRINT_SPEED;
        }

        // Obliczanie kierunków ruchu wzglêdem kamery
        glm::vec3 forward = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z)); // Ruch tylko w p³aszczyŸnie poziomej
        glm::vec3 right = glm::normalize(glm::cross(forward, cameraUp));

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) eggPosition += forward * currentSpeed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) eggPosition -= forward * currentSpeed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) eggPosition -= right * currentSpeed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) eggPosition += right * currentSpeed * deltaTime;

        // Skakanie
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && canJump) {
            velocityY = JUMP_FORCE;
            canJump = false;
        }
    }
}

// Callback zmiany rozmiaru okna
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
    if (uiShader) {
        // Aktualizacja projekcji UI przy zmianie rozmiaru
        glm::mat4 projection = glm::ortho(0.0f, (float)width, 0.0f, (float)height);
        uiShader->use();
        uiShader->setMat4("projection", projection);
    }
}

// Callback ruchu myszk¹ (obracanie kamer¹)
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (currentState != GAME_STATE_PLAYING) return;

    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos); // Odwrócone, bo Y roœnie w dó³ ekranu
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Blokada kamery góra/dó³ (¿eby nie zrobiæ fiko³ka)
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Obliczanie wektora kierunku kamery
    glm::vec3 direction;
    direction.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    direction.y = std::sin(glm::radians(pitch));
    direction.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}