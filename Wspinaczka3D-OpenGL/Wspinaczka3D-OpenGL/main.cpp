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

// Struktura opisująca płaski stół (prostopadłościan)
struct TableHitbox {
    float minX, maxX; // Zakres w osi X
    float minZ, maxZ; // Zakres w osi Z
    float topY;       // Wysokość blatu (powierzchnia, na której się stoi)
};

// Struktura opisująca rampę (pochyłą powierzchnię)
struct RampHitbox {
    float minX, maxX;
    float minZ, maxZ;
    float startY, endY; // Wysokość początkowa i końcowa rampy
    float lengthZ;
};

// Struktury dla systemu chmur (ozdoba tła)
struct CloudComponent { glm::vec3 offset; float scale; };
struct Cloud {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    std::vector<CloudComponent> components;
};

// Struktura dla ruchomej platformy
struct MovingPlatform {
    TableHitbox hitbox;
    glm::vec3 startPos;
    glm::vec3 endPos;
    float speed;
    float progress; // Od 0.0 do 1.0
    int direction;  // 1 (w stronę end), -1 (w stronę start)
    glm::vec3 currentOffset; // O ile platforma przesunęła się w tej klatce
};


// ==========================================
// 2. PROTOTYPY FUNKCJI (Deklaracje)
// ==========================================
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void updateCrackGeometry(int currentCrackCount);

// Główna funkcja kolizji rampy - oblicza wysokość gracza na pochyłości
bool checkRampCollision(float oldY, float newY, float& velocityY, float EGG_HALF_HEIGHT, float currentEggX, float& eggPositionY, bool& canJump, float& maxFallHeight, const float CRASH_HEIGHT_THRESHOLD, const float CRACK_HEIGHT_THRESHOLD, int& crackCount, const int MAX_CRACKS, float currentFrame, float& crashStartTime, const RampHitbox& ramp, void (*updateCrackGeom)(int));

// Funkcje pomocnicze do kolizji prostokątnych
static bool isInsideXZ(const glm::vec3& pos, const TableHitbox& t);
static void checkHorizontalCollisionAndRevert(const glm::vec3& newPos, const glm::vec3& oldPos, const TableHitbox& t);

// ==========================================
// 3. ZMIENNE GLOBALNE
// ==========================================
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

// Stany gry - zarządzanie menu i rozgrywką
enum GameState {
    GAME_STATE_MENU,
    GAME_STATE_PLAYING,
    GAME_STATE_CRASHED
};
GameState currentState = GAME_STATE_MENU;
bool needsReset = false; // Flaga informująca, że trzeba zresetować pozycję gracza

// Zmienne do obsługi UI (Menu)
Shader* uiShader = nullptr;
unsigned int uiVAO = 0, uiVBO = 0;
unsigned int menuTexture = 0;

// === KAMERA I GRACZ ===
glm::vec3 eggPosition = glm::vec3(0.0f, 0.7f, 5.0f); // Startowa pozycja gracza
glm::vec3 previousEggPosition = eggPosition;         // Pozycja z poprzedniej klatki (do kolizji)
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraDistance = 7.0f;

// Obsługa myszy
float lastX;
float lastY;
bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;

// Czas klatki (dla płynnego ruchu niezależnie od FPS)
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// === FIZYKA ===
float velocityY = 0.0f; // Prędkość wertykalna (skok/upadek)
bool canJump = true;    // Czy gracz stoi na ziemi?
const float GRAVITY = -9.8f;
const float JUMP_FORCE = 4.0f;

// === SYSTEM USZKODZEŃ ===
float maxFallHeight = 0.0f;       // Najwyższy punkt osiągnięty podczas skoku
const float CRASH_HEIGHT_THRESHOLD = 1.5f; // Wysokość upadku powodująca śmierć
const float CRACK_HEIGHT_THRESHOLD = 0.9f; // Wysokość upadku powodująca pęknięcie
int crackCount = 0;
const int MAX_CRACKS = 3;
std::vector<float> crackVertices; // Wierzchołki rysowanych pęknięć
unsigned int crackVAO = 0, crackVBO = 0;

// Animacja rozbicia jajka
float crashStartTime = 0.0f;
const float CRASH_ANIMATION_DURATION = 0.7f;
// Fragmenty skorupki do animacji wybuchu
glm::vec3 fragments[5] = { glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(-0.5f, 0.5f, -0.5f) };
const float EGG_HALF_HEIGHT = 0.7f; // Połowa wysokości jajka (ważne do kolizji)

// === DEFINICJE OBIEKTÓW (HITBOXY) ===
// Stół 1 (Wymiary 1.6x1.6)
TableHitbox table1 = { -2.8f, -1.2f, -0.8f, 0.8f, 0.68f }; // Środek X=-2.0f

// Stół 2 (Wymiary 1.6x1.6)
TableHitbox table2 = { -0.8f, 0.8f, -0.8f, 0.8f, 1.45f }; // Środek X=0.0f

// Stół 3 (Wymiary 1.6x1.6)
TableHitbox table3 = { 1.7f, 3.3f, -0.8f, 0.8f, 1.45f }; // Środek X=2.5f

// Stół 4 (Lewo, Wymiary 1.6x1.6)
TableHitbox table4 = { 4.2f, 5.8f, -2.3f, -0.7f, 1.45f }; // Środek X=5.0f, Z=-1.5f

// Stół 5 (Prawo, Wymiary 1.6x1.6)
TableHitbox table5 = { 6.7f, 8.3f, 0.7f, 2.3f, 1.45f }; // Środek X=7.5f, Z=1.5f

// Stół 6 (Lewo, Wymiary 1.6x1.6)
TableHitbox table6 = { 9.2f, 10.8f, -2.3f, -0.7f, 1.45f }; // Środek X=10.0f, Z=-1.5f

// Stół 7 (Prawo, Wymiary 1.6x1.6)
TableHitbox table7 = { 11.7f, 13.3f, 0.7f, 2.3f, 1.45f }; // Środek X=12.5f, Z=1.5f

// Stół 8 (Lewo, Wymiary 1.6x1.6)
TableHitbox table8 = { 14.2f, 15.8f, -2.3f, -0.7f, 1.45f }; // Środek X=15.0f, Z=-1.5f

// Stół 9 (Podejście, Wymiary 1.6x1.6)
TableHitbox table9 = { 17.2f, 18.8f, -0.8f, 0.8f, 1.45f }; // Środek X=18.0f, Z=0.0f

// Stół do testow
TableHitbox table10 = { 17.2f, 18.8f, 1.7f, 3.3f, 0.68f };

// Rampa ( X=20.0f. Długość 4m do X=24.0f)
RampHitbox ramp = { 20.0f, 24.0f, -1.2f, 1.2f, 2.05f, 2.85f, 4.0f };
// Hitbox boczny rampy (żeby nie przenikać przez jej boki na dole)
TableHitbox ramp_horizontal_box = { 20.0f, 24.0f, -0.9f, 0.9f, 2.05f };

GlassBridge* glassBridge = nullptr; // Wskaźnik na most

// Trampolina
Trampoline* bouncyTrampoline = nullptr;

// Definicja Bezpiecznej Strefy (wysoko w górze)
// Załóżmy, że trampolina jest na X=40, wybija nas na X=45, Y=20
TableHitbox safeZone = { 43.0f, 47.0f, -2.0f, 2.0f, 15.0f };

// Definicja ruchomej platformy (Start Poziomu 2)
// Porusza się od Z=-4.5 do Z=4.5 na wysokości Y=15.0, pozycja X=52.0 (za poduszką)
MovingPlatform level2Platform = {
    { 50.5f, 53.5f, -1.5f, 1.5f, 15.0f }, // Wstępny hitbox (będzie aktualizowany w pętli)
    glm::vec3(52.0f, 15.0f, -4.5f),       // Pozycja Startowa (Lewa strona)
    glm::vec3(52.0f, 15.0f, 4.5f),        // Pozycja Końcowa (Prawa strona)
    2.0f,                                 // Prędkość
    0.0f, 1, glm::vec3(0.0f)              // Zmienne techniczne (nie zmieniać)
};

// Chmury
std::vector<Cloud> clouds;
float cloudSpawnTimer = 0.0f;

// ==========================================
// 4. IMPLEMENTACJA FUNKCJI POMOCNICZYCH
// ==========================================

// Sprawdza, czy punkt jest wewnątrz prostokąta (tylko osie X i Z)
static bool isInsideXZ(const glm::vec3& pos, const TableHitbox& t) {
    return pos.x > t.minX && pos.x < t.maxX &&
        pos.z > t.minZ && pos.z < t.maxZ;
}

// Sprawdza kolizję "boczną" (chodzenie w ściany) i cofa ruch, jeśli nastąpiła
static void checkHorizontalCollisionAndRevert(const glm::vec3& newPos, const glm::vec3& oldPos, const TableHitbox& t) {
    // Obliczamy poziom, na którym spoczywałby środek jajka stojąc na blacie
    float restingCenterY = t.topY + EGG_HALF_HEIGHT;

    // Jeśli jajko jest WYŻEJ niż blat (minus margines tolerancji), to znaczy, że wchodzimy NA stół, a nie W stół.
    // 0.10f to tolerancja ułatwiająca wskakiwanie.
    if (newPos.y > restingCenterY - 0.10f) {
        return; // Brak kolizji bocznej, jesteśmy nad obiektem
    }

    // Jeśli jesteśmy niżej i weszliśmy w obrys XZ stołu -> cofamy ruch X i Z
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

// Ładowanie tekstur z pliku za pomocą biblioteki stb_image
static unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true); // Odwracamy teksturę (OpenGL ma Y=0 na dole)
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

    // Każda chmura składa się z kilku sfer (komponentów)
    int numComponents = rand() % 4 + 3;
    for (int i = 0; i < numComponents; ++i) {
        CloudComponent component;
        component.offset = glm::vec3(randomFloat(-2.5f, 2.5f), randomFloat(-1.0f, 1.0f), randomFloat(-1.5f, 1.5f));
        component.scale = randomFloat(1.5f, 2.5f);
        newCloud.components.push_back(component);
    }
    clouds.push_back(newCloud);
}

// Proceduralne generowanie pęknięć na skorupce jajka
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

        int numSegments = 5 + i * 3; // Im więcej pęknięć, tym dłuższe

        // Generowanie linii pęknięcia (błądzenie losowe)
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

    // Przesłanie danych do bufora graficznego (VBO)
    const void* dataPtr = crackVertices.empty() ? nullptr : crackVertices.data();
    size_t dataSize = crackVertices.size() * sizeof(float);

    glBindBuffer(GL_ARRAY_BUFFER, crackVBO);
    glBufferData(GL_ARRAY_BUFFER, dataSize, dataPtr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// === LOGIKA KOLIZJI Z RAMPĄ ===
// Sprawdza, czy gracz stoi na pochyłej powierzchni.
// Oblicza wysokość podłoża na podstawie pozycji X gracza.
bool checkRampCollision(float oldY, float newY, float& velocityY, float EGG_HALF_HEIGHT, float currentEggX, float& eggPositionY, bool& canJump, float& maxFallHeight, const float CRASH_HEIGHT_THRESHOLD, const float CRACK_HEIGHT_THRESHOLD, int& crackCount, const int MAX_CRACKS, float currentFrame, float& crashStartTime, const RampHitbox& ramp, void (*updateCrackGeom)(int)) {
    float distanceX = ramp.maxX - ramp.minX; // Długość rampy
    float heightChange = ramp.endY - ramp.startY; // Różnica wysokości

    // Obliczamy procent, jak daleko gracz jest na rampie (0.0 = początek, 1.0 = koniec)
    float rampRatio = glm::clamp((currentEggX - ramp.minX) / distanceX, 0.0f, 1.0f);

    // Wyliczamy wysokość podłogi w tym konkretnym punkcie X
    float surfaceY = ramp.startY + heightChange * rampRatio;
    float desiredCenterY = surfaceY + EGG_HALF_HEIGHT; // Tu powinien być środek jajka

    // Sprawdzamy, czy gracz dotyka powierzchni rampy (z tolerancją 0.5f na wejście)
    if (oldY >= desiredCenterY - 0.5f && newY <= desiredCenterY && velocityY <= 0.0f) {

        // Obliczamy obrażenia od upadku
        float fallDistance = maxFallHeight - desiredCenterY;
        if (fallDistance < 0.0f) { fallDistance = 0.0f; }

        if (fallDistance >= CRASH_HEIGHT_THRESHOLD) {
            currentState = GAME_STATE_CRASHED; // Śmierć
            crashStartTime = currentFrame;
            crackCount = MAX_CRACKS;
        }
        else if (fallDistance >= CRACK_HEIGHT_THRESHOLD) {
            crackCount = glm::min(crackCount + 1, MAX_CRACKS); // Pęknięcie
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

// Funkcja blokująca przenikanie przez boki trampoliny
static void checkTrampolineSideCollision(glm::vec3& currentPos, const glm::vec3& oldPos, Trampoline* t) {
    if (!t) return;

    float playerFeetY = currentPos.y - 0.7f; // Poziom stóp

    // === ZMIANA LOGIKI WYSOKOŚCI ===
    // Zamiast sprawdzać 't->height' (poziom maty), ustawiamy barierę wyżej.
    // Trampolina jest w dole (Y=-2.0), podłoga na Y=0.0.
    // Ustawiamy barierę na Y = 1.0f.
    // To oznacza, że "ściana" trampoliny wystaje 1 metr ponad zwykłą podłogę.
    // Zwykły skok z podłogi nie pozwoli tego przeskoczyć, ale spadając z mostu (Y=2.85) przelecisz nad nią.

    float barrierCeiling = 1.0f;

    // Jeśli jesteśmy NAD barierą, pozwalamy na ruch (spadamy z góry)
    if (playerFeetY > barrierCeiling) {
        return;
    }

    // === KOLIZJA POZIOMA (XZ) ===
    glm::vec3 flatPlayer = glm::vec3(currentPos.x, 0.0f, currentPos.z);
    glm::vec3 flatTrampoline = glm::vec3(t->position.x, 0.0f, t->position.z);

    float distance = glm::distance(flatPlayer, flatTrampoline);

    // Twoja wartość 1.1f jest dobra, jeśli model jest szeroki.
    // Blokujemy wejście, jeśli gracz jest za blisko środka, a nie jest nad barierą.
    float collisionDist = t->radius + 1.1f;

    if (distance < collisionDist) {
        // Cofamy ruch
        currentPos.x = oldPos.x;
        currentPos.z = oldPos.z;
    }
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

    // Pobranie rozdzielczości monitora
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

    // Inicjalizacja GLAD (wskaźniki do funkcji OpenGL)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Blad inicjalizacji GLAD!" << std::endl;
        return -1;
    }

    // Ustawienia globalne OpenGL
    glEnable(GL_DEPTH_TEST); // Włączenie bufora głębokości (Z-buffer)
    glDisable(GL_CULL_FACE); // Wyłączenie ukrywania tylnych ścianek (widoczność rampy od spodu)
    srand(static_cast<unsigned int>(time(0)));

    // Kompilacja Shaderów (z plików .glsl)
    Shader ourShader("vertex_shader.glsl", "fragment_shader.glsl");

    // === KONFIGURACJA UI (MENU) ===
    uiShader = new Shader("ui_vertex.glsl", "ui_fragment.glsl");
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
    uiShader->use();
    uiShader->setMat4("projection", projection);
    uiShader->setInt("uiTexture", 0);

    // Współrzędne prostokąta menu (wycentrowanego)
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
            eggVertices.push_back(0.5f * std::sin(theta) * std::cos(phi)); // X (szerokość)
            eggVertices.push_back(0.7f * std::cos(theta));               // Y (wysokość)
            eggVertices.push_back(0.5f * std::sin(theta) * std::sin(phi)); // Z (głębokość)
        }
    }
    // Generowanie indeksów (trójkątów) dla siatki jajka
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

    // === GEOMETRIA PODŁOGI ===
    /*float floorVertices[] = { 50.0f, 0.0f, 50.0f, -50.0f, 0.0f, 50.0f, -50.0f, 0.0f, -50.0f, 50.0f, 0.0f, 50.0f, -50.0f, 0.0f, -50.0f, 50.0f, 0.0f, -50.0f };
    unsigned int floorVAO, floorVBO;
    glGenVertexArrays(1, &floorVAO); glGenBuffers(1, &floorVBO); glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO); glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);*/

    // === GEOMETRIA SZEŚCIANU (Fragmenty rozbicia) ===
    float cubeVertices[] = { -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f };
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO); glGenBuffers(1, &cubeVBO); glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO); glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);

    // === GEOMETRIA PĘKNIĘĆ (Linie) ===
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

    // Ładowanie modeli 3D z plików .obj
    Model tableModel("models/table.obj");
    Model rampModel("models/ramp.obj");
    Model tileModel("models/glass_tile.obj");
    Model floorModel("models/floor.obj");
    Model trampolineModel("models/trampoline.obj");
    Model pillowModel("models/pillow.obj");
    glassBridge = new GlassBridge(glm::vec3(25.0f, 0.0f, 0.0f), 2.85f, &tileModel);
    glm::vec3 trampolinePos = glm::vec3(41.0f, 0.0, 0.0f);
    // Trampolina hitbox
    glm::vec3 scale = glm::vec3(0.2f);
    glm::vec3 offset = glm::vec3(0.0f, 0.0f, 0.0f);
    float hitHeight = 0.5f;
    float force = 35.0f;
    float hitRadius = 0.4f;
    bouncyTrampoline = new Trampoline(
        trampolinePos,
        hitRadius,
        hitHeight,
        force,
        &trampolineModel,
        scale,
        offset
    );

    // ==========================================
    // 6. GŁÓWNA PĘTLA GRY
    // ==========================================
    while (!glfwWindowShouldClose(window))
    {
        // Obliczanie czasu klatki (delta time)
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Zapamiętanie pozycji przed ruchem (do kolizji bocznych)
        previousEggPosition = eggPosition;

        // Obsługa wejścia (klawiatura)
        processInput(window);

        // === FIZYKA: KOREKCJA KOLIZJI HORYZONTALNEJ ===
        // Najpierw ruszamy gracza (w processInput), a teraz sprawdzamy, czy wszedł w ścianę
        if (currentState == GAME_STATE_PLAYING) {
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table1);
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table2);
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table3);
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table4);
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table5);
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table6);
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table7);
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table8);
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table9);
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, ramp_horizontal_box);
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, table10); //stol tylko do testow
            if (bouncyTrampoline) {
                checkTrampolineSideCollision(eggPosition, previousEggPosition, bouncyTrampoline);
            }
            // <--- DODANO: Kolizja boczna z bezpieczną strefą (metą)
            checkHorizontalCollisionAndRevert(eggPosition, previousEggPosition, safeZone);
        }

        // === OBSŁUGA RESETU GRY ===
        if (needsReset) {
            eggPosition = glm::vec3(0.0f, 0.7f, 5.0f); // Powrót na start
            velocityY = 0.0f;
            canJump = true;
            maxFallHeight = 0.7f;
            crackCount = 0;
            updateCrackGeometry(0);
            if (glassBridge) glassBridge->Reset();
            needsReset = false;
        }

        // === FIZYKA I LOGIKA GRY (Ruch w pionie) ===
        if (currentState == GAME_STATE_PLAYING)
        {
            float oldY = eggPosition.y; // Zapamiętujemy wysokość przed grawitacją

            // Aktualizacja maksymalnej wysokości (do obliczania obrażeń)
            if (velocityY >= 0.0f) {
                maxFallHeight = eggPosition.y > maxFallHeight ? eggPosition.y : maxFallHeight;
            }

            // Grawitacja
            velocityY += GRAVITY * deltaTime;
            eggPosition.y += velocityY * deltaTime;

            bool standingOnSomething = false;

            // --- LOGIKA RUCHOMEJ PLATFORMY ---
            // 1. Obliczanie nowej pozycji platformy
            float platDist = glm::distance(level2Platform.startPos, level2Platform.endPos);
            level2Platform.progress += (level2Platform.speed * deltaTime / platDist) * level2Platform.direction;
            if (level2Platform.progress >= 1.0f || level2Platform.progress <= 0.0f) level2Platform.direction *= -1;
            level2Platform.progress = glm::clamp(level2Platform.progress, 0.0f, 1.0f);

            // 2. Obliczanie przesunięcia (offsetu)
            glm::vec3 lastPlatPos = glm::mix(level2Platform.startPos, level2Platform.endPos,
                glm::clamp(level2Platform.progress - (level2Platform.speed * deltaTime * level2Platform.direction / platDist), 0.0f, 1.0f));
            glm::vec3 currPlatPos = glm::mix(level2Platform.startPos, level2Platform.endPos, level2Platform.progress);
            level2Platform.currentOffset = currPlatPos - lastPlatPos;

            // 3. Aktualizacja Hitboxa (żeby kolizja podążała za modelem)
            float platRadius = 1.5f; // Połowa szerokości platformy
            level2Platform.hitbox.minX = currPlatPos.x - platRadius;
            level2Platform.hitbox.maxX = currPlatPos.x + platRadius;
            level2Platform.hitbox.minZ = currPlatPos.z - platRadius;
            level2Platform.hitbox.maxZ = currPlatPos.z + platRadius;
            level2Platform.hitbox.topY = currPlatPos.y;

            // 4. Sprawdzanie kolizji z graczem
            if (!standingOnSomething && isInsideXZ(eggPosition, level2Platform.hitbox)) {
                float dY = level2Platform.hitbox.topY + EGG_HALF_HEIGHT;
                // Jeśli lądujemy na platformie
                if (oldY >= dY - 0.1f && eggPosition.y <= dY && velocityY <= 0.0f) {
                    eggPosition.y = dY;
                    velocityY = 0.0f;
                    canJump = true;
                    standingOnSomething = true;
                    maxFallHeight = dY;
                    // PRZYKLEJENIE: Przesuwamy jajko razem z platformą
                    eggPosition += level2Platform.currentOffset;
                }
            }

            // === KOLIZJA Z RAMPĄ ===
            // Sprawdzamy, czy jesteśmy w obszarze rampy XZ
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
            // === KOLIZJA Z SZKLANYM MOSTEM ===
            if (!standingOnSomething && glassBridge) {
                bool onBridge = glassBridge->checkCollision(
                    eggPosition,
                    eggPosition.y, // referencja, zostanie zaktualizowana jeśli stoimy
                    velocityY,     // referencja, zostanie wyzerowana jeśli stoimy
                    EGG_HALF_HEIGHT
                );

                if (onBridge) {
                    standingOnSomething = true;
                    canJump = true;
                    maxFallHeight = eggPosition.y; // reset obrażeń
                }
            }

            // <--- DODANO: KOLIZJA Z TRAMPOLINĄ ===
            if (!standingOnSomething && bouncyTrampoline) {
                // checkCollision aktualizuje velocityY, jeśli trafimy
                bool bounced = bouncyTrampoline->checkCollision(
                    eggPosition,
                    eggPosition.y, // referencja
                    velocityY,     // referencja (tu zostanie wpisane 25.0f)
                    EGG_HALF_HEIGHT
                );

                if (bounced) {
                    canJump = false; // Nie można skoczyć w powietrzu po wybiciu
                    // WAŻNE: Resetujemy maxFallHeight do obecnej wysokości,
                    // żeby gra nie myślała, że spadamy z kosmosu przy lądowaniu.
                    maxFallHeight = eggPosition.y;

                    // Opcjonalnie: Dźwięk "Boing!" :)
                }
            }

            // <--- DODANO: KOLIZJA Z BEZPIECZNĄ STREFĄ  ===
            if (!standingOnSomething && isInsideXZ(eggPosition, safeZone)) {
                float desiredY_Safe = safeZone.topY + EGG_HALF_HEIGHT;

                // Sprawdzamy czy lądujemy na powierzchni
                if (oldY >= desiredY_Safe - 0.001f && eggPosition.y <= desiredY_Safe && velocityY <= 0.0f) {



                    // Fizyka lądowania
                    maxFallHeight = desiredY_Safe;
                    eggPosition.y = desiredY_Safe;
                    velocityY = 0.0f;
                    canJump = true;
                    standingOnSomething = true;
                }
            }

            // === KOLIZJE ZE STOŁAMI (Wertykalne - lądowanie) ===
            // Sprawdzamy stół 1
            if (isInsideXZ(eggPosition, table1)) {
                float desiredY1 = table1.topY + EGG_HALF_HEIGHT;
                // Jeśli opadaliśmy i przecięliśmy poziom blatu
                if (oldY >= desiredY1 - 0.001f && eggPosition.y <= desiredY1 && velocityY <= 0.0f) {
                    float fallDistance = maxFallHeight - desiredY1;
                    if (fallDistance < 0.0f) { fallDistance = 0.0f; }
                    // Logika uszkodzeń
                    if (fallDistance >= CRASH_HEIGHT_THRESHOLD) { currentState = GAME_STATE_CRASHED; crashStartTime = currentFrame; crackCount = MAX_CRACKS; }
                    else if (fallDistance >= CRACK_HEIGHT_THRESHOLD) { crackCount = glm::min(crackCount + 1, MAX_CRACKS); if (crackCount >= MAX_CRACKS) { currentState = GAME_STATE_CRASHED; crashStartTime = currentFrame; } updateCrackGeometry(crackCount); }

                    // Lądowanie
                    maxFallHeight = desiredY1;
                    eggPosition.y = desiredY1;
                    velocityY = 0.0f;
                    canJump = true;
                    standingOnSomething = true;
                }
            }
            // === KOLIZJA DLA STOŁU 10 (TESTOWEGO) ===
            if (isInsideXZ(eggPosition, table10)) {
                float desiredY10 = table10.topY + EGG_HALF_HEIGHT;
                if (oldY >= desiredY10 - 0.001f && eggPosition.y <= desiredY10 && velocityY <= 0.0f) {
                    float fallDistance = maxFallHeight - desiredY10;
                    if (fallDistance < 0.0f) fallDistance = 0.0f;

                    // Obrażenia
                    if (fallDistance >= CRASH_HEIGHT_THRESHOLD) { currentState = GAME_STATE_CRASHED; crashStartTime = currentFrame; crackCount = MAX_CRACKS; }
                    else if (fallDistance >= CRACK_HEIGHT_THRESHOLD) {
                        crackCount = glm::min(crackCount + 1, MAX_CRACKS);
                        if (crackCount >= MAX_CRACKS) { currentState = GAME_STATE_CRASHED; crashStartTime = currentFrame; }
                        updateCrackGeometry(crackCount);
                    }

                    // Lądowanie
                    maxFallHeight = desiredY10;
                    eggPosition.y = desiredY10;
                    velocityY = 0.0f;
                    canJump = true;
                    standingOnSomething = true;
                }
            }

            // Pozostałe stoły (2, 3, 4, 5, 6, 7, 8, 9) mają blat 1.45f
            float desiredY_high_tables = 1.45f + EGG_HALF_HEIGHT;

            // --- FUNKCJA WSPÓLNA DLA WYSOKICH STOŁÓW ---
            auto checkHighTableCollision = [&](const TableHitbox& t) {
                if (!standingOnSomething && isInsideXZ(eggPosition, t)) {
                    if (oldY >= desiredY_high_tables - 0.001f && eggPosition.y <= desiredY_high_tables && velocityY <= 0.0f) {
                        float fallDistance = maxFallHeight - desiredY_high_tables;
                        if (fallDistance < 0.0f) { fallDistance = 0.0f; }

                        // Logika uszkodzeń
                        if (fallDistance >= CRASH_HEIGHT_THRESHOLD) { currentState = GAME_STATE_CRASHED; crashStartTime = currentFrame; crackCount = MAX_CRACKS; }
                        else if (fallDistance >= CRACK_HEIGHT_THRESHOLD) { crackCount = glm::min(crackCount + 1, MAX_CRACKS); if (crackCount >= MAX_CRACKS) { currentState = GAME_STATE_CRASHED; crashStartTime = currentFrame; } updateCrackGeometry(crackCount); }

                        // Lądowanie
                        maxFallHeight = desiredY_high_tables;
                        eggPosition.y = desiredY_high_tables;
                        velocityY = 0.0f;
                        canJump = true;
                        standingOnSomething = true;
                    }
                }
                };

            // Sprawdzamy wszystkie stoły o wysokości 1.45f
            checkHighTableCollision(table2);
            checkHighTableCollision(table3);
            checkHighTableCollision(table4);
            checkHighTableCollision(table5);
            checkHighTableCollision(table6);
            checkHighTableCollision(table7);
            checkHighTableCollision(table8);
            checkHighTableCollision(table9);

            // === KOLIZJA Z ZIEMIĄ (Podłoga na 0.0f) ===
            if (!standingOnSomething) {
                if (eggPosition.y < 0.7f) { // 0.7f to środek jajka stojącego na Y=0
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

        // Czyszczenie ekranu (kolor tła i bufor głębokości)
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // Jasny błękit
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

            // Zabezpieczenie przed wchodzeniem kamery pod podłogę
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

        // --- RYSOWANIE PODŁOGI (UŻYCIE MODELU) ---
        // Zmieniamy na 1, żeby model używał swojej tekstury (zdefiniowanej w pliku .mtl/.obj)
        ourShader.setInt("useTexture", 1);

        // Zmieniamy kolor na biały, aby nie "farbować" tekstury trawy na zielono
        ourShader.setVec4("objectColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        // Deklarujemy zmienną 'model' PIERWSZY RAZ tutaj
        glm::mat4 model = glm::mat4(1.0f);
        // Opcjonalnie: Przesuwamy podłogę minimalnie w dół, żeby nie migotała z podstawami stołów
        model = glm::translate(model, glm::vec3(0.0f, -0.01f, 0.0f));
        ourShader.setMat4("model", model);

        // Rysujemy model załadowany z pliku (zamiast ręcznego glDrawArrays)
        floorModel.Draw(ourShader);


        // --- RYSOWANIE STOŁÓW ---
        ourShader.setInt("useTexture", 1);
        ourShader.setVec4("objectColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        // Stół 1 (Blat 0.68f, Środek X: -2.0f)
        // UWAGA: Tutaj usuwamy "glm::mat4" z początku, bo zmienna 'model' została utworzona wyżej (przy podłodze)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-2.0f, -0.05f, 0.0f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // Stół 2 (Blat 1.45f, Środek X: 0.0f)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.77f, 0.0f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // Stół 3 (Blat 1.45f, Środek X: 2.5f)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.5f, 0.77f, 0.0f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // Stół 4 (Lewo, Środek X: 5.0f, Z: -1.5f)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(5.0f, 0.77f, -1.5f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // Stół 5 (Prawo, Środek X: 7.5f, Z: 1.5f)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(7.5f, 0.77f, 1.5f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // Stół 6 (Lewo, Środek X: 10.0f, Z: -1.5f)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(10.0f, 0.77f, -1.5f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // Stół 7 (Prawo, Środek X: 12.5f, Z: 1.5f)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(12.5f, 0.77f, 1.5f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // Stół 8 (Lewo, Środek X: 15.0f, Z: -1.5f)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(15.0f, 0.77f, -1.5f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // Stół 9 (Podejście, Środek X: 18.0f, Z: 0.0f)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(18.0f, 0.77f, 0.0f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // Stół 10 (Testowy, Środek X: 18.0f, Z: 2.5f, Niski)
        model = glm::mat4(1.0f);
        // Przesuwamy w dół (-0.05f), bo model jest tak wycentrowany dla niskich stołów (jak table1)
        model = glm::translate(model, glm::vec3(18.0f, -0.05f, 2.5f));
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
            ourShader.setVec4("objectColor", glm::vec4(1.0f, 0.9f, 0.7f, 1.0f));
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(eggIndices.size()), GL_UNSIGNED_INT, 0);

            // Rysowanie pęknięć (czarne linie)
            if (crackCount > 0 && !crackVertices.empty()) {
                glBindVertexArray(crackVAO);
                ourShader.setMat4("model", model);
                ourShader.setVec4("objectColor", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
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
                ourShader.setVec4("objectColor", glm::vec4(1.0f, 0.9f, 0.7f, 1.0f));

                for (int i = 0; i < 5; ++i) {
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, eggPosition);
                    // Fizyka wybuchu fragmentów
                    float explosionForce = 3.0f * (1.0f - ratio);
                    glm::vec3 fragmentOffset = fragments[i] * explosionForce * timeElapsed;
                    fragmentOffset.y -= 5.0f * timeElapsed * timeElapsed; // Grawitacja fragmentów
                    model = glm::translate(model, fragmentOffset);
                    float scaleFactor = glm::mix(0.1f, 0.0f, ratio); // Fragmenty znikają
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
        ourShader.setVec4("objectColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // Białe chmury
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

        // --- RYSOWANIE MOSTU ---
        if (glassBridge) {
            // Włączamy przezroczystość (ważne dla szkła, nawet z teksturą)
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Jajko po Mce XD
            //glDepthMask(GL_FALSE); 

            glassBridge->Draw(ourShader);

            //glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }

        // <--- DODANO: RYSOWANIE TRAMPOLINY
        if (bouncyTrampoline) {
            bouncyTrampoline->Draw(ourShader);
        }

        // <--- RYSOWANIE PODUSZKI (BEZPIECZNA STREFA)
        // Hitbox safeZone: X od 43 do 47 (Szerokość 4), Z od -2 do 2 (Głębokość 4), Y=15

        // 1. Ustawienia shadera
        ourShader.setInt("useTexture", 1); // Włącz tekstury (użyje pillow.jpg z mtl)
        ourShader.setVec4("objectColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        // 2. Obliczanie pozycji (środek hitboxa)
        float pillowCenterX = (safeZone.minX + safeZone.maxX) / 2.0f; // Powinno wyjść 45.0f
        float pillowCenterZ = (safeZone.minZ + safeZone.maxZ) / 2.0f; // Powinno wyjść 0.0f
        float pillowY = safeZone.topY; // Powierzchnia na 15.0f

        // 3. Macierz modelu
        model = glm::mat4(1.0f);

        // Przesuwamy w miejsce docelowe. 
        // Dodajemy +0.15f do Y, bo model poduszki ma środek w 0,0,0, a wysokość 0.3. 
        // Chcemy żeby leżała NA stole (o ile tam jest stół) lub wisiała w powietrzu jako magiczna platforma.
        model = glm::translate(model, glm::vec3(pillowCenterX, pillowY - 0.15f, pillowCenterZ));

        // Skalujemy! Model ma wymiary 1x1. SafeZone ma 4x4.
        // Zrobimy ją trochę mniejszą niż hitbox, żeby nie wystawała dziwnie
        model = glm::scale(model, glm::vec3(4.0f, 1.0f, 4.0f));

        ourShader.setMat4("model", model);
        pillowModel.Draw(ourShader);

        // --- RYSOWANIE RUCHOMEJ PLATFORMY ---
        // Obliczamy aktualną pozycję do rysowania
        glm::vec3 drawPos = glm::mix(level2Platform.startPos, level2Platform.endPos, level2Platform.progress);

        model = glm::mat4(1.0f);
        // Przesuwamy model tam gdzie jest platforma + korekta wysokości modelu stołu (-0.68f)
        model = glm::translate(model, glm::vec3(drawPos.x, drawPos.y - 0.68f, drawPos.z));

        // Skalujemy x2, żeby była duża i łatwo było trafić (wymiar ok 3.2 x 3.2)
        model = glm::scale(model, glm::vec3(2.0f, 1.0f, 2.0f));

        ourShader.setMat4("model", model);
        ourShader.setInt("useTexture", 1); // Użyj tekstury drewna ze stołu
        tableModel.Draw(ourShader);

        // --- RYSOWANIE UI (MENU) ---
        // Włączamy przezroczystość dla tekstury menu (kanał Alpha)
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
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Pokaż kursor
            }
        }

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST); // Przywracamy sprawdzanie głębokości dla 3D

        glfwSwapBuffers(window); // Podmiana buforów ekranu
        glfwPollEvents();        // Sprawdzenie zdarzeń systemowych (klawiatura, mysz)
    }

    // === ZWOLNIENIE ZASOBÓW (CLEANUP) ===
    glDeleteVertexArrays(1, &eggVAO); glDeleteBuffers(1, &eggVBO); glDeleteBuffers(1, &eggEBO);
    //glDeleteVertexArrays(1, &floorVAO); glDeleteBuffers(1, &floorVBO);
    glDeleteVertexArrays(1, &cubeVAO); glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &crackVAO); glDeleteBuffers(1, &crackVBO);
    glDeleteVertexArrays(1, &sphereVAO); glDeleteBuffers(1, &sphereVBO); glDeleteBuffers(1, &sphereEBO);
    glDeleteVertexArrays(1, &uiVAO); glDeleteBuffers(1, &uiVBO);
    glDeleteTextures(1, &menuTexture);
    delete uiShader;
    // <--- DODANO: Usuwanie wskaźnika
    delete bouncyTrampoline;

    glfwTerminate(); // Zamknięcie GLFW
    return 0;
}

// Funkcja obsługująca wejście z klawiatury
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // MENU -> GRA
    if (currentState == GAME_STATE_MENU) {
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            currentState = GAME_STATE_PLAYING;
            needsReset = true; // Zresetuj pozycję przy starcie
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Ukryj kursor
            firstMouse = true;
        }
    }
    // ŚMIERĆ -> MENU
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

        // Obliczanie kierunków ruchu względem kamery
        glm::vec3 forward = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z)); // Ruch tylko w płaszczyźnie poziomej
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

// Callback ruchu myszką (obracanie kamerą)
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (currentState != GAME_STATE_PLAYING) return;

    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos); // Odwrócone, bo Y rośnie w dół ekranu
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Blokada kamery góra/dół (żeby nie zrobić fikołka)
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Obliczanie wektora kierunku kamery
    glm::vec3 direction;
    direction.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    direction.y = std::sin(glm::radians(pitch));
    direction.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}