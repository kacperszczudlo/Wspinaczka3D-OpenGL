#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <random> // Do generowania liczb losowych
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Model.h"
#include "Mesh.h"
#include "Shader.h"



// Prototypy funkcji
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

// Ustawienia okna
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

// NOWE STANY GRY
enum GameState {
    GAME_STATE_MENU,
    GAME_STATE_PLAYING
};
GameState currentState = GAME_STATE_MENU;

// NOWE ZMIENNE GLOBALNE DLA UI
Shader* uiShader = nullptr;
unsigned int uiVAO, uiVBO;
unsigned int menuTexture;

// Kamera i postaæ
glm::vec3 eggPosition = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 previousEggPosition = eggPosition;
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraDistance = 7.0f; // Lekko oddalamy kamerê, by lepiej widzieæ scenê

// Myszka
float lastX;
float lastY;
bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;

// Czas
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Fizyka jajka
float velocityY = 0.0f;
bool canJump = true;
const float GRAVITY = -9.8f;
const float JUMP_FORCE = 5.0f;

// "Wysokoœæ" jajka – œrodek jajka jest na y=0, dó³ jest na ok. -0.7
const float EGG_HALF_HEIGHT = 0.7f;

struct TableHitbox {
    float minX, maxX;
    float minZ, maxZ;
    float topY;
};

TableHitbox table1 = {
    -0.8f, 0.8f,  // minX, maxX
    -0.8f, 0.8f,  // minZ, maxZ
    -0.02f        // topY
};

TableHitbox table2 = {
    2.12f, 3.85f,   // minX, maxX
    -0.84f, 0.86f,  // minZ, maxZ
    0.75f        // topY
};

bool isInsideXZ(const glm::vec3& pos, const TableHitbox& t) {
    return pos.x > t.minX && pos.x < t.maxX &&
        pos.z > t.minZ && pos.z < t.maxZ;
}

// Struktury do zarz¹dzania chmurami
struct CloudComponent {
    glm::vec3 offset; // Przesuniêcie wzglêdem œrodka chmury
    float scale;
};

struct Cloud {
    glm::vec3 position;
    glm::vec3 velocity;
    std::vector<CloudComponent> components;
};

std::vector<Cloud> clouds; // Wektor przechowuj¹cy wszystkie aktywne chmury
float cloudSpawnTimer = 0.0f;




// Funkcja do generowania losowej liczby zmiennoprzecinkowej w zakresie
float randomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

// Funkcja do tworzenia nowej chmury
void spawnCloud() {
    Cloud newCloud;
    // Pozycja startowa chmury (daleko, z boku)
    newCloud.position = glm::vec3(randomFloat(-50.0f, 50.0f), randomFloat(10.0f, 20.0f), -70.0f);
    // Prêdkoœæ chmury (wolno do przodu)
    newCloud.velocity = glm::vec3(0.0f, 0.0f, randomFloat(1.5f, 3.0f));

    // Generowanie losowych "kawa³ków" chmury
    int numComponents = rand() % 4 + 3; // Od 3 do 6 kul na chmurê
    for (int i = 0; i < numComponents; ++i) {
        CloudComponent component;
        component.offset = glm::vec3(randomFloat(-2.5f, 2.5f), randomFloat(-1.0f, 1.0f), randomFloat(-1.5f, 1.5f));
        component.scale = randomFloat(1.5f, 2.5f);
        newCloud.components.push_back(component);
    }
    clouds.push_back(newCloud);
}


// NOWA FUNKCJA DO £ADOWANIA TEKSTURY
unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA; // Nasz plik PNG powinien mieæ RGBA (przezroczystoœæ)

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        std::cout << "Blad ladowania tekstury: " << path << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}

int main()
{
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
    // Domyœlnie startujemy z widocznym kursorem dla menu
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Blad inicjalizacji GLAD!" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // Inicjalizacja generatora liczb losowych
    srand(static_cast<unsigned int>(time(0)));

    Shader ourShader("vertex_shader.glsl", "fragment_shader.glsl");

    // === INICJALIZACJA UI (NOWY KOD) ===
    uiShader = new Shader("ui_vertex.glsl", "ui_fragment.glsl");
    // Ustaw macierz projekcji ortograficznej
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
    uiShader->use();
    uiShader->setMat4("projection", projection);
    uiShader->use(); // U¿yj ponownie, aby ustawiæ uniform
    glUniform1i(glGetUniformLocation(uiShader->ID, "uiTexture"), 0);


    // Definiujemy prostok¹t (quad) na dole ekranu
    // Mo¿esz dostosowaæ te wartoœci, aby zmieniæ pozycjê napisu
    float width = 500.0f;
    float height = 500.0f;

    // Teraz oblicz pozycjê na podstawie rozmiaru
    float x_pos = (SCR_WIDTH / 2.0f) - (width / 2.0f);
    float y_pos = (SCR_HEIGHT / 2.0f) - (height / 2.0f);

    // x, y, tex_u, tex_v
    float uiVertices[] = {
        x_pos,       y_pos + height,   0.0f, 1.0f,
        x_pos,       y_pos,            0.0f, 0.0f,
        x_pos + width, y_pos,            1.0f, 0.0f,

        x_pos,       y_pos + height,   0.0f, 1.0f,
        x_pos + width, y_pos,            1.0f, 0.0f,
        x_pos + width, y_pos + height,   1.0f, 1.0f
    };

    glGenVertexArrays(1, &uiVAO);
    glGenBuffers(1, &uiVBO);
    glBindVertexArray(uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uiVertices), &uiVertices, GL_STATIC_DRAW);
    // atrybut pozycji (vec2)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // atrybut tekstury (vec2)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0); // Od³¹cz VAO

    // Wczytaj teksturê menu
    menuTexture = loadTexture("menu_prompt.png");
    // === KONIEC INICJALIZACJI UI ===

    // === Konfiguracja JAJKA ===
    std::vector<float> eggVertices;
    std::vector<unsigned int> eggIndices;
    int eggSegments = 40;
    for (int i = 0; i <= eggSegments; ++i) {
        float theta = glm::pi<float>() * i / eggSegments;
        for (int j = 0; j <= eggSegments; ++j) {
            float phi = 2 * glm::pi<float>() * j / eggSegments;
            float x = 0.5f * sin(theta) * cos(phi);
            float y = 0.7f * cos(theta);
            float z = 0.5f * sin(theta) * sin(phi);
            eggVertices.push_back(x);
            eggVertices.push_back(y);
            eggVertices.push_back(z);
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
    glBufferData(GL_ARRAY_BUFFER, eggVertices.size() * sizeof(float), &eggVertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eggEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, eggIndices.size() * sizeof(unsigned int), &eggIndices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // === Konfiguracja POD£OGI ===
    float floorVertices[] = { 50.0f, -0.7f,  50.0f, -50.0f, -0.7f,  50.0f, -50.0f, -0.7f, -50.0f, 50.0f, -0.7f,  50.0f, -50.0f, -0.7f, -50.0f, 50.0f, -0.7f, -50.0f };
    unsigned int floorVAO, floorVBO;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), &floorVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // === Konfiguracja SZEŒCIANU ===
    float cubeVertices[] = { -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f };
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // === Konfiguracja geometrii dla KULI (chmury) ===
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    int segments = 20;
    for (int i = 0; i <= segments; ++i) {
        for (int j = 0; j <= segments; ++j) {
            float theta = glm::pi<float>() * i / segments;
            float phi = 2 * glm::pi<float>() * j / segments;
            sphereVertices.push_back(cos(phi) * sin(theta));
            sphereVertices.push_back(cos(theta));
            sphereVertices.push_back(sin(phi) * sin(theta));
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
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), &sphereVertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), &sphereIndices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // === £ADOWANIE MODELU 3D (np. œciana wspinaczkowa) ===
    Model tableModel("models/table.obj");  

    // Pêtla renderowania
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        previousEggPosition = eggPosition;
        processInput(window);

        std::cout << "Egg: x=" << eggPosition.x
            << " y=" << eggPosition.y
            << " z=" << eggPosition.z << std::endl;

        // === FIZYKA I LOGIKA GRY (TYLKO W STANIE 'PLAYING') ===
        if (currentState == GAME_STATE_PLAYING)
        {
            float oldY = eggPosition.y;

            // --- GRAWITACJA ---
            velocityY += GRAVITY * deltaTime;
            eggPosition.y += velocityY * deltaTime;

            bool standingOnSomething = false;

            // === STO£Y – l¹dowanie TYLKO z góry ===
            // STÓ£ 1
            if (isInsideXZ(eggPosition, table1)) {
                float desiredY1 = table1.topY + EGG_HALF_HEIGHT;

                if (oldY >= desiredY1 - 0.05f && eggPosition.y <= desiredY1 && velocityY <= 0.0f) {
                    eggPosition.y = desiredY1;
                    velocityY = 0.0f;
                    canJump = true;
                    standingOnSomething = true;
                }
            }

            // STÓ£ 2
            if (!standingOnSomething && isInsideXZ(eggPosition, table2)) {
                float desiredY2 = table2.topY + EGG_HALF_HEIGHT;

                if (oldY >= desiredY2 - 0.05f && eggPosition.y <= desiredY2 && velocityY <= 0.0f) {
                    eggPosition.y = desiredY2;
                    velocityY = 0.0f;
                    canJump = true;
                    standingOnSomething = true;
                }
            }

            // === ZIEMIA – jeœli nie stoisz na ¿adnym stole ===
            if (!standingOnSomething) {
                if (eggPosition.y < 0.0f) {
                    eggPosition.y = 0.0f;
                    velocityY = 0.0f;
                    canJump = true;
                }
            }

            // --- CHMURY (jak mia³eœ) ---
            cloudSpawnTimer += deltaTime;
            if (cloudSpawnTimer > 2.0f && clouds.size() < 15) {
                spawnCloud();
                cloudSpawnTimer = 0.0f;
            }

            for (int i = 0; i < clouds.size(); ++i) {
                clouds[i].position += clouds[i].velocity * deltaTime;
                if (clouds[i].position.z > 50.0f) {
                    clouds.erase(clouds.begin() + i);
                    i--;
                }
            }
        }



		// Renderowanie

        glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // Ta linijka zostaje bez zmian

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 150.0f);

        glm::mat4 view;
        if (currentState == GAME_STATE_PLAYING)
        {
            // Kamera pod¹¿aj¹ca za graczem
            glm::vec3 cameraPos = eggPosition - cameraFront * cameraDistance;

            // Twoja istniej¹ca poprawka kamery
            const float minCameraHeight = -0.5f;
            if (cameraPos.y < minCameraHeight)
            {
                cameraPos.y = minCameraHeight;
            }
            view = glm::lookAt(cameraPos, eggPosition, cameraUp);
        }
        else // currentState == GAME_STATE_MENU
        {
            // Statyczna kamera w menu, patrz¹ca na scenê
            glm::vec3 menuCameraPos = glm::vec3(0.0f, 5.0f, 15.0f); // Mo¿esz dostosowaæ
            // Patrzymy na platformê startow¹ (szeœcian jest na y=-0.2)
            view = glm::lookAt(menuCameraPos, glm::vec3(0.0f, 2.0f, 0.0f), cameraUp);
        }

        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

       

        // Rysowanie pod³ogi
        glBindVertexArray(floorVAO);
        ourShader.setInt("useTexture", 0);
        ourShader.setMat4("model", glm::mat4(1.0f));
        ourShader.setVec3("objectColor", glm::vec3(0.2f, 0.6f, 0.1f));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // === STO£Y ===
        ourShader.setInt("useTexture", 1);

        // STÓ£ 1 (jak do tej pory, na œrodku)
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -0.75f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);

        // STÓ£ 2 (np. w prawo, ¿eby mo¿na by³o doskoczyæ)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(3.0f, 0.0f, 0.0f)); // <- przesuniêty w X
        model = glm::scale(model, glm::vec3(1.0f));
        ourShader.setMat4("model", model);
        tableModel.Draw(ourShader);


        // Rysowanie jajka
        glBindVertexArray(eggVAO);
        ourShader.setInt("useTexture", 0);
        model = glm::translate(glm::mat4(1.0f), eggPosition);
        ourShader.setMat4("model", model);
        ourShader.setVec3("objectColor", glm::vec3(1.0f, 0.9f, 0.7f));
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(eggIndices.size()), GL_UNSIGNED_INT, 0);

        // Rysowanie chmur
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

        // === RENDEROWANIE UI 2D (NOWY KOD) ===
// Musimy w³¹czyæ blending (przezroczystoœæ) i wy³¹czyæ test g³êbi
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST); // UI jest zawsze na wierzchu

        if (currentState == GAME_STATE_MENU)
        {
            uiShader->use();
            glBindVertexArray(uiVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, menuTexture);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }

        // Przywróæ stany OpenGL
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST); // W³¹cz z powrotem dla sceny 3D
        // === KONIEC RENDEROWANIA UI ===

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

    // --- LOGIKA DLA MENU ---
    if (currentState == GAME_STATE_MENU)
    {
        // Jeœli wciœniêto Enter, przejdŸ do stanu gry
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
        {
            currentState = GAME_STATE_PLAYING;
            // Ukryj kursor i zablokuj go w oknie
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            // Zresetuj pozycjê myszy, aby unikn¹æ "skoku" kamery
            firstMouse = true;
        }
    }
    // --- LOGIKA DLA GRY ---
    else if (currentState == GAME_STATE_PLAYING)
    {
        // Ca³a dotychczasowa obs³uga ruchu
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

        // === BLOKOWANIE WEJŒCIA BOKIEM NA STO£Y ===
        bool insideTable1XZ = isInsideXZ(eggPosition, table1);
        bool insideTable2XZ = isInsideXZ(eggPosition, table2);

        // je¿eli próbujemy wejœæ w hitbox sto³u, bêd¹c poni¿ej jego blatu,
        // cofamy XZ – trzeba u¿yæ skoku, ¿eby znaleŸæ siê nad sto³em
        if (insideTable1XZ && eggPosition.y < table1.topY + EGG_HALF_HEIGHT - 0.05f) {
            eggPosition.x = previousEggPosition.x;
            eggPosition.z = previousEggPosition.z;
        }
        if (insideTable2XZ && eggPosition.y < table2.topY + EGG_HALF_HEIGHT - 0.05f) {
            eggPosition.x = previousEggPosition.x;
            eggPosition.z = previousEggPosition.z;
        }
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    // Ignoruj ruch myszy, jeœli nie jesteœmy w grze
    if (currentState != GAME_STATE_PLAYING) {
        return;
    }

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    yaw += xoffset;
    pitch += yoffset;
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}