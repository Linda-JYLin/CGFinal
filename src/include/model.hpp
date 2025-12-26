#include "include/shader.hpp"
#include <map>

// ———————— 网格类（Mesh） ————————
// 顶点类
struct Vertex {
    glm::vec3 Position; // 位置
    glm::vec3 Normal;   // 法线
    glm::vec2 TexCoords;
};

// 纹理类
struct Texture {
    unsigned int id;
    std::string type; 
    aiString path;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    unsigned int VAO, VBO, EBO;
    glm::vec3 baseColor;    //用于存储材料基础颜色

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures, glm::vec3 color = glm::vec3(1.0f))
        : vertices(std::move(vertices)), indices(std::move(indices)), textures(std::move(textures)), baseColor(color) {
        setupMesh();
    }

    void Draw(Shader& shader) {
        unsigned int diffuseNr = 1;
        bool hasDiffuseMap = false;

        // 向shader传递颜色
        shader.setVec3("Material_baseColor", baseColor);

        // 绑定纹理
        for (unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i); // 激活相应的纹理单元
            std::string name = textures[i].type;
            if (name == "texture_diffuse") {
                hasDiffuseMap = true;
                shader.setInt("texture_diffuse1", i);
            }
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        shader.setBool("hasTexture", hasDiffuseMap);

        // 绘制 Mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // 重置/清理状态
        glActiveTexture(GL_TEXTURE0);
    }

private:
    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // tex coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }
};

// ———————— Model 类（使用 Assimp） ————————

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
        if (nrComponents == 1) { format = GL_RED; } //std::cerr << "nrComponents == 1" << filename << std::endl; }
        else if (nrComponents == 3) { format = GL_RGB; }// std::cerr << "nrComponents == 3" << filename << std::endl; }
        else if (nrComponents == 4) { format = GL_RGBA; }//std::cerr << "nrComponents == 4" << filename << std::endl;}
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

unsigned int TextureFromMemory(const aiTexture* aiTex) {
    if (!aiTex) return 0;
    
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = nullptr;

    if (aiTex->mHeight == 0) {
        // 数据是压缩格式 (PNG/JPG)，使用 stb_image 从内存解码
        data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(aiTex->pcData), aiTex->mWidth, &width, &height, &nrComponents, 0);
    }
    else {
        // 数据是原始格式 (RGBA)
        data = reinterpret_cast<unsigned char*>(aiTex->pcData);
        width = aiTex->mWidth;
        height = aiTex->mHeight;
        nrComponents = 4;
    }

    if (data) {
        GLenum format = (nrComponents == 1) ? GL_RED : (nrComponents == 3 ? GL_RGB : GL_RGBA);

        glBindTexture(GL_TEXTURE_2D, textureID);
        // 关键修复：解决某些图片宽度不是4的倍数导致的崩溃
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // 设置参数以确保能显示
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (aiTex->mHeight == 0) stbi_image_free(data);
        return textureID;
    }
    return 0;
}

unsigned int TextureFromRawData(void* data, unsigned int width, unsigned int height) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    glBindTexture(GL_TEXTURE_2D, textureID);
    // 直接上传原始像素数据，跳过解码步骤
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    return textureID;
}

class Model {
public:
    std::vector<Texture> textures_loaded;
    std::vector<Mesh> meshes;
    std::string directory;
    std::map<std::string, std::string> textureMap;
    std::string TEXTURES_DIR;

    // --- 新增成员 ---
    Assimp::Importer importer; // 必须作为成员变量，否则函数结束 scene 内存会被释放
    const aiScene* scene = nullptr;

    // 修改构造函数：移除暂时不需要的 map 以简化逻辑
    Model(const std::string& path, const std::string& texture_path) : TEXTURES_DIR(texture_path) {
        loadModel(path);
    }

    // 增加 baseTransform 参数
    void Draw(Shader& shader, glm::mat4 baseTransform = glm::mat4(1.0f)) {
        if (scene && scene->mRootNode) {
            drawNode(scene->mRootNode, shader, baseTransform);
        }
    }

private:
    void loadModel(const std::string& path) {
        // 步骤 1：验证问题时，可以添加 aiProcess_PreTransformVertices
        // 但为了后续能让赛车“动起来”，我们不使用它，而是通过代码处理变换
        scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "Assimp error: " << importer.GetErrorString() << std::endl;
            return;
        }
        directory = path.substr(0, path.find_last_of('/'));

        // 必须清空，防止多次加载/残留
        meshes.clear();

        // 步骤 2：预处理所有 Mesh 并存入 meshes 数组
        // 注意：这里需要确保 meshes 的索引与 scene->mMeshes 一一对应
        for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
            meshes.push_back(processMesh(scene->mMeshes[i], scene));
        }
    }

    // --- 核心修复：递归处理变换矩阵并绘制 ---
    //void drawNode(aiNode* node, Shader& shader, glm::mat4 parentTransform) {
    //    // 1. 将 Assimp 变换矩阵转换为 GLM 矩阵
    //    glm::mat4 nodeTransform = convertMatrixToGLM(node->mTransformation);

    //    // 2. 计算当前节点的全局变换（父节点矩阵 * 当前节点矩阵）
    //    glm::mat4 globalTransform = parentTransform * nodeTransform;

    //    // 3. 绘制该节点下的所有 Mesh
    //    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
    //        // node->mMeshes 存储的是该节点引用的 scene->mMeshes 的索引
    //        unsigned int meshIndex = node->mMeshes[i];

    //        // 将最终的矩阵传给 Shader 的 "model" uniform
    //        shader.setMat4("model", globalTransform);
    //        meshes[meshIndex].Draw(shader);
    //    }

        void drawNode(aiNode* node, Shader& shader, glm::mat4 parentTransform) {
            glm::mat4 nodeTransform = convertMatrixToGLM(node->mTransformation);
            glm::mat4 globalTransform = parentTransform * nodeTransform;

            for (unsigned int i = 0; i < node->mNumMeshes; i++) {
                shader.setMat4("model", globalTransform); // 这里现在会包含 main 传进来的 baseTransform
                meshes[node->mMeshes[i]].Draw(shader);
            }

        // 4. 递归处理子节点
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            drawNode(node->mChildren[i], shader, globalTransform);
        }
    }

    // 辅助函数：Assimp 矩阵转 GLM 矩阵
    glm::mat4 convertMatrixToGLM(const aiMatrix4x4& from) {
        glm::mat4 to;
        // 注意：Assimp 的 a1, a2, a3, a4 对应 GLM 的第一列
        to[0][0] = from.a1; to[0][1] = from.b1; to[0][2] = from.c1; to[0][3] = from.d1;
        to[1][0] = from.a2; to[1][1] = from.b2; to[1][2] = from.c2; to[1][3] = from.d2;
        to[2][0] = from.a3; to[2][1] = from.b3; to[2][2] = from.c3; to[2][3] = from.d3;
        to[3][0] = from.a4; to[3][1] = from.b4; to[3][2] = from.c4; to[3][3] = from.d4;
        return to;
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
            if (mesh->mNormals)
                vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            else
                vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            // 纹理坐标 (仅处理第一组 UV)
            if (mesh->mTextureCoords[0]) {
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            }
            else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }
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

        // --- 提取材质基础颜色 ---
        aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
        // GLTF 规范通常使用 Base Color
        if (AI_SUCCESS != aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &color)) {
            // 如果没拿到 Base Color，尝试拿 Diffuse Color
            aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &color);
        }
        glm::vec3 meshColor(color.r, color.g, color.b);

        // 我们只关注漫反射贴图 (Diffuse Maps)
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", scene);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        std::vector<Texture> metallicMaps = loadMaterialTextures(material, aiTextureType_METALNESS, "texture_metallic", scene);
        textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());

        return Mesh(vertices, indices, textures, meshColor);
    }
    
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, const aiScene* scene) {
        std::vector<Texture> textures;
        //if (!mat || !scene) return textures; // 防御性编程

        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);

            std::string textureKey = str.C_Str();

            // Step 1: 检查是否已加载
            bool skip = false;
            for (const auto& loadedTex : textures_loaded) {
                if (loadedTex.path == str) {
                    textures.push_back(loadedTex);
                    skip = true;
                    break;
                }
            }
            if (skip) continue;

            Texture texture;
            texture.type = typeName;
            texture.path = str; // 保留原始路径用于查重
            texture.id = 0;

            std::string actualFilename;

            // Step 2: 判断是内部索引还是外部文件
            //if (textureKey.rfind("*", 0) == 0) { //内部索引
            //    if (textureMap.find(textureKey) != textureMap.end()) {
            //        actualFilename = textureMap[textureKey]; // 从用户提供的映射表获取真实文件名
            //    }
            //    else {
            //        std::cerr << "Error: No mapping found for internal texture: " << textureKey << std::endl;
            //        continue;
            //    }
            //}
            //else {
            //    // 普通外部文件路径（如 "Cat_diffuse.jpg"）
            //    actualFilename = textureKey;
            //}
            if (textureKey.size() > 0 && textureKey[0] == '*') {
                // 如果是内嵌贴图，直接从 scene 中获取
                const aiTexture* aiTex = scene->GetEmbeddedTexture(textureKey.c_str());
                if (aiTex) {
                    texture.id = TextureFromMemory(aiTex);
                }
            }
            else if (!textureKey.empty()) {
                // 处理外部文件 (OBJ)
                texture.id = TextureFromFile(textureKey.c_str(), TEXTURES_DIR);
            }

            // Step 3: 加载纹理
            /*unsigned int textureID = TextureFromFile(actualFilename.c_str(), TEXTURES_DIR);
            if (textureID == 0) {
                std::cerr << "Failed to load texture: " << actualFilename << " from dir: " << TEXTURES_DIR << std::endl;
                continue;
            }*/

            if(texture.id != 0) {
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
            else {
                std::cerr << "Failed to load texture: " << textureKey << " from dir: " << TEXTURES_DIR << std::endl;
            }
        }
        return textures;
    }
};