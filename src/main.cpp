#include <iostream>
#include <filesystem> 
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"
#include "include/camera.hpp"
#include "include/shader.hpp"
#include "include/model.hpp"
#include "include/skybox.hpp"

// ———————— 全局变量 ————————
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ———————— 回调函数 ————————
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
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

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // 控制 Zoom（FOV）
    // camera.ProcessMouseScroll(static_cast<float>(yoffset));

    // 前后移动相机（沿视线方向）
    camera.Position += camera.Front * static_cast<float>(-yoffset) * 2.0f;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

}

// ———————— 主函数 ————————
int main() {
    // 初始化 GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Model Viewer (Assimp + GLEW)", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // 初始化 GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // 创建着色器（需准备 .vs 和 .fs 文件）
    Shader ourShader((SHADERS_FOLDER + "model.vs").c_str(), (SHADERS_FOLDER + "model.fs").c_str());
    
    // 初始化、加载模型
    // 准备赛车贴图
    std::map<std::string, std::string> carTextureMap = {
        {"*0", "2015_mclaren_p1_gtr_wheel.etc_0.png"},
        {"*1", "car_tyre_slick_pirelli_02.etc_1.png"},
        {"*2", "2015_mclaren_p1_gtr_misc.etc_2.png"},
        {"*3", "car_rotor_03.etc_3.png"},
        {"*4", "car_windows.etc_4.png"},
        {"*5", "2015_mclaren_p1_gtr_ext_51.etc_5.png"},
        {"*6", "2015_mclaren_p1_gtr_cab.etc_6.png"},
        {"*7", "2015_mclaren_p1_gtr_lights.etc_7.png"},
        {"*8", "2015_mclaren_p1_gtr_badges.etc_8.png"},
        {"*9", "car_chassis.etc_9.png"}
    };
    // 猫模型是obj和mtl所以textureMap可以为空
    std::map<std::string, std::string> cattextureMap;

    Model cat1((ASSETS_FOLDER+"cat/Cat1/12221_Cat_v1_l3.obj").c_str(), (ASSETS_FOLDER + "cat/Cat1/").c_str(),cattextureMap);
    Model mclaren((ASSETS_FOLDER+"car/2015 McLaren P1 GTR.glb").c_str(), (ASSETS_FOLDER + "car/texture").c_str(), carTextureMap);

    // 初始化、加载天空盒
    // face
    std::vector<std::string> faces = {
        PROJECT_ROOT + "/src/assets/skybox/right.jpg",
        PROJECT_ROOT + "/src/assets/skybox/left.jpg",
        PROJECT_ROOT + "/src/assets/skybox/top.jpg",
        PROJECT_ROOT + "/src/assets/skybox/bottom.jpg",
        PROJECT_ROOT + "/src/assets/skybox/front.jpg",
        PROJECT_ROOT + "/src/assets/skybox/back.jpg"
    };
    Skybox skybox(faces);

    // 渲染循环
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        // ------------模型可以共享的设置-------------
        // 设置 MVP
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        // 光照
        ourShader.setVec3("viewPos", camera.Position);
        ourShader.setVec3("light.position", glm::vec3(0.0f, 1.0f, 10.0f));
        ourShader.setVec3("light.color", glm::vec3(1.0f, 1.0f, 1.0f));


        // ------------画猫的模型---------------------
        // 设置模型矩阵
        glm::mat4 modelCat = glm::mat4(1.0f);
        // 先旋转后平移
        modelCat = glm::rotate(modelCat, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
        // 平移因子
        modelCat = glm::translate(modelCat, glm::vec3(-2.0f, -0.5f, -5.0f)); // 放在左边
        // 缩放因子
        modelCat = glm::scale(modelCat, glm::vec3(0.2f, 0.2f, 0.2f));
        ourShader.setMat4("model", modelCat);
        cat1.Draw(ourShader);

        // ------------画赛车的模型---------------------
        // 设置模型矩阵
        glm::mat4 modelCar = glm::mat4(1.0f);
        // 平移因子
        modelCar = glm::translate(modelCar, glm::vec3(2.0f, -1.0f, -5.0f)); // 放在右边，略低一点
        modelCar = glm::scale(modelCar, glm::vec3(0.8f, 0.8f, 0.8f));   // 赛车更小的缩放
        // 缩放因子
        ourShader.setMat4("model", modelCar);
        mclaren.Draw(ourShader);

        // ------------画天空盒（最后画天空盒！！不然其它模型会被它挡住）-----------------------
        skybox.draw(view, projection);
        // ------------画天空盒-----------------------------------------------------------------

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}