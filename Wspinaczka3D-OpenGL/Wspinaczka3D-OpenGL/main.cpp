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


// Klasa Shader
class Shader {
public:
    unsigned int ID;
    Shader(const char* vertexPath, const char* fragmentPath) {
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            vShaderFile.close();
            fShaderFile.close();
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();
        unsigned int vertex, fragment;
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }
    void use() { glUseProgram(ID); }
    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    void setVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

private:
    void checkCompileErrors(unsigned int shader, std::string type) {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};

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

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Wspinaczka3D", primaryMonitor, NULL);
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

    // Pêtla renderowania
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // === FIZYKA I LOGIKA GRY (TYLKO W STANIE 'PLAYING') ===
        if (currentState == GAME_STATE_PLAYING)
        {
            velocityY += GRAVITY * deltaTime;
            eggPosition.y += velocityY * deltaTime;
            if (eggPosition.y < 0.0f) {
                eggPosition.y = 0.0f;
                velocityY = 0.0f;
                canJump = true;
            }

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
        } // === KONIEC BLOKU if (currentState == GAME_STATE_PLAYING) ===

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
        ourShader.setMat4("model", glm::mat4(1.0f));
        ourShader.setVec3("objectColor", glm::vec3(0.2f, 0.6f, 0.1f));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Rysowanie szeœcianu
        glBindVertexArray(cubeVAO);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -0.2f, 0.0f));
        model = glm::scale(model, glm::vec3(4.0f));
        ourShader.setMat4("model", model);
        ourShader.setVec3("objectColor", glm::vec3(0.5f, 0.5f, 0.5f));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Rysowanie jajka
        glBindVertexArray(eggVAO);
        model = glm::translate(glm::mat4(1.0f), eggPosition);
        ourShader.setMat4("model", model);
        ourShader.setVec3("objectColor", glm::vec3(1.0f, 0.9f, 0.7f));
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(eggIndices.size()), GL_UNSIGNED_INT, 0);

        // Rysowanie chmur
        glBindVertexArray(sphereVAO);
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