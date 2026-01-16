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
        shader.setVec3("Material_baseColor", baseColor);
        bool hasDiffuseMap = false;

        // 只需要找到【第一张】漫反射贴图即可，因为你的 Shader 目前只支持一个采样器
        for (unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            std::string name = textures[i].type;

            // 关键逻辑：无论模型里有多少贴图，我们只把类型为 "texture_diffuse" 
            // 且是第一张发现的贴图绑定给 shader 里的 texture_diffuse1
            if (!hasDiffuseMap && name == "texture_diffuse") {
                shader.setInt("texture_diffuse1", i); // 这里的 i 是纹理单元索引
                hasDiffuseMap = true;
            }

            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        shader.setBool("hasTexture", hasDiffuseMap);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
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
        // GLB 通常走这里：数据是压缩格式 (PNG/JPG)
        // 关键：GLTF 贴图标准是左上角。我们在加载时不翻转，
        // 配合顶点处理中的 v = v 逻辑。
        stbi_set_flip_vertically_on_load(false);
        data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(aiTex->pcData), aiTex->mWidth, &width, &height, &nrComponents, 0);
    }
    else {
        // 原始数据格式
        width = aiTex->mWidth;
        height = aiTex->mHeight;
        data = reinterpret_cast<unsigned char*>(aiTex->pcData);
        nrComponents = 4;
    }

    if (data) {
        GLenum format = GL_RGBA;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);

        // 【关键修复 1】：强制 1 字节对齐，防止宽度非 4 倍数导致的斜切/错位
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // 【关键修复 2】：对于 GLB 这种紧凑贴图，强烈建议使用 CLAMP_TO_EDGE
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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
        // 检查是否为GLB
        bool isGLB = (path.find(".glb") != std::string::npos || path.find(".gltf") != std::string::npos);

        unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_FlipUVs;


        // 步骤 1：验证问题时，可以添加 aiProcess_PreTransformVertices
        // 但为了后续能让赛车“动起来”，我们不使用它，而是通过代码处理变换
        scene = importer.ReadFile(path, flags);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "Assimp error: " << importer.GetErrorString() << std::endl;
            return;
        }
        directory = path.substr(0, path.find_last_of('/'));

        // 必须清空，防止多次加载/残留
        meshes.clear();
        processNode(scene->mRootNode, scene, isGLB);
        
    }


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

    void processNode(aiNode* node, const aiScene* scene, bool isGLB) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene, isGLB));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene, isGLB);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene, bool isGLB) {
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
           /* else
                vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);*/
            // 纹理坐标 (仅处理第一组 UV)
            // 在 processMesh 函数的纹理坐标循环中：
            if (mesh->mTextureCoords[0]) {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;

                // 如果你在加载图片时 stbi_set_flip_vertically_on_load(false);
                // 那么对于 GLB，这里通常不需要处理。
                // 如果发现贴图上下颠倒，请将下一行取消注释：
                // vec.y = 1.0f - vec.y; 

                vertex.TexCoords = vec;
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
        
        // 1. 获取漫反射贴图 (针对 OBJ)
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);

        // 2. 如果是 GLB 或 Diffuse 为空，尝试加载 BaseColor (针对 PBR GLB)
        if (diffuseMaps.empty()) {
            diffuseMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_diffuse", scene);
        }
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        // 3. 处理颜色 (Vector3)
        aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
        if (AI_SUCCESS != aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &color)) {
            aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &color);
        }

        return Mesh(vertices, indices, textures, glm::vec3(color.r, color.g, color.b));
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