#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

// Prototypy funkcji
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

// Ustawienia okna
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

// Kamera i postaæ
// ZMIANA: Jajko startuje dalej od œrodka, aby widzieæ szeœcian
glm::vec3 eggPosition = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraDistance = 5.0f;

// Myszka
float lastX;
float lastY;
bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;

// Czas
float deltaTime = 0.0f;
float lastFrame = 0.0f;
// NOWOŒÆ: Fizyka jajka
float velocityY = 0.0f; // Pionowa prêdkoœæ jajka
bool canJump = true;   // Czy jajko jest na ziemi i mo¿e skoczyæ?
const float GRAVITY = -9.8f;
const float JUMP_FORCE = 5.0f;


// Klasa Shader (bez zmian)
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
        int success; char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) { glGetShaderInfoLog(shader, 1024, NULL, infoLog); std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl; }
        }
        else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) { glGetProgramInfoLog(shader, 1024, NULL, infoLog); std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl; }
        }
    }
};


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
    if (window == NULL) { std::cout << "Blad tworzenia okna GLFW!" << std::endl; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cout << "Blad inicjalizacji GLAD!" << std::endl; return -1; }

    glEnable(GL_DEPTH_TEST);

    Shader ourShader("vertex_shader.glsl", "fragment_shader.glsl");

    // === Konfiguracja JAJKA ===
    std::vector<float> eggVertices;
    std::vector<unsigned int> eggIndices;
    int eggSegments = 40;
    for (int i = 0; i <= eggSegments; ++i) { float theta = glm::pi<float>() * i / eggSegments; for (int j = 0; j <= eggSegments; ++j) { float phi = 2 * glm::pi<float>() * j / eggSegments; float x = 0.5f * sin(theta) * cos(phi); float y = 0.7f * cos(theta); float z = 0.5f * sin(theta) * sin(phi); eggVertices.push_back(x); eggVertices.push_back(y); eggVertices.push_back(z); } }
    for (int i = 0; i < eggSegments; ++i) { for (int j = 0; j < eggSegments; ++j) { int first = (i * (eggSegments + 1)) + j; int second = first + eggSegments + 1; eggIndices.push_back(first); eggIndices.push_back(second); eggIndices.push_back(first + 1); eggIndices.push_back(second); eggIndices.push_back(second + 1); eggIndices.push_back(first + 1); } }

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

    // === NOWOŒÆ: Konfiguracja SZEŒCIANU NA ŒRODKU ===
    float cubeVertices[] = {
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f
    };
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    // Pêtla renderowania
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // === NOWOŒÆ: Symulacja fizyki skoku ===
        // 1. Zastosuj grawitacjê do prêdkoœci pionowej
        velocityY += GRAVITY * deltaTime;
        // 2. Zmieñ pozycjê Y jajka na podstawie prêdkoœci pionowej
        eggPosition.y += velocityY * deltaTime;
        // 3. SprawdŸ kolizjê z pod³og¹
        if (eggPosition.y < 0.0f) {
            eggPosition.y = 0.0f; // Ustaw jajko na pod³odze
            velocityY = 0.0f;     // Zatrzymaj spadanie
            canJump = true;       // Pozwól na kolejny skok
        }

        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::vec3 cameraPos = eggPosition - cameraFront * cameraDistance;
        glm::mat4 view = glm::lookAt(cameraPos, eggPosition, cameraUp);
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // --- Rysowanie POD£OGI ---
        glBindVertexArray(floorVAO);
        ourShader.setMat4("model", glm::mat4(1.0f));
        ourShader.setVec3("objectColor", glm::vec3(0.2f, 0.6f, 0.1f));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // --- NOWOŒÆ: Rysowanie SZEŒCIANU ---
        glBindVertexArray(cubeVAO);
        glm::mat4 model = glm::mat4(1.0f);
        // Przesuwamy go lekko w górê, aby sta³ NA pod³odze, a nie w niej
        model = glm::translate(model, glm::vec3(0.0f, -0.2f, 0.0f));
        // Powiêkszamy go, ¿eby by³ bardziej widoczny
        model = glm::scale(model, glm::vec3(4.0f));
        ourShader.setMat4("model", model);
        ourShader.setVec3("objectColor", glm::vec3(0.5f, 0.5f, 0.5f)); // Szary kolor
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // --- Rysowanie JAJKA ---
        glBindVertexArray(eggVAO);
        model = glm::translate(glm::mat4(1.0f), eggPosition);
        ourShader.setMat4("model", model);
        ourShader.setVec3("objectColor", glm::vec3(1.0f, 0.9f, 0.7f));
        glDrawElements(GL_TRIANGLES, eggIndices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Zwolnienie zasobów
    glDeleteVertexArrays(1, &eggVAO);
    glDeleteBuffers(1, &eggVBO);
    glDeleteBuffers(1, &eggEBO);
    glDeleteVertexArrays(1, &floorVAO);
    glDeleteBuffers(1, &floorVBO);
    // NOWOŒÆ: Zwolnienie zasobów szeœcianu
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);

    glfwTerminate();
    return 0;
}

// Funkcje callback (bez zmian)
// ZMIANA: Zaktualizowano funkcjê processInput
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // NOWOŒÆ: Logika sprintu
    const float WALK_SPEED = 2.5f;
    const float SPRINT_SPEED = 5.0f;
    float currentSpeed = WALK_SPEED;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        currentSpeed = SPRINT_SPEED;
    }

    // Ruch lewo/prawo/przód/ty³
    glm::vec3 forward = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 right = glm::normalize(glm::cross(forward, cameraUp));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) eggPosition += forward * currentSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) eggPosition -= forward * currentSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) eggPosition -= right * currentSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) eggPosition += right * currentSpeed * deltaTime;

    // NOWOŒÆ: Logika skoku
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && canJump) {
        velocityY = JUMP_FORCE; // Nadaj prêdkoœæ pionow¹
        canJump = false;        // Zablokuj mo¿liwoœæ kolejnego skoku w powietrzu
    }
}
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn); float ypos = static_cast<float>(yposIn);
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX; float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;
    float sensitivity = 0.1f; xoffset *= sensitivity; yoffset *= sensitivity;
    yaw += xoffset; pitch += yoffset;
    if (pitch > 89.0f) pitch = 89.0f; if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}