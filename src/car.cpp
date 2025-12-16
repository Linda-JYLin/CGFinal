#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

// *** 外部依赖项 ***
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// 纹理加载库 (stb_image)
#define STB_IMAGE_IMPLEMENTATION
// 假设 stb_image.h 文件已放置在 include 路径中
#include "stb_image.h" 

// *******************************************************************
// 1. 路径定义
// *******************************************************************
// 路径相对于项目根目录 E:\CGFINAL
const std::string PROJECT_ROOT = "E:/CGFINAL/";

const std::string MODEL_PATH = PROJECT_ROOT + "src/assets/source/2015 McLaren P1 GTR.glb";
const std::string TEXTURES_DIR = PROJECT_ROOT + "src/assets/textures/";
const std::string VERTEX_SHADER_PATH = PROJECT_ROOT + "simple.vs";
const std::string FRAGMENT_SHADER_PATH = PROJECT_ROOT + "simple.fs";

// *******************************************************************
// 2. 辅助结构体和类 (Vertex, Texture)
// *******************************************************************

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    unsigned int id;
    std::string type; // 例如 "texture_diffuse"
    aiString path;
};

// *******************************************************************
// 3. Shader 类实现
// *******************************************************************

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
            std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        unsigned int vertex, fragment;
        int success;
        char infoLog[512];

        // 顶点着色器
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        };

        // 片段着色器
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        };

        // 着色器程序
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(ID, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use() { glUseProgram(ID); }
    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }
    void setInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
};

// *******************************************************************
// 4. Texture 辅助函数
// *******************************************************************

unsigned int TextureFromFile(const char* path, const std::string& directory) {
    std::string filename = std::string(path);
    filename = directory + "/" + filename; // 拼接完整路径

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    // 注意：stbi_load 不支持 .etc_x.png 这种非标准的文件名，它只看文件内容。
    // 如果您的纹理是 ETC 压缩格式并使用 stb_image 加载，可能会失败。
    // 但对于标准 PNG，这是可行的。
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;
        else {
            std::cerr << "Texture format not supported for " << filename << std::endl;
            stbi_image_free(data);
            return 0;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // 设置纹理参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cerr << "Texture failed to load at path: " << filename << std::endl;
        stbi_image_free(data);
        return 0;
    }

    return textureID;
}

// *******************************************************************
// 5. Mesh 类实现
// *******************************************************************

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    unsigned int VAO;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
        : vertices(std::move(vertices)), indices(std::move(indices)), textures(std::move(textures)) {
        setupMesh();
    }

    void Draw(Shader& shader) {
        unsigned int diffuseNr = 1;

        // 绑定纹理
        for (unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i); // 激活相应的纹理单元
            std::string number;
            std::string name = textures[i].type;
            if (name == "texture_diffuse") number = std::to_string(diffuseNr++);

            shader.setInt((name + number).c_str(), i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        // 绘制 Mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // 重置/清理状态
        glActiveTexture(GL_TEXTURE0);
    }

private:
    unsigned int VBO, EBO;

    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // 顶点位置
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // 顶点法线
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // 纹理坐标
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }
};

// *******************************************************************
// 6. Model 类实现
// *******************************************************************

class Model {
public:
    std::vector<Texture> textures_loaded;
    std::vector<Mesh> meshes;
    std::string directory;

    Model(const std::string& path) {
        // 假设 Model 文件路径是 "src/assets/source/model.glb"
        // directory 应该被设置为 "src/assets/source"
        directory = path.substr(0, path.find_last_of('/'));
        // 纹理路径应该基于项目根目录，所以我们调整 directory 来加载纹理
        directory = TEXTURES_DIR.substr(0, TEXTURES_DIR.size() - 1); // TEXTURES_DIR 是 "src/assets/textures/"

        loadModel(path);
    }

    void Draw(Shader& shader) {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

private:
    std::map<std::string, std::string> textureMap = {
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

    void loadModel(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return;
        }

        std::cout << "Model loaded successfully: " << path << std::endl;
        processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode* node, const aiScene* scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        // 处理顶点
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            // 位置
            vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            // 法线
            if (mesh->HasNormals())
                vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            // 纹理坐标 (仅处理第一组 UV)
            if (mesh->mTextureCoords[0])
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }

        // 处理索引
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // 处理材质
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // 我们只关注漫反射贴图 (Diffuse Maps)
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        return Mesh(vertices, indices, textures);
    }

    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName) {
        std::vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            std::string textureKey = str.C_Str();

            // 检查 Assimp 返回的路径是否是内部索引 (*N)
            if (textureKey.rfind('*', 0) == 0 && textureMap.count(textureKey)) {
                // 如果是内部索引，从我们手动创建的映射表中获取实际的文件名
                std::string actualFileName = textureMap.at(textureKey);

                // 检查是否已加载纹理
                // 检查是否已加载纹理
                bool skip = false;
                for (unsigned int j = 0; j < textures_loaded.size(); j++) {
                    // ****** 修正点：使用 .path.C_Str() 将 aiString 转换为 const char* 进行比较 ******
                    if (std::string(textures_loaded[j].path.C_Str()) == actualFileName) {
                        textures.push_back(textures_loaded[j]);
                        skip = true;
                        break;
                    }
                }

                if (!skip) {
                    // 如果未加载，则加载新纹理
                    Texture texture;
                    // 使用 TEXTURES_DIR 和实际文件名来加载
                    texture.id = TextureFromFile(actualFileName.c_str(), TEXTURES_DIR);
                    texture.type = typeName;
                    texture.path = actualFileName; // 存储实际文件名
                    textures.push_back(texture);
                    textures_loaded.push_back(texture);
                }
            }
            else {
                // 如果 Assimp 返回的是正常的外部文件名（不太可能是您的 .glb 文件）
                // 您可以在这里添加处理逻辑。
                std::cerr << "Warning: Texture path is not a recognized index: " << textureKey << std::endl;
            }
        }
        return textures;
    }
};

// *******************************************************************
// 7. Main 函数和 OpenGL 初始化
// *******************************************************************

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "McLaren P1 GTR Viewer", NULL, NULL);
    if (window == NULL) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (glewInit() != GLEW_OK) { std::cerr << "Failed to initialize GLEW" << std::endl; return -1; }

    glEnable(GL_DEPTH_TEST);

    // ** 加载着色器 **
    Shader ourShader(VERTEX_SHADER_PATH.c_str(), FRAGMENT_SHADER_PATH.c_str());

    // ** 关键步骤：加载模型 **
    // 再次提醒：程序的工作目录必须是 E:\CGFINAL
    Model mclaren(MODEL_PATH);

    // 设置相机和变换矩阵
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)800 / (float)600, 0.1f, 100.0f);

    // 初始 View 矩阵：将相机放在 (0, 0, 5) 并看向原点
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));

    ourShader.use();
    ourShader.setMat4("projection", projection);
    ourShader.setMat4("view", view);


    // 渲染循环
    while (!glfwWindowShouldClose(window)) {
        // 清除
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 旋转模型 (示例：让模型绕 Y 轴旋转)
        float angle = (float)glfwGetTime() * 0.5f;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f)); // 稍微向下移动，以适应车辆模型

        ourShader.use();
        ourShader.setMat4("model", model);

        // 绘制模型
        mclaren.Draw(ourShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}