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
#include "include/terrain/terrain.hpp"
#include "include/car.hpp"

// ———————— 全局变量 ————————
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
bool cursorLocked = false;   // 默认锁定鼠标并控制视角
float eyeHeight = 10.0f;    // 初始相机高度

Camera camera(glm::vec3(0.0f, eyeHeight, 5.0f));
bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

Car myCar(glm::vec3(2.0f, 0.0f, -5.0f));
bool driveMode = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

//补充俯视模式状态
bool topView = false;   //是否处于俯视模式
glm::vec3 originalCameraPos;    //保存原始相机位置
glm::vec3 originalCameraFront;  //保存原始相机朝向
glm::vec3 originalCameraUp;     //保存原始相机上方向
float originalZoom;          //保存原始FOV

// ———————— 回调函数 ————————
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    if (!cursorLocked) {
        // 如果没有锁定鼠标，仅仅记录坐标，不更新相机视角
        lastX = static_cast<float>(xposIn);
        lastY = static_cast<float>(yposIn);
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

    if(!topView)
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // 控制 Zoom（FOV）
     /*camera.ProcessMouseScroll(static_cast<float>(yoffset));*/

     camera.Position += camera.Front * static_cast<float>(yoffset) * 2.0f;
}

void processInput(GLFWwindow* window, Car& myCar, Terrain& terrain) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // --- 2. 左Alt键：控制鼠标视角锁定 ---
    static bool altPressed = false;
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
        if (!altPressed) {
            cursorLocked = !cursorLocked;
            glfwSetInputMode(window, GLFW_CURSOR, cursorLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            firstMouse = true;
            altPressed = true;
        }
    }
    else { altPressed = false; }

    // --- 3. WASDQE 移动控制 ---
    // WASD 移动
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);

    // QE 修改相对于地面的高度
    float speed = camera.MovementSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) eyeHeight += speed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) eyeHeight -= speed;

    // 按 C 键：手动切换【自由探索】和【驾驶模式】
    static bool cPressed = false;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        if (!cPressed) {
            driveMode = !driveMode;
            cPressed = true;
            // 如果切回驾驶模式，可以重置一些相机状态
            if (driveMode) firstMouse = true;
        }
    }
    else { cPressed = false; }

    if (driveMode) {
        // --- 模式 A：驾驶赛车 ---
        bool w = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        bool s = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
        bool a = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
        bool d = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

        float groundY = terrain.getHeightWorld(myCar.Position.x, myCar.Position.z);
        myCar.Update(deltaTime, groundY, w, s, a, d);
    }
    else {
        // --- 模式 B：自由相机 ---
        // 只有在这里，WASD 才控制相机移动
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);

        // 自由模式下可以继续按 QE 调整 eyeHeight 方便观察
    }
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // 初始化 GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // ———————— 初始化从这开始 ———————— 
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 创建着色器（需准备 .vs 和 .fs 文件）
    Shader ourShader((std::string(SHADERS_FOLDER) + "model.vs").c_str(), (std::string(SHADERS_FOLDER) + "model.fs").c_str());
    
    std::map<std::string, std::string> carTextureMap;
    // 猫模型是obj和mtl所以textureMap可以为空
    std::map<std::string, std::string> cattextureMap;

    Model cat1((std::string(ASSETS_FOLDER)+"cat/Cat1/12221_Cat_v1_l3.obj").c_str(), (std::string(ASSETS_FOLDER) + "cat/Cat1/").c_str());
    Model mclaren((std::string(ASSETS_FOLDER)+"car/f1_2025_mclaren_mcl39.glb").c_str(), (std::string(ASSETS_FOLDER) + "car/").c_str());

    // 初始化、加载天空盒
    std::vector<std::string> faces = {
        ASSETS_FOLDER  "skybox/right.jpg",
        ASSETS_FOLDER  "skybox/left.jpg",
        ASSETS_FOLDER  "skybox/top.jpg",
        ASSETS_FOLDER  "skybox/bottom.jpg",
        ASSETS_FOLDER  "skybox/front.jpg",
        ASSETS_FOLDER  "skybox/back.jpg"
    };
    Skybox skybox(faces);

    // 初始化地形
    //Terrain terrain(
    //    ASSETS_FOLDER "terrain/heightmap_2049.png",
    //    1800.0f,
    //    64, 64, 33, 1.0f
    //);
    Terrain terrain(
        ASSETS_FOLDER "terrain/heightmap_4097.png",
        1800.0f,
        128, 128, 33, 1.0f
    );
    terrain.setLODDistances(200.0f, 600.0f, 1400.0f);

    // ———————— 渲染循环 ————————
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, myCar, terrain);

        if (driveMode) {
            // --- 1. 计算相机理想的目标位置 ---
            float distance = -1.5f;
            float height = 1.2f;

            // 基于赛车当前的朝向 (myCar.Heading) 计算车后方的点
            glm::vec3 offset;
            offset.x = -glm::sin(glm::radians(myCar.Heading)) * distance;
            offset.z = -glm::cos(glm::radians(myCar.Heading)) * distance;
            offset.y = height;

            glm::vec3 targetCameraPos = myCar.Position + offset;

            // --- 2. 平滑插值 (关键：让相机“飞”过去) ---
            // 0.1f 是平滑系数，数值越小越丝滑，越大越紧跟
            float smoothSpeed = 20.0f;
            camera.Position = glm::mix(camera.Position, targetCameraPos, smoothSpeed * deltaTime);

            // --- 3. 处理相机旋转 (平滑转向) ---
            // 让相机始终平滑地看向赛车中心点（或稍微靠前一点）
            float targetYaw = myCar.Heading + 90.0f;
            float targetPitch = -5.0f;

            // 对角度也进行平滑插值，防止赛车急速转弯时相机闪烁
			camera.Yaw = targetYaw; //glm::mix(camera.Yaw, targetYaw, smoothSpeed * deltaTime);
            camera.Pitch = targetPitch; //glm::mix(camera.Pitch, targetPitch, smoothSpeed * deltaTime);

            // 记得调用你 Camera 类的更新向量函数（注意大小写，通常是 updateCameraVectors）
            camera.UpdateVectors();
        }

        if (!topView) {
            // 获取当前位置的地形高度
            float groundY = terrain.getHeightWorld(camera.Position.x, camera.Position.z);

            // 关键：相机的 Y = 当前地形高度 + 我们的手动偏移量
            camera.Position.y = groundY + eyeHeight;
        }

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        // ------------模型可以共享的设置-------------
        // 设置 MVP
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 2.0f, 5000.0f); // 调整near和far，确保能看到远处的地形
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        // 光照
        ourShader.setVec3("viewPos", camera.Position);
        ourShader.setVec3("light.position", glm::vec3(0.0f, 1.0f, 10.0f));
        ourShader.setVec3("light.color", glm::vec3(1.0f, 1.0f, 1.0f));

        // ------------画猫的模型---------------------
        glm::vec3 catPos(-2.0f, 0.0f, -5.0f);   // 设置猫位置

        catPos.y = terrain.getHeightWorld(catPos.x, catPos.z) + 0.5f;     // 设置猫的高度：获取当前位置的地形高度+猫的原点距离脚底的距离

        // 设置模型矩阵
        glm::mat4 modelCat = glm::mat4(1.0f);
        // 先旋转后平移（OpenGL中，最后写的变换，最先作用在顶点上）
        modelCat = glm::translate(modelCat, catPos); // 放在左边
        // 旋转因子
        modelCat = glm::rotate(modelCat, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
        // 缩放因子
        modelCat = glm::scale(modelCat, glm::vec3(0.2f, 0.2f, 0.2f));
        ourShader.setMat4("model", modelCat);
        cat1.Draw(ourShader, modelCat);

        // ------------画赛车的模型---------------------
        //glm::vec3 carPos = glm::vec3(2.0f, 0.0f, -5.0f);    // 设置赛车位置

        //carPos.y = terrain.getHeightWorld(carPos.x, carPos.z) + 5.0f;   //设置赛车高度：获取当前位置的地形高度+车轮半径

        //// 设置模型矩阵
        //glm::mat4 modelCar = glm::mat4(1.0f);
        //// 平移因子
        //modelCar = glm::translate(modelCar, carPos); // 放在右边，略低一点
        //modelCar = glm::scale(modelCar, glm::vec3(1.0f, 1.0f, 1.0f));   // 赛车更小的缩放
        //// 缩放因子
        //ourShader.setMat4("model", modelCar);
        //mclaren.Draw(ourShader, modelCar);
        
        ourShader.use();

        // 获取地形法线
        glm::vec3 normal = terrain.getNormalWorld(myCar.Position.x, myCar.Position.z);
        // 赛车水平朝向向量
        glm::vec3 forward = glm::vec3(sin(glm::radians(myCar.Heading)), 0, cos(glm::radians(myCar.Heading)));
        // 计算贴地的坐标系
        glm::vec3 right = glm::normalize(glm::cross(forward, normal));
        glm::vec3 actualForward = glm::normalize(glm::cross(normal, right));

        // 构建旋转矩阵
        glm::mat4 rotation = glm::mat4(1.0f);
        rotation[0] = glm::vec4(right, 0.0f);
        rotation[1] = glm::vec4(normal, 0.0f);
        rotation[2] = glm::vec4(actualForward, 0.0f);

        glm::mat4 modelCar = glm::translate(glm::mat4(1.0f), myCar.Position);
        modelCar = modelCar * rotation; // 应用地形倾斜旋转
        modelCar = glm::scale(modelCar, glm::vec3(1.0f));

        ourShader.setMat4("model", modelCar);
        mclaren.Draw(ourShader, modelCar, myCar);

        // -------------画地形---------------------------
        terrain.render(view, projection, camera.Position);

        // ------------画天空盒（最后画天空盒！！不然其它模型会被它挡住）-----------------------
        skybox.draw(view, projection);
        // ------------画天空盒-----------------------------------------------------------------

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;

}
