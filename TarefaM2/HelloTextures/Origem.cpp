#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "SceneObject.cpp"
#include "Shader.h"

const GLuint WIDTH = 1000, HEIGHT = 1000;

bool rotateX = false, rotateY = false, rotateZ = false;
bool translateX = false, translateY = false, translateZ = false;
int translateDirectionX = 0, translateDirectionY = 0, translateDirectionZ = 0;
float scale = 1.0f;

void adjustScale(int key)
{
    const float scaleFactor = 0.05f;
    if (key == GLFW_KEY_KP_ADD)
        scale += scale * scaleFactor;
    else if (key == GLFW_KEY_KP_SUBTRACT)
        scale -= scale * scaleFactor;
}

void adjustRotation(int key)
{
    if (key == GLFW_KEY_X) {
        rotateX = true;
        rotateY = rotateZ = false;
    } else if (key == GLFW_KEY_Y) {
        rotateY = true;
        rotateX = rotateZ = false;
    } else if (key == GLFW_KEY_Z) {
        rotateZ = true;
        rotateX = rotateY = false;
    }
}

void adjustTranslation(int key)
{
    if (key == GLFW_KEY_RIGHT) {
        translateX = true;
        translateDirectionX = 1;
    } else if (key == GLFW_KEY_LEFT) {
        translateX = true;
        translateDirectionX = -1;
    } else if (key == GLFW_KEY_UP) {
        translateY = true;
        translateDirectionY = 1;
    } else if (key == GLFW_KEY_DOWN) {
        translateY = true;
        translateDirectionY = -1;
    } else if (key == GLFW_KEY_W) {
        translateZ = true;
        translateDirectionZ = 1;
    } else if (key == GLFW_KEY_S) {
        translateZ = true;
        translateDirectionZ = -1;
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (action == GLFW_PRESS) {
        adjustScale(key);
        adjustRotation(key);
        adjustTranslation(key);
    }
}

void resetTranslationVariables()
{
    translateX = translateY = translateZ = false;
    translateDirectionX = translateDirectionY = translateDirectionZ = 0;
}

std::vector<SceneObject> generateObjects(int numObjects, GLuint vertexArrayObject, int numVertices, Shader* shader)
{
    std::vector<SceneObject> objects;
    const float horizontalSpacing = 2.75f;

    for (int i = 0; i < numObjects; ++i)
    {
        float xPosition = (i % 2 == 0) ? (-horizontalSpacing * (i / 2)) : (horizontalSpacing * ((i / 2) + 1));
        objects.emplace_back(vertexArrayObject, numVertices, shader, glm::vec3(xPosition, 0.0f, 0.0f));
    }

    return objects;
}

bool initializeBuffers(GLuint& VBO, GLuint& VAO, const std::vector<GLfloat>& vbuffer)
{
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vbuffer.size() * sizeof(GLfloat), vbuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

bool readOBJFile(const std::string& filepath, std::vector<glm::vec3>& vertices, std::vector<GLuint>& indices, std::vector<GLfloat>& vbuffer)
{
    std::vector<glm::vec3> colors = {
        {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f}
    };

    std::ifstream inputFile(filepath);
    if (!inputFile.is_open())
    {
        std::cerr << "Error opening OBJ file: " << filepath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(inputFile, line))
    {
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "v")
        {
            glm::vec3 v;
            ssline >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        else if (word == "f")
        {
            std::string tokens[3];
            ssline >> tokens[0] >> tokens[1] >> tokens[2];

            for (const std::string& token : tokens)
            {
                int posLastValue = token.find_last_of('/');
                int normal = std::stoi(token.substr(posLastValue + 1));
                glm::vec3 normalColors = colors[normal - 1];

                int pos = token.find('/');
                int index = std::stoi(token.substr(0, pos)) - 1;
                indices.push_back(index);

                vbuffer.push_back(vertices[index].x);
                vbuffer.push_back(vertices[index].y);
                vbuffer.push_back(vertices[index].z);
                vbuffer.push_back(normalColors.r);
                vbuffer.push_back(normalColors.g);
                vbuffer.push_back(normalColors.b);
            }
        }
    }

    inputFile.close();
    return true;
}

int loadSimpleOBJ(const std::string& filepath, int& numVertices)
{
    std::vector<glm::vec3> vertices;
    std::vector<GLuint> indices;
    std::vector<GLfloat> vbuffer;

    if (!readOBJFile(filepath, vertices, indices, vbuffer))
    {
        std::cerr << "Error reading OBJ file: " << filepath << std::endl;
        return -1;
    }

    numVertices = vbuffer.size() / 6;

    GLuint VBO, VAO;
    if (!initializeBuffers(VBO, VAO, vbuffer))
    {
        std::cerr << "Error initializing vertex and array buffers." << std::endl;
        return -1;
    }

    return VAO;
}

int main()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Tarefa M2 - Leonardo Feilke!", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL version supported: " << glGetString(GL_VERSION) << std::endl;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    Shader shader("VShader.vs", "FShader.fs");
    glUseProgram(shader.ID);

    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("view", glm::value_ptr(view));

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / height, 0.1f, 100.0f);
    shader.setMat4("projection", glm::value_ptr(projection));

    glEnable(GL_DEPTH_TEST);

    int numVertices;
    GLuint VAO = loadSimpleOBJ("../../3D_models/Cube/cube.obj", numVertices);

    int numObjects = 4;
    std::vector<SceneObject> objects = generateObjects(numObjects, VAO, numVertices, &shader);

    while (!glfwWindowShouldClose(window))
    {
        resetTranslationVariables();
        glfwPollEvents();

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLineWidth(10);
        glPointSize(20);

        for (auto& obj : objects)
        {
            if (rotateX) obj.rotateX();
            if (rotateY) obj.rotateY();
            if (rotateZ) obj.rotateZ();

            if (translateX) obj.translateX(translateDirectionX);
            if (translateY) obj.translateY(translateDirectionY);
            if (translateZ) obj.translateZ(translateDirectionZ);

            obj.setScale(glm::vec3(scale, scale, scale));
            obj.updateModelMatrix();
            obj.renderObject();
        }

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}
